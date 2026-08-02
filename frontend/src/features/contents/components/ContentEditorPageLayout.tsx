import { useCallback, type ReactNode } from 'react';
import { useNavigate, useParams } from 'react-router-dom';
import type { ContentDetailT } from 'shared';
import { useContentDetail } from '@/features/contents/query/content-read-queries';
import { EmptyState } from '@/components/ui/EmptyState';
import { RequireRouteParams } from '@/components/layout/RequireRouteParams';
import { Spinner } from '@/components/ui/Spinner';
import { appRoutes } from '@/app/routes';

interface ContentEditorPageLayoutProps {
  missingContentHint: string;
  notFoundTitle: string;
  findContent: (content: ContentDetailT) => boolean;
  renderEditor: (props: { gid: string; content: ContentDetailT; onDone: () => void }) => ReactNode;
}

export function ContentEditorPageLayout({
  missingContentHint,
  notFoundTitle,
  findContent,
  renderEditor,
}: ContentEditorPageLayoutProps) {
  return (
    <RequireRouteParams names={['gid'] as const} hint="請從總覽頁進入具體內容組。">
      {({ gid }) => (
        <ContentEditorPageContent
          gid={gid}
          missingContentHint={missingContentHint}
          notFoundTitle={notFoundTitle}
          findContent={findContent}
          renderEditor={renderEditor}
        />
      )}
    </RequireRouteParams>
  );
}

function ContentEditorPageContent({
  gid,
  missingContentHint,
  notFoundTitle,
  findContent,
  renderEditor,
}: ContentEditorPageLayoutProps & { gid: string }) {
  const { contentId } = useParams();
  const navigate = useNavigate();
  const content = useContentDetail(contentId);

  const onDone = useCallback(() => {
    navigate(appRoutes.group(gid));
  }, [gid, navigate]);

  if (!contentId) {
    return <EmptyState title="頁面不存在" hint={missingContentHint} />;
  }

  if (content.isPending) {
    return (
      <div className="pt-16 text-center">
        <Spinner label="加載中" />
      </div>
    );
  }

  if (content.isError) {
    return <EmptyState title="加載失敗" hint="請刷新重試。" />;
  }

  const isExpectedContent = content.data ? findContent(content.data) : false;

  if (content.data && content.data.id !== contentId) {
    return <EmptyState title="加載失敗" hint="內容詳情返回了不匹配的 ID，請刷新重試。" />;
  }

  if (!content.data || !isExpectedContent || content.data.group_id !== gid) {
    if (content.isFetching) {
      return (
        <div className="pt-16 text-center">
          <Spinner label="加載中" />
        </div>
      );
    }
    return <EmptyState title={notFoundTitle} />;
  }

  return <>{renderEditor({ gid, content: content.data, onDone })}</>;
}
