// 全局 confirm：替代瀏覽器 window.confirm，用 Radix Dialog 風格化。
//
// 用法：
//   const confirm = useConfirm();
//   if (await confirm({ title: '刪除這一幀?', description: '此操作不可逆。', destructive: true })) {
//     del.mutate(...);
//   }
//
// 在 App 根掛 <ConfirmProvider /> 一次。

import { useCallback, useEffect, useRef, useState, type ReactNode } from 'react';
import * as Dialog from '@radix-ui/react-dialog';
import { AlertTriangle } from 'lucide-react';
import { Button } from '@/components/ui/Button';
import { IconBlock } from '@/components/ui/IconBlock';
import { dialogContentConfirmCls, dialogOverlayConfirmCls } from '@/components/ui/styles/dialog';
import { ConfirmCtx, type ConfirmOptions } from './confirm-context';

type Resolve = (ok: boolean) => void;
type ConfirmRequest = ConfirmOptions & { resolve: Resolve };

export function ConfirmProvider({ children }: { children: ReactNode }) {
  const [active, setActive] = useState<ConfirmRequest | null>(null);
  const activeRef = useRef<ConfirmRequest | null>(null);
  const queueRef = useRef<ConfirmRequest[]>([]);

  const show = useCallback((request: ConfirmRequest | null) => {
    activeRef.current = request;
    setActive(request);
  }, []);

  const confirm = useCallback(
    (options: ConfirmOptions) => {
      return new Promise<boolean>((resolve) => {
        // 單一路徑：始終入隊，再決定要不要立即 show。兩次同步 confirm() 不會產生
        // 「都看到 active=null 都直接 show」的覆蓋競態。
        queueRef.current.push({ ...options, resolve });
        if (!activeRef.current) {
          const next = queueRef.current.shift();
          if (next) show(next);
        }
      });
    },
    [show]
  );

  const close = useCallback(
    (ok: boolean) => {
      const request = activeRef.current;
      if (!request) return;
      request.resolve(ok);
      show(queueRef.current.shift() ?? null);
    },
    [show]
  );

  useEffect(() => {
    return () => {
      activeRef.current?.resolve(false);
      activeRef.current = null;
      for (const request of queueRef.current) request.resolve(false);
      queueRef.current = [];
    };
  }, []);

  return (
    <ConfirmCtx.Provider value={confirm}>
      {children}
      <Dialog.Root
        open={!!active}
        onOpenChange={(open) => {
          if (!open) close(false);
        }}
      >
        <Dialog.Portal>
          {/* z-index 故意比其它 dialog 高一檔：
                普通 dialog （ImageContentEditor / Groups create）： overlay z-40 / content z-50
                Confirm                                  ： overlay z-50 / content z-60
              因為業務裏 dialog 內部還可能再觸發 confirm（如 ImageContentEditor 刪音頻），
              confirm 必須蓋在那些 dialog 之上。改前請確認這條不變量。 */}
          <Dialog.Overlay className={dialogOverlayConfirmCls} />
          <Dialog.Content className={dialogContentConfirmCls}>
            <div className="flex items-center gap-4">
              <IconBlock size="lg" tone={active?.destructive ? 'danger' : 'muted'}>
                <AlertTriangle size={24} />
              </IconBlock>
              <div className="min-w-0 flex-1">
                <Dialog.Title className="font-serif text-[20px] font-bold leading-[1.2]">
                  {active?.title}
                </Dialog.Title>
                {active?.description && (
                  <Dialog.Description className="font-sans text-[13px] text-stone mt-1.5 leading-relaxed">
                    {active.description}
                  </Dialog.Description>
                )}
              </div>
            </div>

            <div className="mt-6 flex items-center justify-end gap-3">
              <Button variant="outline" onClick={() => close(false)}>
                {active?.cancelText ?? '取消'}
              </Button>
              <Button
                variant={active?.destructive ? 'danger' : 'primary'}
                onClick={() => close(true)}
                autoFocus
              >
                {active?.confirmText ?? '確認'}
              </Button>
            </div>
          </Dialog.Content>
        </Dialog.Portal>
      </Dialog.Root>
    </ConfirmCtx.Provider>
  );
}
