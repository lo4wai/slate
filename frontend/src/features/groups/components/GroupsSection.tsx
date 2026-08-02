import { memo, useCallback, useState } from 'react';
import { FolderHeart, Plus } from 'lucide-react';
import type { GroupSummaryT } from 'shared';
import {
  useCreateGroup,
  useDeleteGroup,
  useReorderGroups,
} from '@/features/groups/query/group-queries';
import { CreateGroupDialog } from '@/features/groups/components/CreateGroupDialog';
import { GroupCardSortable } from '@/features/groups/components/GroupCard';
import { useConfirmAction } from '@/hooks/useMutationAction';
import { useToast } from '@/components/feedback/toast-context';
import { Section } from '@/components/layout/Section';
import { Button } from '@/components/ui/Button';
import { EmptyState } from '@/components/ui/EmptyState';
import { Spinner } from '@/components/ui/Spinner';
import { SortableGrid } from '@/components/dnd/SortableGrid';
import { getApiErrorMessage } from '@/lib/api-errors';
import { useDndOrder } from '@/components/dnd/useDndOrder';

interface GroupsSectionProps {
  groups: GroupSummaryT[] | undefined;
  isPending: boolean;
}

export const GroupsSection = memo(function GroupsSection({
  groups,
  isPending,
}: GroupsSectionProps) {
  const create = useCreateGroup();
  const { mutate: deleteGroupMutate, isPending: deletePending } = useDeleteGroup();
  const reorder = useReorderGroups();
  const toast = useToast();
  const deleteGroup = useConfirmAction<GroupSummaryT>({
    isPending: deletePending,
    getConfirmOptions: useCallback(
      (group) => ({
        title: `刪除「${group.name}」？`,
        description: `這一組連同 ${group.content_count} 項內容的圖片與音頻會全部刪除，不可逆。`,
        destructive: true,
        confirmText: '刪除整組',
      }),
      []
    ),
    run: useCallback(
      (group, callbacks) => deleteGroupMutate(group.id, callbacks),
      [deleteGroupMutate]
    ),
    successToast: '已刪除',
    errorToast: '刪除失敗',
  });
  const renderGroup = useCallback(
    (group: GroupSummaryT) => (
      <GroupCardSortable group={group} deleteDisabled={deletePending} onDelete={deleteGroup} />
    ),
    [deleteGroup, deletePending]
  );

  const [createOpen, setCreateOpen] = useState(false);

  const { sensors, currentOrder, orderedItems, onDragEnd } = useDndOrder(
    groups,
    useCallback((g) => g.id, []),
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

  return (
    <Section
      title="內容"
      badge={<FolderHeart size={18} />}
      subtitle="支持圖片和音頻，音頻會隨圖片同步播放"
      action={
        <Button onClick={() => setCreateOpen(true)} iconLeft={<Plus size={16} />} size="sm">
          新建組
        </Button>
      }
    >
      {isPending ? (
        <div className="flex justify-center py-8">
          <Spinner label="加載中" />
        </div>
      ) : groups && groups.length > 0 ? (
        <SortableGrid
          sensors={sensors}
          order={currentOrder}
          items={orderedItems}
          onDragEnd={onDragEnd}
          getKey={(group) => group.id}
          className="grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4 fade-up fade-up-2"
          renderItem={renderGroup}
        />
      ) : (
        <EmptyState
          icon={<FolderHeart size={26} />}
          title="尚無內容"
          hint="新建組開始上傳圖片。設備會按順序循環顯示。"
          action={
            <Button onClick={() => setCreateOpen(true)} iconLeft={<Plus size={16} />}>
              新建第一組
            </Button>
          }
        />
      )}

      <CreateGroupDialog
        open={createOpen}
        onOpenChange={setCreateOpen}
        onCreate={async (name) => {
          try {
            await create.mutateAsync({ name });
            toast.success('已創建');
            setCreateOpen(false);
          } catch (err) {
            toast.error('創建失敗', getApiErrorMessage(err));
          }
        }}
        isPending={create.isPending}
      />
    </Section>
  );
});
