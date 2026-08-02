import { useState } from 'react';
import { Link, Navigate, useLocation } from 'react-router-dom';
import { Input } from '@/components/ui/Input';
import { appRoutes } from '@/app/routes';
import { AuthFormLayout } from '@/features/auth/components/AuthFormLayout';
import { useAuth } from '@/features/auth/hooks/useAuth';
import { useAuthForm } from '@/features/auth/hooks/useAuthForm';
import { redirectFromLocationState } from '@/features/auth/lib/redirect';

type AuthPageMode = 'login' | 'register';

export function AuthPage({ mode }: { mode: AuthPageMode }) {
  return mode === 'login' ? <LoginAuthPage /> : <RegisterAuthPage />;
}

function LoginAuthPage() {
  const { token, login } = useAuth();
  const location = useLocation();
  const redirectTo = redirectFromLocationState(location.state);
  const [identifier, setIdentifier] = useState('');
  const [password, setPassword] = useState('');
  const authForm = useAuthForm();

  if (token) return <Navigate to={redirectTo} replace />;

  async function onSubmit(e: { preventDefault(): void }) {
    e.preventDefault();
    await authForm.run(
      () => login({ identifier, password }, redirectTo),
      '登錄失敗，請檢查賬號和密碼'
    );
  }

  return (
    <AuthFormLayout
      title="登錄"
      subtitle="登錄後管理墨箋與內容。"
      submitLabel="進入"
      loading={authForm.loading}
      error={authForm.error}
      onSubmit={onSubmit}
      footer={
        <p className="mt-7 text-center font-sans text-[13px] text-stone">
          還沒有賬號？{' '}
          <Link to={appRoutes.register} className="text-ink border-b border-ink">
            立即註冊
          </Link>
        </p>
      }
    >
      <Input
        label="賬號或郵箱"
        type="text"
        value={identifier}
        onChange={(e) => setIdentifier(e.target.value)}
        autoFocus
        required
        autoComplete="username"
        placeholder="用户名或郵箱"
      />
      <Input
        label="密碼"
        type="password"
        value={password}
        onChange={(e) => setPassword(e.target.value)}
        required
        autoComplete="current-password"
        placeholder="請輸入密碼"
      />
    </AuthFormLayout>
  );
}

function RegisterAuthPage() {
  const { token, register } = useAuth();
  const location = useLocation();
  const redirectTo = redirectFromLocationState(location.state);
  const [email, setEmail] = useState('');
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [confirm, setConfirm] = useState('');
  const [fieldErrors, setFieldErrors] = useState<Partial<Record<RegisterField, string>>>({});
  const authForm = useAuthForm();

  if (token) return <Navigate to={redirectTo} replace />;

  async function onSubmit(e: { preventDefault(): void }) {
    e.preventDefault();
    authForm.setError(null);
    setFieldErrors({});

    const trimmedEmail = email.trim();
    const validationError = validateRegisterForm({
      email: trimmedEmail,
      username,
      password,
      confirm,
    });
    if (validationError) {
      setFieldErrors({ [validationError.field]: validationError.message });
      return;
    }

    await authForm.run(
      () => register({ email: trimmedEmail, username, password }, redirectTo),
      '註冊失敗，請稍後再試'
    );
  }

  return (
    <AuthFormLayout
      title="註冊"
      subtitle="創建賬號，開始管理墨箋與內容。"
      submitLabel="創建賬號"
      loading={authForm.loading}
      error={authForm.error}
      onSubmit={onSubmit}
      footer={
        <p className="mt-7 text-center font-sans text-[13px] text-stone">
          已有賬號？{' '}
          <Link to={appRoutes.login} className="text-ink border-b border-ink">
            去登錄
          </Link>
        </p>
      }
    >
      <Input
        label="用户名"
        type="text"
        value={username}
        onChange={(e) => setUsername(e.target.value)}
        autoFocus
        required
        autoComplete="username"
        placeholder="字母、數字、下劃線，3-32 位"
        error={fieldErrors.username}
      />
      <Input
        label="郵箱"
        type="email"
        value={email}
        onChange={(e) => setEmail(e.target.value)}
        required
        autoComplete="email"
        placeholder="you@example.com"
        error={fieldErrors.email}
      />
      <Input
        label="密碼"
        type="password"
        value={password}
        onChange={(e) => setPassword(e.target.value)}
        required
        minLength={8}
        autoComplete="new-password"
        placeholder="請輸入密碼"
        error={fieldErrors.password}
      />
      <Input
        label="確認密碼"
        type="password"
        value={confirm}
        onChange={(e) => setConfirm(e.target.value)}
        required
        minLength={8}
        autoComplete="new-password"
        placeholder="再次輸入密碼"
        error={fieldErrors.confirm}
      />
    </AuthFormLayout>
  );
}

type RegisterField = 'email' | 'username' | 'password' | 'confirm';

function validateRegisterForm({
  email,
  username,
  password,
  confirm,
}: {
  email: string;
  username: string;
  password: string;
  confirm: string;
}): { field: RegisterField; message: string } | null {
  if (!/^[a-zA-Z0-9_]{3,32}$/.test(username)) {
    return { field: 'username', message: '用户名只能包含字母、數字、下劃線，3-32 位' };
  }
  if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email)) {
    return { field: 'email', message: '請輸入有效的郵箱地址' };
  }
  if (password.length < 8) {
    return { field: 'password', message: '密碼至少 8 位' };
  }
  if (password !== confirm) {
    return { field: 'confirm', message: '兩次輸入的密碼不一致' };
  }
  return null;
}
