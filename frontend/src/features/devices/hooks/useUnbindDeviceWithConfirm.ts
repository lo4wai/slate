import { useCallback } from 'react';
import type { DeviceSummaryT } from 'shared';
import { useConfirmAction } from '@/hooks/useMutationAction';
import { useUnbindDevice } from '@/features/devices/query/device-queries';

export function useUnbindDeviceWithConfirm(device: DeviceSummaryT, onSuccess?: () => void) {
  const { mutate, isPending } = useUnbindDevice();

  const confirmUnbind = useConfirmAction<string>({
    isPending,
    getConfirmOptions: useCallback(
      () => ({
        title: '解綁這台設備？',
        description: `${device.name ?? device.mac} 將從你的賬號移除。素材保留，設備屏會切回配對碼狀態。`,
        destructive: true,
        confirmText: '解綁',
      }),
      [device.mac, device.name]
    ),
    run: useCallback((deviceId, callbacks) => mutate(deviceId, callbacks), [mutate]),
    successToast: { message: '已解綁', hint: '設備屏會顯示新配對碼。' },
    errorToast: '解綁失敗',
    onSuccess: useCallback(() => onSuccess?.(), [onSuccess]),
  });
  const unbindWithConfirm = useCallback(() => {
    void confirmUnbind(device.id);
  }, [confirmUnbind, device.id]);

  return { unbindWithConfirm, isPending };
}
