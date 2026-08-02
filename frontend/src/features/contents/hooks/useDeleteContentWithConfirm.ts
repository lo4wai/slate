import { useCallback } from 'react';
import type { ContentSummaryT } from 'shared';
import { useConfirmAction } from '@/hooks/useMutationAction';
import { useDeleteContent } from '@/features/contents/query/content-mutation-queries';

export function useDeleteContentWithConfirm({
  gid,
  content,
  description,
}: {
  gid: string;
  content: Pick<ContentSummaryT, 'id' | 'seq' | 'frame_name' | 'audio_etag' | 'kind'>;
  description?: string;
}) {
  const { mutate, isPending } = useDeleteContent(gid);

  const confirmDelete = useConfirmAction<string>({
    isPending,
    getConfirmOptions: useCallback(
      () => ({
        title: `刪除第 ${content.seq + 1} 項？`,
        description: description ?? defaultDeleteDescription(content),
        destructive: true,
        confirmText: '刪除',
      }),
      [content, description]
    ),
    run: useCallback((contentId, callbacks) => mutate(contentId, callbacks), [mutate]),
    successToast: '已刪除',
    errorToast: '刪除失敗',
  });
  const deleteWithConfirm = useCallback(() => {
    void confirmDelete(content.id);
  }, [confirmDelete, content.id]);

  return { deleteWithConfirm, isPending };
}

function defaultDeleteDescription(
  content: Pick<ContentSummaryT, 'frame_name' | 'audio_etag' | 'kind'>
): string {
  if (content.kind === 'dynamic') {
    return content.frame_name
      ? `「${content.frame_name}」動態內容會一併刪除，不可逆。`
      : '這項動態內容會刪除，不可逆。';
  }
  return content.frame_name
    ? `「${content.frame_name}」連同圖${content.audio_etag ? '與音頻' : ''}一起刪除，不可逆。`
    : `這項內容的圖${content.audio_etag ? '與音頻' : ''}會刪除，不可逆。`;
}
