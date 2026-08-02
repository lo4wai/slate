// 統一的 Select 組件 — Radix Select 包一層，trigger / content / item
// 與 Input 使用同款 affordance。
//
// 單值受控用法:
//   <Select value={v} onValueChange={setV} placeholder="未選">
//     <SelectItem value="a">A</SelectItem>
//     <SelectItem value="b" hint="2 項">B</SelectItem>
//   </Select>
//
// 需要分隔條:<SelectSeparator />
//
// 這個組件刻意不做「options[]」那種 declarative 數據驅動 — 因為 GroupSelector
// 的「未選組」item 與正常 group items 之間需要一根分隔線，而 DitherControls
// 不需要。children 形式讓兩者複用 trigger 但保留各自結構自由度。

import { type ReactNode } from 'react';
import * as RS from '@radix-ui/react-select';
import { ChevronDown, Check } from 'lucide-react';
import { cn } from '@/lib/cn';

interface SelectProps {
  value: string;
  onValueChange: (v: string) => void;
  placeholder?: string;
  disabled?: boolean;
  className?: string;
  'aria-label'?: string;
  children: ReactNode;
}

export function Select({
  value,
  onValueChange,
  placeholder,
  disabled,
  className,
  'aria-label': ariaLabel,
  children,
}: SelectProps) {
  return (
    <RS.Root value={value} onValueChange={onValueChange} disabled={disabled}>
      <RS.Trigger aria-label={ariaLabel} className={cn(selectTriggerCls, className)}>
        <RS.Value placeholder={placeholder} />
        <RS.Icon>
          {/* chevron 用 stone(暖棕)而非 stone-light:在 cream-deep 底上後者
              幾乎被吃掉,失去"這是下拉框"的視覺暗示 */}
          <ChevronDown size={14} className="text-ink" />
        </RS.Icon>
      </RS.Trigger>
      <RS.Portal>
        <RS.Content position="popper" sideOffset={6} className={selectContentCls}>
          <RS.ScrollUpButton className="flex h-5 items-center justify-center bg-paper text-stone outline-none focus-visible:!outline-none">
            <ChevronDown size={13} className="rotate-180" />
          </RS.ScrollUpButton>
          <RS.Viewport className={selectViewportCls}>{children}</RS.Viewport>
          <RS.ScrollDownButton className="flex h-5 items-center justify-center bg-paper text-stone outline-none focus-visible:!outline-none">
            <ChevronDown size={13} />
          </RS.ScrollDownButton>
        </RS.Content>
      </RS.Portal>
    </RS.Root>
  );
}

interface SelectItemProps {
  value: string;
  children: ReactNode;
  /** 右側灰字小注(如 "12 幀") */
  hint?: ReactNode;
  className?: string;
}

export function SelectItem({ value, children, hint, className }: SelectItemProps) {
  return (
    <RS.Item value={value} className={cn(selectItemCls, className)}>
      <RS.ItemText>
        <span className="block truncate">{children}</span>
      </RS.ItemText>
      {hint != null && (
        <span className="ml-auto shrink-0 font-mono text-[11px] text-stone-light">{hint}</span>
      )}
      {/* 選中標記:右側 check,與 hint 共存時排在 hint 之後 */}
      <RS.ItemIndicator className="ml-1 shrink-0 text-ink">
        <Check size={13} strokeWidth={2.5} />
      </RS.ItemIndicator>
    </RS.Item>
  );
}

export function SelectSeparator() {
  return <RS.Separator className="h-px bg-line my-1 mx-3" />;
}

const selectTriggerCls = cn(
  'w-full inline-flex items-center justify-between gap-2',
  'px-4 py-3',
  'font-serif text-[16px]',
  'border border-ink bg-cream/30 text-ink',
  'transition-colors duration-150',
  'focus-visible:!outline-none focus-visible:bg-cream/60',
  'hover:bg-cream/50',
  'disabled:opacity-40 disabled:cursor-not-allowed'
);

const selectContentCls = cn(
  'w-[var(--radix-select-trigger-width)] max-w-[calc(100vw-2rem)]',
  'bg-paper border border-ink',
  'shadow-dropdown',
  'py-1 z-[60] overflow-hidden',
  'outline-none focus-visible:!outline-none'
);

const selectViewportCls = cn(
  'max-h-[min(22rem,var(--radix-select-content-available-height))]',
  'overflow-y-auto',
  'outline-none focus-visible:!outline-none'
);

const selectItemCls = cn(
  'flex min-w-0 items-center gap-2 mx-1 px-3 py-2 text-[14px]',
  'cursor-pointer outline-none focus-visible:!outline-none',
  'hover:bg-cream',
  'data-[highlighted]:bg-cream',
  'data-[state=checked]:text-ink data-[state=checked]:font-medium data-[state=checked]:bg-cream-deep'
);
