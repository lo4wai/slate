import { Spinner } from '@/components/ui/Spinner';
import { FrameBitmapPreview } from '@/components/eink/FrameBitmapPreview';

export function DynamicFramePreview({
  data,
  pending,
  hasConfig,
  caption,
}: {
  data: ArrayBuffer | null;
  pending: boolean;
  hasConfig: boolean;
  caption?: string | null;
}) {
  const showPlaceholder = !data;
  return (
    <div className="frame-preview-surface">
      <FrameBitmapPreview data={data} caption={caption} />
      {showPlaceholder && !pending && (
        <div className="absolute inset-0 z-20 flex items-center justify-center pointer-events-none">
          <span className="font-serif italic text-[13px] text-stone-light">
            {hasConfig ? '修改參數後自動更新' : '選擇類型後開始配置'}
          </span>
        </div>
      )}
      {pending && (
        <div className="absolute inset-0 z-20 flex items-center justify-center">
          <Spinner />
        </div>
      )}
    </div>
  );
}

export function SavedOrLiveDynamicFramePreview({
  savedData,
  savedPending,
  liveData,
  livePending,
  hasConfig,
  caption,
}: {
  savedData?: ArrayBuffer;
  savedPending?: boolean;
  liveData: ArrayBuffer | null;
  livePending: boolean;
  hasConfig: boolean;
  caption?: string | null;
}) {
  const displayData = liveData ?? savedData ?? null;
  const pending = livePending || (!liveData && Boolean(savedPending));

  return (
    <DynamicFramePreview
      data={displayData}
      pending={pending}
      hasConfig={hasConfig}
      caption={caption}
    />
  );
}
