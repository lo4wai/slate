// 新建幀頁面 — 路由 /groups/:gid/contents/new
// 統一入口，支持圖片 + 所有動態類型在同一頁面切換。

import { useCallback } from 'react';
import { useNavigate } from 'react-router-dom';
import { ContentCreateEditor } from '@/features/contents/components/ContentCreateEditor';
import { RequireRouteParams } from '@/components/layout/RequireRouteParams';
import { appRoutes } from '@/app/routes';

export function ContentNewPage() {
  const navigate = useNavigate();

  return (
    <RequireRouteParams names={['gid'] as const} hint="請從總覽頁進入具體內容組。">
      {({ gid }) => <ContentNewPageContent gid={gid} navigate={(path) => navigate(path)} />}
    </RequireRouteParams>
  );
}

function ContentNewPageContent({
  gid,
  navigate,
}: {
  gid: string;
  navigate: (path: string) => void;
}) {
  const onDone = useCallback(() => {
    navigate(appRoutes.group(gid));
  }, [gid, navigate]);

  const onEditCreatedImage = useCallback(
    (contentId: string) => {
      navigate(appRoutes.editImageContent(gid, contentId));
    },
    [gid, navigate]
  );

  return <ContentCreateEditor gid={gid} onDone={onDone} onEditCreatedImage={onEditCreatedImage} />;
}
