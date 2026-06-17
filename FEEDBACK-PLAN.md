# 反馈系统设计提案（暂未实现）

## 目标
让用户能一键将 SkyNet 桌面端的运行日志 / 问题报告发送给开发者，无需截图或手动复制粘贴。

## 方案：GitHub Issue 自动创建

### 工作流
```
用户在桌面端点击「Report Issue」→ 弹窗填写描述 → 
桌面端通过 GitHub API 在 ghshhf/glibc-packages 创建 Issue →
自动附加：当前日志 + 系统信息（OS/Electron/Node 版本）
```

### 所需组件

#### 1. GitHub Token 配置
- 用户需要在 GitHub 生成 Personal Access Token (`repo` 权限)
- 在桌面端设置页输入 Token（保存在 `electron-store` 或系统密钥链）
- 或用 OAuth Device Flow 简化授权

#### 2. Issue 模板预填充
```
## 环境信息
- OS: {os}
- SkyNet 版本: {version}
- Electron: {electronVersion}
- Node: {nodeVersion}

## 日志
```log
{最近 200 行日志}
```

## 用户描述
{用户输入}
```

#### 3. 按钮位置
- 仪表盘右侧日志面板工具栏：「Report Issue」按钮
- 或「设置」页新增「反馈」区域

### 隐私注意
- GitHub Issue 默认为**公开**，日志中可能包含本地路径和用户名
- 可在 Issue 创建前让用户预览并编辑内容
- 未来可切到私有仓库的 Issue

### 备选：自建服务器中转
如果需要私密反馈，可以架设一个简单的 HTTP 服务器（Cloudflare Worker / VPS），
桌面端把日志 POST 到服务器，开发者通过管理面板查看。

### 状态
📄 **已设计，未实现** — 等需要时再开发。
