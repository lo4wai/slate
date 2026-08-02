// 音頻播放預覽。server 給的是 raw PCM，播放控制由 usePcmAudioPlayer 封裝。

import { useState, useCallback, useEffect } from 'react';
import { Loader2, Pause, Play } from 'lucide-react';
import { useContentAudio } from '@/features/contents/query/content-audio-queries';
import { useToast } from '@/components/feedback/toast-context';
import { cn } from '@/lib/cn';
import { usePcmAudioPlayer } from './usePcmAudioPlayer';

interface AudioPlayPreviewProps {
  contentId: string;
  /** 沒 etag 表示無音頻,組件返 null */
  etag: string | null;
  className?: string;
  /** 標籤：顯示在按鈕旁（可選，如「試聽」） */
  label?: string;
}

export function AudioPlayPreview({ contentId, etag, className, label }: AudioPlayPreviewProps) {
  const [requested, setRequested] = useState(false);
  const [playAfterLoad, setPlayAfterLoad] = useState(false);
  const audio = useContentAudio(contentId, etag, requested);
  const toast = useToast();
  const onPlaybackError = useCallback(
    (message: string, hint?: string) => toast.error(message, hint),
    [toast]
  );
  const { playing, ensureContext, play, stop } = usePcmAudioPlayer({
    onError: onPlaybackError,
  });

  useEffect(() => {
    if (!playAfterLoad || !audio.data) return;
    setPlayAfterLoad(false);
    void play(audio.data);
  }, [audio.data, play, playAfterLoad]);

  useEffect(() => {
    if (audio.error) setPlayAfterLoad(false);
  }, [audio.error]);

  useEffect(() => {
    stop();
    setRequested(false);
    setPlayAfterLoad(false);
  }, [contentId, etag, stop]);

  if (!etag) return null;

  const loading = requested && audio.isFetching;
  const title = audio.error ? '音頻加載失敗' : loading ? '加載中' : playing ? '停止' : '試聽';

  return (
    <button
      type="button"
      onClick={(e) => {
        e.stopPropagation();
        e.preventDefault();
        if (loading || audio.error) return;
        if (playing) {
          setPlayAfterLoad(false);
          stop();
          return;
        }
        if (audio.data) {
          void play(audio.data);
        } else {
          setRequested(true);
          setPlayAfterLoad(true);
          void ensureContext()
            .then((ctx) => {
              if (ctx) return;
              setPlayAfterLoad(false);
              toast.error('音頻播放失敗', '當前環境不支持 WebAudio。');
            })
            .catch((err) => {
              setPlayAfterLoad(false);
              toast.error('音頻播放失敗', err instanceof Error ? err.message : undefined);
            });
        }
      }}
      disabled={loading || !!audio.error}
      title={title}
      aria-label={title}
      className={cn(
        'inline-flex items-center gap-1.5 px-2 py-1.5 text-stone hover:text-ink hover:bg-cream transition-colors disabled:opacity-50',
        className
      )}
    >
      {loading ? (
        <Loader2 size={14} className="animate-spin" />
      ) : playing ? (
        <Pause size={14} />
      ) : (
        <Play size={14} />
      )}
      {label && <span className="font-sans text-[12px]">{label}</span>}
    </button>
  );
}
