import { Injectable } from '@nestjs/common';
import { WeatherConfig, type WeatherConfigT } from 'shared';
import { z } from 'zod';
import { fetchJson as fetchJsonWithTimeout } from '../../../common/http/fetch';
import { getDateTimeFormat } from '../../../common/utils/intl';
import type { DataProvider, DynamicContentFetchCtx } from '../dynamic-content.types';
import { datePartsInTz } from '../timezone';
import {
  CachedInflightFetcher,
  DEFAULT_PROVIDER_CACHE_TTL_SEC,
  DEFAULT_PROVIDER_FETCH_TIMEOUT_MS,
  isRecentTimestamp,
} from './provider-cache';

export interface WeatherForecastDay {
  label: string;
  val: string;
  text: string;
  tempMin: number | string;
  tempMax: number | string;
  code: number;
}

export interface WeatherProviderData {
  tempC: number | string;
  feelsLikeC: number | string;
  humidity: number | string;
  pressure: number | string;
  windDisplay: string;
  summary: string;
  code: number;
  obsTime: string;
  updatedAt: string;
  fc: WeatherForecastDay[];
}

interface HkoCurrentWx {
  btime?: string;
  temp?: string;
  rh?: string;
}

interface HkoRhrRead {
  BulletinTime?: string;
  hkotemp?: string;
  hkorh?: string;
  FormattedObsTime?: string;
}

interface HkoMain {
  BulletinTime?: string;
  Temperature?: string;
  RH?: string;
  HomeMaxTemperature?: string;
  HomeMinTemperature?: string;
}

interface HkoForecastDay {
  ForecastDate?: string;
  ForecastWind?: string;
  ForecastWeather?: string;
  ForecastMaxtemp?: string;
  ForecastMintemp?: string;
  ForecastMaxrh?: string;
  ForecastMinrh?: string;
  ForecastIcon?: string;
  PSR?: string;
  IconDesc?: string;
}

interface HkoF9D {
  BulletinDate?: string;
  GeneralSituation?: string;
  WeatherForecast?: HkoForecastDay[];
}

interface HkoApiResponse {
  currwx?: HkoCurrentWx;
  RHRREAD?: HkoRhrRead;
  hko?: HkoMain;
  F9D?: HkoF9D;
}

export interface WeatherCitySearchResult {
  id: string;
  name: string;
  adm1: string;
  adm2: string;
}

const FC_LABELS = ['今日', '明日', '后天'];
const MAX_CACHE_ENTRIES = 128;
const HKO_API_URL = 'https://www.hko.gov.hk/wxinfo/json/one_json.xml';

@Injectable()
export class WeatherProvider implements DataProvider<WeatherConfigT, WeatherProviderData> {
  readonly type = 'weather';
  private readonly fetcher = new CachedInflightFetcher<string, WeatherProviderData>(
    MAX_CACHE_ENTRIES
  );

  constructor() {}

  validateConfig(raw: unknown): WeatherConfigT {
    return WeatherConfig.parse(raw);
  }

  async searchCities(
    _query: string,
    _limit = 8,
    _now = Date.now()
  ): Promise<WeatherCitySearchResult[]> {
    return [
      { id: 'hong-kong', name: '香港', adm1: '香港', adm2: '香港' },
    ];
  }

  private cacheKey(_c: WeatherConfigT): string {
    return 'hko:hk:Asia/Hong_Kong';
  }

  async fetchData(
    config: WeatherConfigT,
    ctx: DynamicContentFetchCtx
  ): Promise<WeatherProviderData> {
    const key = this.cacheKey(config);
    const now = ctx.now.getTime();
    const ttlSec = Math.max(config.refresh_interval_sec ?? DEFAULT_PROVIDER_CACHE_TTL_SEC, 300);
    return this.fetcher.getOrFetch(key, now, ttlSec * 1000, () =>
      this.fetchFromHKO(config, ctx)
    );
  }

  private async fetchFromHKO(
    config: WeatherConfigT,
    ctx: DynamicContentFetchCtx
  ): Promise<WeatherProviderData> {
    const json = await fetchJson<HkoApiResponse>(HKO_API_URL);

    const currwx = json.currwx ?? {};
    const hko = json.hko ?? {};
    const f9d = json.F9D ?? {};

    const tempC = toDisplayNumber(currwx.temp ?? hko.Temperature);
    const humidity = toDisplayNumber(currwx.rh ?? hko.RH);

    let summary = (f9d.GeneralSituation ?? '').trim();
    summary = stripHtmlTags(summary);

    const fc = (f9d.WeatherForecast ?? []).slice(0, 3).map((day, index) => {
      const weatherText = day.IconDesc ?? day.ForecastWeather ?? '--';
      const tempMin = toDisplayNumber(day.ForecastMintemp);
      const tempMax = toDisplayNumber(day.ForecastMaxtemp);
      return {
        label: forecastLabelHKO(day.ForecastDate, config.tz, ctx.now) ?? FC_LABELS[index] ?? '--',
        val: `${weatherText}  ${tempMin}~${tempMax}°`,
        text: weatherText,
        tempMin,
        tempMax,
        code: Number.parseInt(day.ForecastIcon ?? '999', 10),
      };
    });

    while (fc.length < 3) {
      fc.push({
        label: FC_LABELS[fc.length]!,
        val: '--',
        text: '--',
        tempMin: '--',
        tempMax: '--',
        code: 999,
      });
    }

    const obsTime = currwx.btime
      ? formatHkoTime(currwx.btime)
      : ctx.now.toISOString();

    return {
      tempC,
      feelsLikeC: tempC,
      humidity,
      pressure: '--',
      windDisplay: '--',
      summary: summary || '--',
      code: Number.parseInt((f9d.WeatherForecast ?? [{}])[0]?.ForecastIcon ?? '999', 10),
      obsTime,
      updatedAt: obsTime,
      fc,
    };
  }

