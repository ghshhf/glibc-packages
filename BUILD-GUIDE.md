# SkyNet 构建指南

> 各平台安装包构建完整说明

---

## 🪟 Windows 构建

### 环境要求
- Node.js 18+ (推荐 22.x)
- Windows 10/11 x64

### 构建步骤

```bash
cd desktop
npm install
npm run build:win
```

### 输出
- `dist/SkyNet-SSI-Runtime-win32-x64/` — 解压即用文件夹
- `dist/SkyNet-SSI-Runtime-1.0.0-win32-x64.zip` — ZIP 分发包

### 运行
解压 ZIP 后双击 `SkyNet.exe` 即可运行。

---

## 📱 Android 构建

### 环境要求
- Android SDK (API 34)
- JDK 17
- Gradle 8.x (wrapper 已包含)

### 构建步骤

```bash
cd android
./gradlew.bat assembleDebug   # Windows
./gradlew assembleDebug       # macOS/Linux
```

### 输出
- `app/build/outputs/apk/debug/app-debug.apk`

### 安装
```bash
adb install app/build/outputs/apk/debug/app-debug.apk
```

---

## 🍎 macOS 构建

### 环境要求
- macOS 12+ (Monterey)
- Xcode Command Line Tools
- Node.js 18+

### 构建步骤

```bash
cd desktop
npm install

# 安装 electron-packager 到项目
npm install --save-dev electron-packager

# 构建 macOS 版本
npx electron-packager . SkyNet-SSI-Runtime \
  --platform=darwin \
  --arch=x64,arm64 \
  --out=../dist \
  --icon=public/icon.icns \
  --overwrite

# 可选：创建 DMG
npm install --save-dev electron-installer-dmg
npx electron-installer-dmg \
  ../dist/SkyNet-SSI-Runtime-darwin-x64/SkyNet-SSI-Runtime.app \
  SkyNet-SSI-Runtime-1.0.0 \
  --out=../dist
```

### 图标准备
需要 `.icns` 格式图标：
```bash
# 使用 sips 和 iconutil 创建 icns
mkdir icon.iconset
sips -z 16 16     icon.png --out icon.iconset/icon_16x16.png
sips -z 32 32     icon.png --out icon.iconset/icon_16x16@2x.png
sips -z 32 32     icon.png --out icon.iconset/icon_32x32.png
sips -z 64 64     icon.png --out icon.iconset/icon_32x32@2x.png
sips -z 128 128   icon.png --out icon.iconset/icon_128x128.png
sips -z 256 256   icon.png --out icon.iconset/icon_128x128@2x.png
sips -z 256 256   icon.png --out icon.iconset/icon_256x256.png
sips -z 512 512   icon.png --out icon.iconset/icon_256x256@2x.png
sips -z 512 512   icon.png --out icon.iconset/icon_512x512.png
sips -z 1024 1024 icon.png --out icon.iconset/icon_512x512@2x.png
iconutil -c icns icon.iconset -o public/icon.icns
```

---

## 🐧 Linux 构建

### 环境要求
- Ubuntu 22.04+ / Debian 12+ / Fedora 39+
- Node.js 18+

### 构建步骤

```bash
cd desktop
npm install

# 构建 Linux 版本
npx electron-packager . SkyNet-SSI-Runtime \
  --platform=linux \
  --arch=x64 \
  --out=../dist \
  --icon=public/icon.png \
  --overwrite

# 可选：创建 AppImage
npm install --save-dev electron-builder
npx electron-builder --linux AppImage
```

### 运行
```bash
./SkyNet-SSI-Runtime
```

---

## 🚀 发布到 GitHub Release

### 1. 打标签
```bash
git tag -a v1.0.0 -m "SkyNet v1.0.0"
git push origin v1.0.0
```

### 2. 使用 GitHub CLI 创建 Release
```bash
# 创建 Release 并上传附件
gh release create v1.0.0 \
  -t "SkyNet SSI Runtime v1.0.0" \
  -n "Release notes here" \
  dist/SkyNet-SSI-Runtime-1.0.0-win32-x64.zip \
  dist/SkyNet-Runtime-v1.0.0.apk
```

### 3. 或使用网页上传
- 访问 https://github.com/{user}/{repo}/releases
- 点击 "Draft a new release"
- 选择 tag，填写标题和说明
- 拖拽上传 ZIP/APK/DMG 文件

---

## 🔧 故障排除

### Windows: electron-builder 报错 (winCodeSign)
**问题**：winCodeSign-2.6.0.7z 包含 macOS 符号链接，Windows 无法解压  
**解决**：改用 `electron-packager`（已配置在 `build-electron.js`）

### Android: Gradle 下载慢
**问题**：Gradle wrapper 下载超时  
**解决**：
1. 手动下载 Gradle 8.11.1-all.zip
2. 放到 `~/.gradle/wrapper/dists/gradle-8.11.1-all/`
3. 重新运行构建

### macOS: 签名问题
**问题**：应用无法打开，提示未验证开发者  
**解决**：
```bash
xattr -cr /Applications/SkyNet-SSI-Runtime.app
```

### Linux: 缺少依赖
**问题**：启动报错 libgtk 等库缺失  
**解决**：
```bash
# Ubuntu/Debian
sudo apt install libgtk-3-0 libnotify4 libnss3 libxss1 libxtst6 xdg-utils

# Fedora
sudo dnf install gtk3 libnotify nss libXScrnSaver libXtst xdg-utils
```

---

## 📁 项目结构

```
glibc-packages/
├── desktop/           # Electron 桌面应用
│   ├── src/          # 主进程 + 预加载脚本
│   ├── public/       # HTML/CSS/JS 仪表盘
│   ├── build-electron.js  # 自定义构建脚本
│   └── package.json
├── android/           # Android WebView 应用
│   ├── app/src/main/ # Java 源码 + 资源
│   └── gradle/       # Gradle wrapper
└── dist/             # 构建输出（.gitignore 忽略）
```

---

## 📝 贡献指南

添加新平台支持：
1. 在 `desktop/` 下添加平台检测和构建配置
2. 更新 `BUILD-GUIDE.md` 添加该平台说明
3. 测试构建并上传 Release 附件
4. 更新 README 下载表格
