// Mono Press icon 容器：0px 圓角、墨線邊框。
//
// 尺寸階梯：
//   sm  w-8  h-8    內嵌 / 列表小圖標
//   md  w-10 h-10   卡片 / section badge / dialog header
//   lg  w-14 h-14   group 大標題
//   xl  w-16 h-16   login/register 品牌塊
//
// 語調：
//   brand  bg-ink text-paper border-ink   logo 唯一
//   soft   bg-paper text-ink border-ink   默認（卡片/header/dialog）
//   danger bg-paper text-clay border-clay 銷燬性/警告
//   muted  bg-cream text-stone border-line 空狀態/弱化
//   avatar bg-ink text-paper border-ink    用户頭像

import type { ReactNode } from 'react';
import { cn } from '@/lib/cn';

type Size = 'sm' | 'md' | 'lg' | 'xl';
type Tone = 'brand' | 'avatar' | 'soft' | 'danger' | 'muted';

const SIZE: Record<Size, string> = {
  sm: 'w-8 h-8',
  md: 'w-10 h-10',
  lg: 'w-14 h-14',
  xl: 'w-16 h-16',
};

const TONE: Record<Tone, string> = {
  brand: 'bg-ink text-paper border border-ink',
  avatar: 'bg-ink text-paper border border-ink !rounded-full',
  soft: 'bg-paper text-ink border border-ink',
  danger: 'bg-paper text-clay border border-clay',
  muted: 'bg-cream text-stone border border-line',
};

interface IconBlockProps {
  size?: Size;
  tone?: Tone;
  children: ReactNode;
  className?: string;
  title?: string;
  'aria-label'?: string;
}

export function IconBlock({
  size = 'md',
  tone = 'soft',
  children,
  className,
  title,
  'aria-label': ariaLabel,
}: IconBlockProps) {
  return (
    <span
      title={title}
      aria-label={ariaLabel}
      className={cn(
        'inline-flex items-center justify-center flex-shrink-0',
        SIZE[size],
        TONE[tone],
        className
      )}
    >
      {children}
    </span>
  );
}
