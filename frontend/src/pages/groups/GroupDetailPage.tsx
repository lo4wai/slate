// 組詳情：metadata + 內容網格（拖拽排序 + 試聽/編輯/刪除）+ 頂部「新增」。
//
// dnd-kit reorder 通過 useDndOrder 複用；本地順序會在保存失敗時回滾。

import { Link, useNavigate } from 'react-router-dom';
import { ArrowLeft, Plus, Layers } from 'lucide-react';
import { useGroup, useUpdateGroup } from '@/features/groups/query/group-queries';
import { useGroupContents } from '@/features/contents/query/content-read-queries';
import { useReorderContents } from '@/features/contents/query/content-mutation-queries';
import type { ContentDetailT, GroupSummaryT } from 'shared';
import { Spinner } from '@/components/ui/Spinner';
import { Button } from '@/components/ui/Button';
import { EmptyState } from '@/components/ui/EmptyState';
import { SortableGrid } from '@/components/dnd/SortableGrid';
import { ContentCard } from '@/features/contents/components/cards/ContentCard';
import { PageHeader } from '@/components/layout/PageHeader';
import { RequireRouteParams } from '@/components/layout/RequireRouteParams';
import { InlineRename } from '@/components/ui/InlineRename';
import { useInlineRename } from '@/hooks/useInlineRename';
import { useToast } from '@/components/feedback/toast-context';
import { getApiErrorMessage } from '@/lib/api-errors';
import { formatBytes } from '@/lib/format';
import { useDndOrder } from '@/components/dnd/useDndOrder';
import { appRoutes } from '@/app/routes';

export function GroupDetailPage() {
  const navigate = useNavigate();

  return (
    <RequireRouteParams
      names={['gid'] as const}
      hint="請從總覽頁進入具體內容組。"
      action={<BackHomeLink />}
    >
      {({ gid }) => <GroupDetailContent gid={gid} navigate={navigate} />}
    </RequireRouteParams>
  );
}

function GroupDetailContent({
  gid,
  navigate,
}: {
  gid: string;
  navigate: ReturnType<typeof useNavigate>;
}) {
  const groupQuery = useGroup(gid);
  const contents = useGroupContents(gid);
  const reorder = useReorderContents(gid);
  const toast = useToast();

  const group = groupQuery.data;
  const { sensors, currentOrder, orderedItems, onDragEnd } = useDndOrder(
    contents.data,
    getContentId,
    (newOrder, { commit, rollback }) =>
      reorder.mutate(
        { order: newOrder },
        {
          onSuccess: commit,
          onError: (err) => {
            rollback();
            toast.error('排序保存失敗', getApiErrorMessage(err));
          },
        }
      )
  );

  const openCreate = () => navigate(appRoutes.newContent(gid));
  const openEdit = (content: ContentDetailT) => {
    if (content.kind === 'dynamic') {
      navigate(appRoutes.editDynamicContent(gid, content.id));
    } else {
      navigate(appRoutes.editImageContent(gid, content.id));
    }
  };
  const goBack = () => {
    navigate(appRoutes.home);
  };

  if (groupQuery.isPending) {
    return (
      <div className="pt-16 text-center">
        <Spinner label="加載中" />
      </div>
    );
  }
  if (groupQuery.isError || !group) {
    return (
      <EmptyState
        title="內容不存在或已被刪除"
        action={
          <Link
            to={appRoutes.home}
            className="inline-flex items-center gap-1 text-[13px] text-stone border-b border-stone"
          >
            <ArrowLeft size={13} /> 返回總覽
          </Link>
        }
      />
    );
  }

  return (
    <div>
      <GroupHeader group={group} onBack={goBack} onAdd={openCreate} />
      <div className="mt-6 fade-up fade-up-1">
        {contents.isPending ? (
          <Spinner label="加載中" />
        ) : contents.isError ? (
          <EmptyState title="加載失敗" hint="請刷新重試。" />
        ) : contents.data && contents.data.length > 0 ? (
          <SortableGrid
            sensors={sensors}
            order={currentOrder}
            items={orderedItems}
            onDragEnd={onDragEnd}
            getKey={(content) => content.id}
            className="grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-4"
            renderItem={(content) => <ContentCard gid={gid} content={content} onEdit={openEdit} />}
          />
        ) : (
          <EmptyState
            title="尚無內容"
            hint="點擊新建幀開始添加內容。"
            action={
              <Button onClick={openCreate} iconLeft={<Plus size={16} />}>
                新建幀
              </Button>
            }
          />
        )}
      </div>
    </div>
  );
}

function getContentId(content: ContentDetailT): string {
  return content.id;
}

function BackHomeLink() {
  return (
    <Link
      to={appRoutes.home}
      className="inline-flex items-center gap-1 text-[13px] text-stone border-b border-stone"
    >
      <ArrowLeft size={13} /> 返回總覽
    </Link>
  );
}

// ───── 組標題 + inline 改名 + 新建內容 ───────────────────────────
function GroupHeader({
  group,
  onBack,
  onAdd,
}: {
  group: GroupSummaryT;
  onBack: () => void;
  onAdd: () => void;
}) {
  const update = useUpdateGroup(group.id);
  const toast = useToast();

  const { editing, draft, setDraft, startEditing, commit, handleKeyDown } = useInlineRename(
    group.name,
    async (name) => {
      try {
        await update.mutateAsync({ name });
        toast.success('已改名');
      } catch (err) {
        toast.error('改名失敗', getApiErrorMessage(err));
        throw err;
      }
    }
  );

  return (
    <PageHeader
      backLabel="總覽"
      onBack={onBack}
      icon={<Layers size={24} />}
      title={group.name}
      titleContent={
        <InlineRename
          editing={editing}
          value={group.name}
          draft={draft}
          onDraftChange={setDraft}
          onStart={startEditing}
          onCommit={commit}
          onKeyDown={handleKeyDown}
          pending={update.isPending}
          titleClassName="font-serif text-[32px] sm:text-[40px] font-bold leading-[1.2] truncate tracking-tight"
          inputClassName="!font-serif !font-bold !text-[32px] sm:!text-[40px] !leading-[1.2]"
          buttonClassName="p-2 -m-1"
        />
      }
      subtitle={`${group.content_count} 項 · ${formatBytes(group.total_bytes)}`}
      action={
        <Button iconLeft={<Plus size={16} />} size="sm" onClick={onAdd}>
          新建幀
        </Button>
      }
    />
  );
}
