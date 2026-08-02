// 新建組對話框—輸入組名後回調 onCreate。

import { useEffect, useState } from 'react';
import { FolderHeart, ArrowRight } from 'lucide-react';
import * as Dialog from '@radix-ui/react-dialog';
import { Input } from '@/components/ui/Input';
import { Button } from '@/components/ui/Button';
import { Spinner } from '@/components/ui/Spinner';
import { DialogHeader } from '@/components/ui/DialogHeader';
import { dialogContentCls, dialogOverlayCls } from '@/components/ui/styles/dialog';

export function CreateGroupDialog({
  open,
  onOpenChange,
  onCreate,
  isPending,
}: {
  open: boolean;
  onOpenChange: (o: boolean) => void;
  onCreate: (name: string) => Promise<void> | void;
  isPending: boolean;
}) {
  const [name, setName] = useState('');

  useEffect(() => {
    if (!open) {
      setName('');
    }
  }, [open]);

  return (
    <Dialog.Root open={open} onOpenChange={onOpenChange}>
      <Dialog.Portal>
        <Dialog.Overlay className={dialogOverlayCls} />
        <Dialog.Content className={dialogContentCls}>
          <DialogHeader
            icon={<FolderHeart size={24} />}
            title="新建組"
            description="創建一個新的內容組來管理幀序列。"
            className="mb-6"
          />

          <div className="space-y-5">
            <Input
              label="名稱"
              value={name}
              onChange={(e) => setName(e.target.value)}
              onKeyDown={(e) => {
                if (e.key === 'Enter' && name.trim() && !isPending) onCreate(name.trim());
              }}
              autoFocus
              placeholder="如：每日卡片"
            />
          </div>

          <div className="mt-7 flex items-center justify-end gap-3">
            <Dialog.Close asChild>
              <Button variant="outline">取消</Button>
            </Dialog.Close>
            <Button
              onClick={() => name.trim() && onCreate(name.trim())}
              disabled={!name.trim() || isPending}
              iconRight={isPending ? undefined : <ArrowRight size={14} />}
            >
              {isPending ? <Spinner /> : '創建'}
            </Button>
          </div>
        </Dialog.Content>
      </Dialog.Portal>
    </Dialog.Root>
  );
}
