import { afterEach, describe, expect, it } from 'bun:test';
import { forecastLabelHKO, WeatherProvider } from './weather.provider';

const originalFetch = globalThis.fetch;

afterEach(() => {
  globalThis.fetch = originalFetch;
});

describe('forecastLabelHKO', () => {
  it('labels forecast dates across year boundaries', () => {
    const now = new Date('2026-12-31T04:00:00.000Z');

    expect(forecastLabelHKO('20270101', 'Asia/Shanghai', now)).toBe('明日');
    expect(forecastLabelHKO('20270102', 'Asia/Shanghai', now)).toBe('后天');
  });

  it('returns HK for city search', async () => {
    const provider = new WeatherProvider();
    const results = await provider.searchCities('anything');
    expect(results).toEqual([{ id: 'hong-kong', name: '香港', adm1: '香港', adm2: '香港' }]);
  });

  it('fetches HKO data and maps fields correctly', async () => {
    globalThis.fetch = (async (_input: Parameters<typeof fetch>[0]) => {
      return Response.json({
        currwx: { btime: '202605181410', temp: '26.1', rh: '92' },
        hko: { Temperature: '26.1', RH: '92' },
        F9D: {
          BulletinDate: '20260518',
          GeneralSituation: '&#x4eca;&#x65e5;&#x591a;&#x4e91;',
          WeatherForecast: [
            {
              ForecastDate: '20260518',
              ForecastWeather: '&#x591a;&#x4e91;',
              ForecastMaxtemp: '30',
              ForecastMintemp: '25',
              ForecastIcon: '62',
              IconDesc: '&#x5c11;&#x96e8;',
            },
            {
              ForecastDate: '20260519',
              ForecastWeather: '&#x9633;',
              ForecastMaxtemp: '31',
              ForecastMintemp: '26',
              ForecastIcon: '101',
              IconDesc: '&#x9633;',
            },
          ],
        },
      });
    }) as unknown as typeof fetch;

    const provider = new WeatherProvider();
    const config = provider.validateConfig({
      type: 'weather',
      tz: 'Asia/Hong_Kong',
      provider: 'qweather',
      location_id: 'hong-kong',
      location_label: '&#x9999;&#x6e2f;',
      refresh_interval_sec: 600,
    });

    const data = await provider.fetchData(config, {
      now: new Date('2026-05-18T06:10:00.000Z'),
      lastData: null,
    });

    expect(data.tempC).toBe(26);
    expect(data.humidity).toBe(92);
    expect(data.fc.length).toBeGreaterThanOrEqual(2);
    expect(data.fc[0].tempMax).toBe(30);
    expect(data.fc[0].tempMin).toBe(25);
  });
});