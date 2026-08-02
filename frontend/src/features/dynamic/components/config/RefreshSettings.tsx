import { Select, SelectItem } from '@/components/ui/Select';
import type {
  DynamicConfigChange,
  RefreshableDynamicConfig,
} from '@/features/dynamic/model/config-types';

const DEFAULT_REFRESH_INTERVAL_SEC = 600;

export function DynamicRefreshSettings({
  config,
  onChange,
}: {
  config: RefreshableDynamicConfig;
  onChange: DynamicConfigChange;
}) {
  const current = config.refresh_interval_sec ?? DEFAULT_REFRESH_INTERVAL_SEC;
  return (
    <div>
      <p className="font-mono text-[10px] text-stone uppercase tracking-[0.18em] mb-1.5">
        刷新間隔
      </p>
      <Select
        value={String(current)}
        onValueChange={(value) => onChange({ ...config, refresh_interval_sec: Number(value) })}
      >
        {refreshOptions(config.type).map((item) => (
          <SelectItem key={item.value} value={String(item.value)} hint={item.hint}>
            {item.label}
          </SelectItem>
        ))}
      </Select>
    </div>
  );
}

function refreshOptions(type?: string): Array<{
  value: number;
  label: string;
  hint: string;
}> {
  return [
    ...(type === 'dashboard' ? [{ value: 60, label: '1 分鐘', hint: '高頻' }] : []),
    { value: 300, label: '5 分鐘', hint: '更實時' },
    { value: 600, label: '10 分鐘', hint: '推薦' },
    { value: 1800, label: '30 分鐘', hint: '省電' },
    { value: 3600, label: '1 小時', hint: '低頻' },
  ];
}