  private fallbackFromLastData(
    config: WeatherConfigT,
    lastData: unknown,
    now: Date
  ): WeatherProviderData | null {
    const parsed = WeatherProviderDataFallback.safeParse(lastData);
    if (!parsed.success) return null;
    const data = parsed.data;
    if (!data.summary && data.tempC === undefined) return null;
    if (!isRecentTimestamp(data.updatedAt, now, reusableWeatherAgeMs(config))) return null;
    return {
      tempC: data.tempC ?? '--',
      feelsLikeC: data.feelsLikeC ?? '--',
      humidity: data.humidity ?? '--',
      pressure: data.pressure ?? '--',
      windDisplay: data.windDisplay ?? '--',
      summary: data.summary ?? '--',
      code: typeof data.code === 'number' ? data.code : 999,
      obsTime: data.obsTime ?? now.toISOString(),
      updatedAt: data.updatedAt ?? now.toISOString(),
      fc: Array.isArray(data.fc) ? data.fc.slice(0, 3) : [],
    };
  }
}

function reusableWeatherAgeMs(config: WeatherConfigT): number {
  const ttlSec = Math.max(config.refresh_interval_sec ?? DEFAULT_PROVIDER_CACHE_TTL_SEC, 300);
  return Math.min(Math.max(ttlSec * 3, 900), 43_200) * 1000;
}

async function fetchJson<T>(url: string): Promise<T> {
  return fetchJsonWithTimeout<T>(url, {
    timeoutMs: DEFAULT_PROVIDER_FETCH_TIMEOUT_MS,
    headers: {},
    userAgent: null,
  });
}

const WeatherForecastDayFallback = z.object({
  label: z.string(),
  val: z.string(),
  text: z.string(),
  tempMin: z.union([z.number(), z.string()]),
  tempMax: z.union([z.number(), z.string()]),
  code: z.number(),
});

const WeatherProviderDataFallback = z.object({
  tempC: z.union([z.number(), z.string()]).optional(),
  feelsLikeC: z.union([z.number(), z.string()]).optional(),
  humidity: z.union([z.number(), z.string()]).optional(),
  pressure: z.union([z.number(), z.string()]).optional(),
  windDisplay: z.string().optional(),
  summary: z.string().optional(),
  code: z.number().optional(),
  obsTime: z.string().optional(),
  updatedAt: z.string().optional(),
  fc: z.array(WeatherForecastDayFallback).optional(),
});

function toDisplayNumber(value: unknown): number | string {
  if (typeof value === 'number' && Number.isFinite(value)) return Math.round(value);
  if (typeof value === 'string' && value.trim()) {
    const n = Number(value);
    return Number.isFinite(n) ? Math.round(n) : value;
  }
  return '--';
}

function stripHtmlTags(html: string): string {
  return html.replace(/<[^>]*>/g, '').replace(/\s+/g, ' ').trim();
}

function formatHkoTime(btime: string): string {
  const y = Number.parseInt(btime.slice(0, 4), 10);
  const m = Number.parseInt(btime.slice(4, 6), 10);
  const d = Number.parseInt(btime.slice(6, 8), 10);
  const h = Number.parseInt(btime.slice(8, 10), 10);
  const min = Number.parseInt(btime.slice(10, 12), 10);
  if (Number.isNaN(y) || y < 2000) return new Date().toISOString();
  const date = new Date(Date.UTC(y, m - 1, d, h, min));
  return date.toISOString();
}

export function forecastLabelHKO(value: unknown, timeZone: string, now: Date): string | null {
  if (typeof value !== 'string' || !value) return '--';
  const year = Number.parseInt(value.slice(0, 4), 10);
  const month = Number.parseInt(value.slice(4, 6), 10);
  const day = Number.parseInt(value.slice(6, 8), 10);
  if (!year || !month || !day) return value;
  const today = datePartsInTz(now, timeZone);
  if (today) {
    const delta = ordinalDay(year, month, day) - ordinalDay(today.year, today.month, today.day);
    if (delta >= 0 && delta < FC_LABELS.length) return FC_LABELS[delta]!;
  }
  const date = new Date(Date.UTC(year, month - 1, day, 12, 0, 0));
  if (Number.isNaN(date.getTime())) return value.slice(4);
  try {
    return getDateTimeFormat('zh-CN', {
      timeZone,
      month: 'numeric',
      day: 'numeric',
    }).format(date);
  } catch {
    return value.slice(4);
  }
}

function ordinalDay(year: number, month: number, day: number): number {
  return Math.floor(Date.UTC(year, month - 1, day) / 86_400_000);
}