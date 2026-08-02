import { fetchText, fetchJson } from '../../../common/http/fetch';
import type { HotListSource } from '../hot-list.types';
import { firstMatch, htmlBlockMatches } from '../html-utils';
import { absoluteUrl, compactHot } from '../hot-list.utils';
import { defineDirectSource } from '../source-factory';

interface SteamResponse {
  topPlayed?: {
    data?: Array<{
      appid?: number;
      name?: string;
      currentPlayers?: number;
    }>;
  };
}

export const NEWSNOW_DIRECT_SOURCES: readonly HotListSource[] = [
  defineDirectSource({
    id: 'steam',
    label: 'Steam',
    async fetch(signal) {
      const json = await fetchJson<SteamResponse>('https://store.steampowered.com/stats/stats/', { signal });
      return (json.topPlayed?.data ?? []).map((item, index) => ({
        title: item.name ?? '',
        hot: compactHot(item.currentPlayers),
        rank: index + 1,
        url: `https://store.steampowered.com/app/${item.appid}/`,
      }));
    },
  }),
  defineDirectSource({
    id: 'zaobao',
    label: '联合早报',
    async fetch(signal) {
      const html = await fetchText('https://www.zaobao.com/realtime/', { signal });
      return htmlBlockMatches(
        html,
        /<article[^>]*class="[^"]*news-item[^"]*"[^>]*>([\s\S]*?)<\/article>/gi
      ).map((match: string[], index: number) => {
        const block = match[1] ?? '';
        const link = firstMatch(block, /<a[^>]+href="([^"]+)"/i);
        return {
          title: firstMatch(block, /<h3[^>]*>([\s\S]*?)<\/h3>/i) ?? '',
          timestamp: firstMatch(block, /<time[^>]*datetime="([^"]+)"[^>]*>/i),
          rank: index + 1,
          url: absoluteUrl('https://www.zaobao.com', link),
        };
      });
    },
  }),
];
