import { fetchText } from '../../../common/http/fetch';
import type { HotListSource } from '../hot-list.types';
import { firstMatch, htmlBlockMatches, htmlBlocks, stripHtml } from '../html-utils';
import { absoluteUrl, compactHot } from '../hot-list.utils';
import { defineDirectSource } from '../source-factory';

export const DAILY_HOT_DIRECT_SOURCES: readonly HotListSource[] = [
  defineDirectSource({
    id: 'hackernews',
    label: 'Hacker News',
    async fetch(signal) {
      const html = await fetchText('https://news.ycombinator.com', { signal });
      const scores = new Map(
        Array.from(html.matchAll(/id="score_(\d+)"[^>]*>(\d+)\s+points?/gi), (match) => [
          match[1] ?? '',
          Number(match[2]),
        ])
      );
      return htmlBlockMatches(
        html,
        /<tr[^>]*class="[^"]*\bathing\b[^"]*"[^>]*id="([^"]+)"[^>]*>([\s\S]*?)<\/tr>/gi
      ).map((match: string[], index: number) => {
        const id = match[1] ?? '';
        const block = match[2] ?? '';
        const url = firstMatch(
          block,
          /<span[^>]*class="[^"]*titleline[^"]*"[^>]*>\s*<a[^>]+href="([^"]+)"/i
        );
        return {
          title: firstMatch(block, /<span[^>]*class="[^"]*titleline[^"]*"[^>]*>([\s\S]*?)<\/span>/i) ?? '',
          hot: compactHot(scores.get(id), '分'),
          rank: index + 1,
          url: absoluteUrl('https://news.ycombinator.com/', url),
        };
      });
    },
  }),
  defineDirectSource({
    id: 'nytimes',
    label: '纽约时报',
    async fetch(signal) {
      const xml = await fetchText('https://cn.nytimes.com/rss/', { signal });
      return htmlBlocks(xml, /<item\b[^>]*>([\s\S]*?)<\/item>/gi).map((block: string, index: number) => ({
        title: firstMatch(block, /<title\b[^>]*>([\s\S]*?)<\/title>/i) ?? '',
        desc: stripHtml(firstMatch(block, /<description\b[^>]*>([\s\S]*?)<\/description>/i)),
        timestamp: firstMatch(block, /<pubDate\b[^>]*>([\s\S]*?)<\/pubDate>/i),
        rank: index + 1,
        url: firstMatch(block, /<link\b[^>]*>([\s\S]*?)<\/link>/i),
      }));
    },
  }),
  defineDirectSource({
    id: 'producthunt',
    label: 'Product Hunt',
    async fetch(signal) {
      const xml = await fetchText('https://www.producthunt.com/feed', { signal });
      return htmlBlocks(xml, /<entry\b[^>]*>([\s\S]*?)<\/entry>/gi).map((block: string, index: number) => ({
        title: firstMatch(block, /<title\b[^>]*>([\s\S]*?)<\/title>/i) ?? '',
        desc: stripHtml(firstMatch(block, /<content\b[^>]*>([\s\S]*?)<\/content>/i)),
        timestamp: firstMatch(block, /<updated\b[^>]*>([\s\S]*?)<\/updated>/i),
        rank: index + 1,
        url: firstMatch(block, /<link\b[^>]*rel="alternate"[^>]*href="([^"]+)"/i),
      }));
    },
  }),
];
