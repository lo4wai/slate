// 圖片內容編輯頁面 — 將路由參數解析後傳入編輯器。

import { ImageContentEditor } from '@/features/contents/components/image-form/ImageContentEditor';
import { ContentEditorPageLayout } from '@/features/contents/components/ContentEditorPageLayout';

export function ImageContentEditorPage() {
  return (
    <ContentEditorPageLayout
      missingContentHint="請從內容列表進入圖片內容編輯頁。"
      notFoundTitle="內容不存在或已刪除"
      findContent={(content) => content.kind === 'image'}
      renderEditor={({ gid, content, onDone }) => (
        <ImageContentEditor gid={gid} content={content} onDone={onDone} />
      )}
    />
  );
}
