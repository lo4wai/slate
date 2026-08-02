// 空狀態:楷書提示 + 副標 + 可選 action。
// icon 走 IconBlock(size lg / tone muted),與 Section badge / 卡片 icon 同體系。

import type { ReactNode } from 'react';
import { IconBlock } from '@/components/ui/IconBlock';
import { cn } from '@/lib/cn';

interface EmptyStateProps {
  title: string;
  hint?: string;
  /** lucide 圖標(顯示在標題上方) */
  icon?: ReactNode;
  action?: ReactNode;
  className?: string;
}

export function EmptyState({ title, hint, icon, action, className }: EmptyStateProps) {
  return (
    <div className={cn('py-16 text-center', className)}>
      {icon && (
        <div className="inline-flex mb-4">
          <IconBlock size="lg" tone="muted">
            {icon}
          </IconBlock>
        </div>
      )}
      <p className="font-serif text-[18px] font-medium text-ink">{title}</p>
      {hint && (
        <p className="text-[14px] text-stone mt-2 max-w-md mx-auto leading-relaxed">{hint}</p>
      )}
      {action && <div className="mt-6 flex justify-center">{action}</div>}
    </div>
  );
}
