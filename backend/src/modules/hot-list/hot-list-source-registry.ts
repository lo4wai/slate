import type { HotListSource } from './hot-list.types';
import { githubTrendingSource } from './sources/github-trending';
import { DAILY_HOT_DIRECT_SOURCES } from './sources/daily-hot-direct';
import { NEWSNOW_DIRECT_SOURCES } from './sources/newsnow-direct';

export const HOT_LIST_SOURCE_REGISTRY = [
  githubTrendingSource,
  ...DAILY_HOT_DIRECT_SOURCES,
  ...NEWSNOW_DIRECT_SOURCES,
] as const satisfies readonly HotListSource[];
