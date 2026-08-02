# Agent Guide

本文件给本地 AI 代理使用，说明 Slate 仓库内必须遵守的协作和发布规则。面向人的贡献说明见 [CONTRIBUTING.md](CONTRIBUTING.md)。

## 仓库结构

Bun workspace monorepo，根 `package.json` 无 `name`（workspace root）。四部分：

| 目录 | 运行时 | 说明 |
| --- | --- | --- |
| `backend/` | Bun + NestJS 11 + Fastify | API、Prisma schema、动态帧渲染、音频转码 |
| `frontend/` | React 19 + Vite 8 | Web 管理端 |
| `shared/` | TypeScript 源码 | zod schema、dither、图像预处理，**不需要构建** |
| `firmware/` | ESP-IDF 5.5.x | **独立工程，不属于 Bun workspace** |

## 开发前必读

修改前先读对应模块 README：`backend/README.md`、`frontend/README.md`、`shared/README.md`、`firmware/README.md`。各 README 包含目录结构、数据流和环境变量等关键信息。

## 本地开发命令

```bash
bun install                                # 一次性，安装所有 workspace 依赖

# 启动 MySQL（首次）
docker run -d --name slate-mysql -p 3306:3306 \
  -e MYSQL_ROOT_PASSWORD=root \
  -e MYSQL_DATABASE=slate

# 配置后端环境
cp backend/.env.example backend/.env

# Prisma 生成 + 迁移（首次或 schema 变更后）
bun run --cwd backend prisma:generate       # prisma generate
bun run --cwd backend prisma:migrate        # prisma migrate dev（创建新 migration）

# 开发服务器
bun run dev:backend                         # :3001，含 prisma generate + migrate deploy + watch
bun run dev:frontend                        # :5173，Vite proxy /api 到 :3001
```

关键约定：
- 后端读取 `backend/.env`。根目录 `.env` 仅 Docker Compose 使用，开发模式不会读取。
- `shared` 不构建：前后端都直接 import `shared/src`，运行期无需 `dist`。前端通过 Vite alias 消费，后端通过 workspace 依赖消费。
- 首次访问 `http://localhost:5173/register` 注册账号。

## 校验命令（提交前必跑）

顺序：format -> lint -> typecheck -> test -> frontend build

```bash
bun run format:check                        # Prettier，TS/TSX/JSON
bun run lint                                # ESLint，frontend + backend，零 warning
bun run typecheck                           # tsc --noEmit，frontend + backend
bun run --cwd backend test                  # Bun test
bun run --cwd frontend build                # tsc && vite build
```

固件改动还需：
```bash
source $IDF_PATH/export.sh
idf.py -C firmware build
```

## 提交规则

- 默认在 `master` 上开发。
- Conventional Commits，中文摘要：`<type>(<scope>): <中文摘要>`。
- Body 用 `- ` 条目说明具体改动。
- 需要 body 时，单个 `-m` 传入：`git commit -m $'subject\n\n- item'`。
- 不要加入 AI 署名、生成标识、协作者 trailer 或工具标记。
- 不要回滚用户已有改动；遇到无关脏工作区，忽略即可。

## 前端 UI 约定

- 颜色、圆角、字体只走 `frontend/src/styles/global.css`（Mono Press 设计系统）。
- 目录约定：`features/*` 放业务域组件/hooks/queries；`components/ui` 只放可复用 UI 组件；`hooks` 只放跨 feature hooks。
- 路由表在 `src/app/App.tsx`，路径常量在 `src/app/routes.ts`。

## 后端约定

- 所有 API 端点都在 `/api/v1` 下，唯二例外：`/healthz`（无前缀）。
- 设备鉴权：`Authorization: Bearer <device_secret>`（64 hex）。Web 管理：JWT。
- 新 migration 用 `bun run --cwd backend prisma:migrate`（即 `prisma migrate dev`）。
- 后端 Prisma 使用 MariaDB adapter 直连 MySQL 8。本地无 TLS 时通常需设 `DB_ALLOW_PUBLIC_KEY_RETRIEVAL=true`。

## 固件改动注意事项

- 影响 partition、NVS schema、同步协议或 OTA 路径时，在最终说明里明确风险和验证结果。
- `firmware/sdkconfig.defaults` 已固化 ESP32-S3 target、Flash/PSRAM、分区表等配置，无需 `idf.py set-target`。

## 发布规则

Slate 使用单一产品版本号。一个 `vX.Y.Z` tag 同时发布 Docker 镜像和固件产物，不要拆成两个 release。

### 发版前必须同步版本号

以下位置必须与 tag 去掉 `v` 后一致：

- `package.json`
- `backend/package.json`
- `frontend/package.json`
- `shared/package.json`
- `bun.lock` 中 workspace package 的版本记录
- `firmware/sdkconfig.defaults` 中的 `CONFIG_APP_PROJECT_VER`

更新 package 版本后运行一次 `bun install`，让 `bun.lock` 同步 workspace 版本。不要只改 `package.json`。

### 只能使用 annotated tag

Release notes 来自 tag body。不要创建 lightweight tag。

```bash
git tag -a v0.2.0
git push origin v0.2.0
```

### CI 发布流程

推送 `vX.Y.Z` 后，`.github/workflows/release.yml` 自动完成：校验 annotated tag -> 版本号一致性 -> format/lint/typecheck/test/build -> Docker 镜像推送 -> 固件构建 -> GitHub Release 创建。

### 禁止事项

- 不要手动编辑 GitHub Release notes；应修改 annotated tag body 后重新推送 tag。
- 不要在 release workflow 之外推送 `vX.Y.Z` 镜像 tag。
- 不要让 package 版本、固件版本和 Git tag 不一致。
- 不要把正式 release 建在非 `vX.Y.Z` tag 上。
- 不要重跑旧版本 tag；`release.yml` 会拒绝非最高 `vX.Y.Z` tag，避免 Docker `latest` 回滚。
