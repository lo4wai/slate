// 登錄/註冊頁面佈局 — 左側 editorial 大字 + 右側表單。

import type { ReactNode } from 'react';
import { IconBlock } from '@/components/ui/IconBlock';

interface AuthLayoutProps {
  /** 頁面標題（如 "登錄"、"註冊"） */
  title: string;
  /** 副標題 */
  subtitle: string;
  /** 表單內容 */
  children: ReactNode;
}

export function AuthLayout({ title, subtitle, children }: AuthLayoutProps) {
  return (
    <div className="min-h-screen grid lg:grid-cols-[1.1fr_1fr] bg-paper">
      {/* 左側：editorial 大字（lg+ 顯示） */}
      <aside className="hidden lg:flex flex-col justify-between p-12 xl:p-16 border-r border-ink">
        <IconBlock size="xl" tone="brand" className="font-serif text-[28px] font-bold">
          墨
        </IconBlock>

        <div className="fade-up">
          <p className="font-sans text-[11px] text-stone uppercase tracking-[0.24em]">
            SLATE · 控制枱
          </p>
          <div className="h-px bg-ink mt-3.5 mb-7" />
          <h1 className="font-serif text-[100px] xl:text-[132px] font-black leading-[0.92] text-ink">
            {title}
          </h1>
          <p className="font-serif text-[20px] text-stone mt-7 max-w-md leading-relaxed">
            {subtitle}
          </p>
        </div>

        <span className="font-mono text-[11px] text-stone tracking-[0.06em]">
          400 × 300 · 1bpp · esp32-s3
        </span>
      </aside>

      {/* 右側：表單 */}
      <main className="flex items-start lg:items-center justify-center px-5 sm:px-8 pt-16 pb-32 lg:py-12">
        <div className="w-full max-w-sm fade-up fade-up-1">
          {/* 移動端 logo */}
          <div className="lg:hidden mb-10 text-center">
            <IconBlock size="xl" tone="brand" className="font-serif text-[28px] font-bold">
              墨
            </IconBlock>
            <h1 className="font-serif text-[36px] font-bold leading-none mt-3 tracking-tight">
              Slate
            </h1>
            <p className="font-sans text-[11px] text-stone mt-2 uppercase tracking-[0.2em]">
              案頭那塊墨水屏
            </p>
          </div>

          {children}
        </div>
      </main>
    </div>
  );
}
