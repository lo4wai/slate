// 抖動算法選擇 + 閾值滑塊(僅 threshold 模式顯示)+ 縮放滑塊(僅選了圖時顯示)。

import { DITHER_MODES, DITHER_INFO } from 'shared';
import type { DitherMode } from 'shared';
import { cn } from '@/lib/cn';

interface DitherControlsProps {
  mode: DitherMode;
  onModeChange: (m: DitherMode) => void;
  threshold: number;
  onThresholdChange: (n: number) => void;
  disabled?: boolean;
  /** 只有選了圖才顯示 scale 滑塊 */
  hasImage: boolean;
  scale: number;
  onScaleChange: (n: number) => void;
  onResetCrop: () => void;
}

export function DitherControls({
  mode,
  onModeChange,
  threshold,
  onThresholdChange,
  disabled = false,
  hasImage,
  scale,
  onScaleChange,
  onResetCrop,
}: DitherControlsProps) {
  return (
    <div className="space-y-5">
      {hasImage && (
        <div>
          <div className="flex items-baseline justify-between mb-2 ml-0.5">
            <p className="font-sans text-[13px] text-stone">縮放</p>
            <div className="flex items-center gap-2">
              <p className="font-mono text-[12px] text-ink tabular-nums">{scale.toFixed(1)}×</p>
              <button
                type="button"
                onClick={onResetCrop}
                className="font-sans text-[11px] text-stone border-b border-stone hover:border-ink hover:text-ink transition-colors"
              >
                重置
              </button>
            </div>
          </div>
          <input
            type="range"
            min="0.5"
            max="3"
            step="0.1"
            value={scale}
            onChange={(e) => onScaleChange(Number(e.target.value))}
          />
          <p className="font-serif italic text-[11px] text-stone-light mt-1.5">
            預覽圖可拖拽定位,滑塊控制縮放。
          </p>
        </div>
      )}

      <div>
        <div className="flex items-baseline justify-between mb-2 ml-0.5">
          <p className="font-sans text-[13px] text-stone">抖動算法</p>
          <p className="font-mono text-[11px] text-stone-light">{DITHER_INFO[mode].hint}</p>
        </div>
        <div className="grid grid-cols-2">
          {DITHER_MODES.map((m) => (
            <button
              key={m}
              type="button"
              aria-pressed={mode === m}
              disabled={disabled}
              onClick={() => onModeChange(m)}
              className={cn(
                'flex items-center justify-between px-3 py-2.5 font-serif text-[13px] border border-ink -ml-px -mt-px transition-colors',
                mode === m ? 'bg-cream-deep text-ink' : 'text-stone hover:bg-cream',
                disabled && 'cursor-not-allowed opacity-50 hover:bg-transparent'
              )}
            >
              <span>{DITHER_INFO[m].label}</span>
              {mode === m && <span className="font-mono text-[10px]">●</span>}
            </button>
          ))}
        </div>
        <p className="font-serif text-[11px] text-stone-light mt-1.5">
          線稿用「線稿 · 純黑白」;照片用「照片 · 推薦」。
        </p>
      </div>

      {mode === 'threshold' && (
        <div>
          <div className="flex items-baseline justify-between mb-2 ml-0.5">
            <p className="font-sans text-[13px] text-stone">閾值</p>
            <p className="font-mono text-[12px] text-ink tabular-nums">
              {threshold}
              <span className="text-stone-light">/255</span>
            </p>
          </div>
          <input
            type="range"
            min="0"
            max="255"
            value={threshold}
            disabled={disabled}
            onChange={(e) => onThresholdChange(Number(e.target.value))}
          />
          <p className="font-serif italic text-[11px] text-stone-light mt-1.5">
            {disabled ? '換圖後可調整抖動參數。' : '簡筆畫 128;帶細灰邊的圖試 180+。'}
          </p>
        </div>
      )}
    </div>
  );
}
