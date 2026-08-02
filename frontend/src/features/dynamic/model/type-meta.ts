import {
  Activity,
  BarChart3,
  Bell,
  BookText,
  Calendar,
  CalendarDays,
  CloudSun,
  Flame,
  Type as TypeIcon,
  type LucideIcon,
} from 'lucide-react';
import type { DynamicTypeT } from 'shared';

export interface DynamicTypeMeta {
  label: string;
  hint: string;
  description: string;
  hasConfigurableParams: boolean;
  supportsAudio: boolean;
  Icon: LucideIcon;
}

export const DYNAMIC_TYPE_META = {
  daily_calendar: {
    label: '日曆',
    hint: '日期 · 星期 · 農曆 · 節氣',
    description: '顯示今日公曆、農曆與干支。',
    hasConfigurableParams: false,
    supportsAudio: true,
    Icon: Calendar,
  },
  month_calendar: {
    label: '月曆',
    hint: '整月日期 · 農曆 · 節日',
    description: '顯示當月日曆。',
    hasConfigurableParams: false,
    supportsAudio: true,
    Icon: CalendarDays,
  },
  weather: {
    label: '天氣',
    hint: '實時氣温 / 濕度 / 風速',
    description: '按城市顯示實時天氣。數據來自 QWeather。',
    hasConfigurableParams: true,
    supportsAudio: true,
    Icon: CloudSun,
  },
  history_today: {
    label: '歷史上的今天',
    hint: '今日曆史大事，每日 0 點更新',
    description: '自動顯示今日歷史事件，可選維基百科或百度百科。',
    hasConfigurableParams: true,
    supportsAudio: true,
    Icon: BookText,
  },
  weather_alert: {
    label: '氣象預警',
    hint: '中央氣象台 · 全國預警',
    description: '顯示中央氣象台全國或指定省份氣象預警。',
    hasConfigurableParams: true,
    supportsAudio: true,
    Icon: Bell,
  },
  earthquake_report: {
    label: '地震速報',
    hint: '中國地震台網 · 最新速報',
    description: '顯示中國地震台網最新地震速報。',
    hasConfigurableParams: true,
    supportsAudio: true,
    Icon: Activity,
  },
  dashboard: {
    label: '外部數據',
    hint: '模板 + JSON 數據推送',
    description: '選擇系統模板或自定義模板，後續只推送數據即可刷新畫面。',
    hasConfigurableParams: true,
    supportsAudio: false,
    Icon: BarChart3,
  },
  font_test: {
    label: '字體測試',
    hint: '切換字體 · 查看 1bpp 字形',
    description: '測試 Fusion Pixel 字體在墨水屏上的渲染。',
    hasConfigurableParams: true,
    supportsAudio: false,
    Icon: TypeIcon,
  },
  hot_list: {
    label: '熱榜',
    hint: '微博 / 知乎 / B站等榜單',
    description: '選擇站點熱榜，自動刷新並以墨水屏列表展示。',
    hasConfigurableParams: true,
    supportsAudio: false,
    Icon: Flame,
  },
} satisfies Record<DynamicTypeT, DynamicTypeMeta>;

export const DYNAMIC_TYPE_ORDER = [
  'daily_calendar',
  'month_calendar',
  'history_today',
  'weather',
  'weather_alert',
  'earthquake_report',
  'hot_list',
  'dashboard',
  'font_test',
] as const satisfies readonly DynamicTypeT[];
