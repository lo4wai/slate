// Mono Press 雙線分隔符 — 1px + 3px 間隔 + 2px。

import { cn } from '@/lib/cn';

interface DoubleRuleProps {
  className?: string;
}

export function DoubleRule({ className }: DoubleRuleProps) {
  return (
    <div className={cn('flex flex-col gap-[3px]', className)}>
      <div className="h-px bg-ink" />
      <div className="h-0.5 bg-ink" />
    </div>
  );
}
