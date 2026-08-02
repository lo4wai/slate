// 按設備屏上的 6 位配對碼綁定設備。
//
// 流程：用户拿到一台已經聯網但未綁定的 Slate 設備 → 設備屏上顯示配對碼 →
// 用户在此對話框輸入 → 後端找到 device、把 owner 設為當前用户、輪換 pair_code。
//
// 命名留到綁定後在設備列表 PATCH name。

import { useState } from 'react';
import * as Dialog from '@radix-ui/react-dialog';
import { ArrowRight, KeyRound } from 'lucide-react';
import { useClaimByPairCode } from '@/features/devices/query/device-queries';
import { useToast } from '@/components/feedback/toast-context';
import { isValidPairCode, normalizePairCode } from '@/features/devices/lib/pair-code';
import { getApiErrorMessage, getApiErrorStatus } from '@/lib/api-errors';
import { Input } from '@/components/ui/Input';
import { Button } from '@/components/ui/Button';
import { Spinner } from '@/components/ui/Spinner';
import { DialogHeader } from '@/components/ui/DialogHeader';
import { dialogContentCls, dialogOverlayCls } from '@/components/ui/styles/dialog';

interface AddDeviceDialogProps {
  open: boolean;
  onOpenChange: (o: boolean) => void;
}

export function AddDeviceDialog({ open, onOpenChange }: AddDeviceDialogProps) {
  const [code, setCode] = useState('');
  const claim = useClaimByPairCode();
  const toast = useToast();

  const codeValid = isValidPairCode(code);

  function reset() {
    setCode('');
  }

  async function onSubmit(e: { preventDefault(): void }) {
    e.preventDefault();
    if (!codeValid) return;
    try {
      const device = await claim.mutateAsync({ pair_code: normalizePairCode(code) });
      // 後端 claim 時若 owner 已有相冊會自動綁第一個，無相冊則後續 create 會反向綁；
      // 這裏只給出與實際後端行為一致的概要提示，不做額外引導（用户可在設備列表看進度）。
      toast.success(
        '設備已綁定',
        device.selected_group_id ? '設備屏將開始同步相冊' : '請創建一個相冊，設備屏會自動同步'
      );
      reset();
      onOpenChange(false);
    } catch (err) {
      const status = getApiErrorStatus(err);
      if (status === 404) {
        toast.error('配對碼無效', '請核對設備屏上的碼，或在設備上長按 ENTER 工廠重置後重試。');
      } else if (status === 403) {
        toast.error('該設備已被其他賬號綁定', '在設備上工廠重置後再試。');
      } else {
        toast.error('綁定失敗', getApiErrorMessage(err));
      }
    }
  }

  return (
    <Dialog.Root
      open={open}
      onOpenChange={(o) => {
        onOpenChange(o);
        if (!o) reset();
      }}
    >
      <Dialog.Portal>
        <Dialog.Overlay className={dialogOverlayCls} />
        <Dialog.Content className={dialogContentCls}>
          <DialogHeader
            icon={<KeyRound size={24} />}
            title="添加設備"
            description="在設備屏上查看 6 位配對碼，輸入此處即綁定。"
            className="mb-6"
          />

          <form onSubmit={onSubmit} className="space-y-5">
            <Input
              label="配對碼"
              value={code}
              onChange={(e) => setCode(e.target.value)}
              placeholder="K7M9X2"
              autoFocus
              autoComplete="off"
              spellCheck={false}
              maxLength={8}
              hint={code && !codeValid ? undefined : '6 位字母+數字，可帶短橫線'}
              error={code && !codeValid ? '配對碼格式不正確' : undefined}
              className="font-mono uppercase tracking-[0.2em] text-center"
            />

            <div className="flex items-center justify-end gap-3 pt-2">
              <Dialog.Close asChild>
                <Button variant="outline" type="button">
                  取消
                </Button>
              </Dialog.Close>
              <Button
                type="submit"
                disabled={!codeValid || claim.isPending}
                iconRight={claim.isPending ? undefined : <ArrowRight size={14} />}
              >
                {claim.isPending ? <Spinner /> : '綁定'}
              </Button>
            </div>
          </form>
        </Dialog.Content>
      </Dialog.Portal>
    </Dialog.Root>
  );
}
