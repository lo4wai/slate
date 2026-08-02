// 動態內容編輯頁 —— create + edit 共用。
//
// 路由：
//   /groups/:gid/contents/dynamic/:contentId/edit — 編輯

import { DynamicContentEditor } from '@/features/dynamic/components/DynamicContentEditor';
import { EmptyState } from '@/components/ui/EmptyState';
import { Button } from '@/components/ui/Button';
import { ContentEditorPageLayout } from '@/features/contents/components/ContentEditorPageLayout';

export function DynamicContentEditorPage() {
  return (
    <ContentEditorPageLayout
      missingContentHint="請從內容列表進入動態內容編輯頁。"
      notFoundTitle="動態內容不存在或已刪除"
      findContent={(content) => content.kind === 'dynamic'}
      renderEditor={({ gid, content, onDone }) => {
        if (!content.dynamic_type || !content.dynamic_config) {
          return (
            <EmptyState
              title="動態內容配置缺失"
              hint="這條動態內容的數據不完整，請返回內容列表後重試。"
              action={
                <Button variant="outline" size="sm" onClick={onDone}>
                  返回
                </Button>
              }
            />
          );
        }

        // ContentDetail 已經帶 dynamic_type / dynamic_config，省一次 GET /contents/:id 請求。
        return (
          <DynamicContentEditor
            gid={gid}
            content={content}
            initialType={content.dynamic_type}
            initialConfig={content.dynamic_config}
            onDone={onDone}
          />
        );
      }}
    />
  );
}
