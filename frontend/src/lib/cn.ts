// className 合併工具 — 使用 tailwind-merge 處理 Tailwind class 衝突。
import { twMerge } from 'tailwind-merge';

export function cn(...parts: Array<string | false | null | undefined>): string {
  return twMerge(parts.filter(Boolean).join(' '));
}
