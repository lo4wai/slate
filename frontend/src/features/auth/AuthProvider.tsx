// JWT 登錄態管理。App 啓動時如果 localStorage 有 token，自動 GET /users/current 回填 user
// （否則 Layout 等組件讀 ctx.user 永遠是 null，刷新後看不到登出菜單）。

import { useCallback, useEffect, useState, type ReactNode } from 'react';
import { useQueryClient } from '@tanstack/react-query';
import { useNavigate } from 'react-router-dom';
import { resetUnauthorizedState, setUnauthorizedHandler } from '@/features/auth/lib/auth-events';
import { AUTH_TOKEN_STORAGE_KEY, tokenStorage } from '@/features/auth/lib/auth-storage';
import { API_PREFIX, api } from '@/lib/http';
import { notifySessionEnded } from '@/features/auth/lib/session-events';
import { meQueryKey, useMe, type CurrentUser } from '@/features/auth/query/auth-queries';
import { appRoutes } from '@/app/routes';
import type { LoginRequestT, LoginResponseT, RegisterRequestT, RegisterResponseT } from 'shared';
import { AuthContext } from './model/auth-context';
import { safeRedirectPath } from './lib/redirect';

export function AuthProvider({ children }: { children: ReactNode }) {
  const [token, setToken] = useState<string | null>(tokenStorage.get());
  const navigate = useNavigate();
  const qc = useQueryClient();
  const me = useMe(!!token);
  const user = token ? (me.data ?? null) : null;

  const handleAuthSuccess = useCallback(
    (data: { token: string; user: CurrentUser }, redirectTo: string) => {
      tokenStorage.set(data.token);
      resetUnauthorizedState();
      setToken(data.token);
      qc.setQueryData(meQueryKey, data.user);
      navigate(safeRedirectPath(redirectTo), { replace: true });
    },
    [navigate, qc]
  );

  const clearLocalSession = useCallback(() => {
    setToken(null);
    qc.clear();
    notifySessionEnded();
  }, [qc]);

  useEffect(() => {
    return setUnauthorizedHandler(() => {
      tokenStorage.clear();
      clearLocalSession();
      if (window.location.pathname !== appRoutes.login)
        navigate(appRoutes.login, { replace: true });
    });
  }, [clearLocalSession, navigate]);

  useEffect(() => {
    function onStorage(event: StorageEvent) {
      if (event.key !== AUTH_TOKEN_STORAGE_KEY) return;
      if (!event.newValue) {
        clearLocalSession();
        if (window.location.pathname !== appRoutes.login)
          navigate(appRoutes.login, { replace: true });
        return;
      }
      setToken(event.newValue);
      resetUnauthorizedState();
      void qc.invalidateQueries({ queryKey: meQueryKey });
    }
    window.addEventListener('storage', onStorage);
    return () => window.removeEventListener('storage', onStorage);
  }, [clearLocalSession, navigate, qc]);

  const login = useCallback(
    async (creds: LoginRequestT, redirectTo: string = appRoutes.home) => {
      const { data } = await api.post<LoginResponseT>(`${API_PREFIX}/sessions`, creds);
      handleAuthSuccess(data, redirectTo);
    },
    [handleAuthSuccess]
  );

  const register = useCallback(
    async (creds: RegisterRequestT, redirectTo: string = appRoutes.home) => {
      const { data } = await api.post<RegisterResponseT>(`${API_PREFIX}/users`, creds);
      handleAuthSuccess(data, redirectTo);
    },
    [handleAuthSuccess]
  );

  const logout = useCallback(() => {
    // 先把本地態清掉並跳登錄頁，再後台 best-effort 撤銷 session：
    // 服務端 logout 現在還是佔位 no-op，等失敗也不影響用户體驗。
    const existingToken = tokenStorage.get();
    resetUnauthorizedState();
    tokenStorage.clear();
    clearLocalSession();
    navigate(appRoutes.login, { replace: true });
    if (existingToken) {
      api
        .delete(`${API_PREFIX}/sessions/current`, {
          headers: { Authorization: `Bearer ${existingToken}` },
        })
        .catch(() => {});
    }
  }, [clearLocalSession, navigate]);

  return (
    <AuthContext.Provider value={{ token, user, login, register, logout }}>
      {children}
    </AuthContext.Provider>
  );
}
