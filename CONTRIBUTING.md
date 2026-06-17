# Contributing to SkyNet (天网)

感谢你对 SkyNet 的关注！本项目是 AI-TP 协议的开源基础设施，包含：
- **包构建系统** — 跨平台 Termux 风格包构建（遵循 [public-apis](https://github.com/ghshhf/public-apis) 规范）
- **SSI 标准架构** — 完全标准化的底层系统规范与 TypeScript 参考实现
- **浏览器运行时** — SSI-KRN 内核运行时（browser-runtime/）

> 包贡献请遵循 `scripts/validate/format.py` 格式校验。SSI 组件贡献请参考 [SPEC-INTERFACE.md](SPEC-INTERFACE.md) 和 [SYSTEM-STANDARD.md](SYSTEM-STANDARD.md)。

<br >

## Package Definition Format

每个包位于 `gpkg/<package-name>/` 目录下，包含至少一个 `build.sh` 文件。字段规范如下：

| Field | Required | Description | Example |
| --- | --- | --- | --- |
| `TERMUX_PKG_HOMEPAGE` | ✅ | 项目主页 URL | `https://zlib.net` |
| `TERMUX_PKG_DESCRIPTION` | ✅ | 简短描述（不超过 100 字符，不以标点结尾） | `Compression library implementing the deflate compression method` |
| `TERMUX_PKG_LICENSE` | ✅ | SPDX 许可证标识符 | `Zlib` |
| `TERMUX_PKG_MAINTAINER` | ✅ | 维护者 | `@ghshhf` |
| `TERMUX_PKG_VERSION` | ✅ | 上游版本号 | `1.3.1` |
| `TERMUX_PKG_SRCURL` | ✅ | 源码 tarball URL（使用 `${TERMUX_PKG_VERSION}` 变量） | `https://zlib.net/zlib-${TERMUX_PKG_VERSION}.tar.gz` |
| `TERMUX_PKG_SHA256` | ✅ | 源码 tarball 的 SHA256 校验和 | `6b39fb9371...` |
| `TERMUX_PKG_DEPENDS` | ⭕ | 逗号分隔的运行时依赖 | `glibc, libgcc` |
| `TERMUX_PKG_BUILD_DEPENDS` | ⭕ | 逗号分隔的构建时依赖 | `glibc-dev` |
| `TERMUX_PKG_EXTRA_CONFIGURE_ARGS` | ⭕ | 传给 `./configure` 的额外参数 | `--static` |
| `TERMUX_PKG_FORCE_CMAKE` | ⭕ | 强制使用 CMake 而非 autotools | `true` |
| `TERMUX_PKG_BUILD_IN_SRC` | ⭕ | 在源码目录内构建（无独立 build 目录） | `true` |

### Naming Convention

- 内部包使用 `-glibc` 后缀：`ncurses-glibc`、`readline-glibc`、`libevent-glibc`
- `glibc` 本身除外
- 包名使用小写字母、数字和连字符（`-`），不使用下划线或大写

### Build System Auto-detection

| Method | Detection |
| --- | --- |
| Autotools | `./configure` 存在 → `./configure && make && make install` |
| CMake | `CMakeLists.txt` 存在 或 `TERMUX_PKG_FORCE_CMAKE=true` → CMake build |
| Meson | `meson.build` 存在 → Meson + ninja |
| Rust/Cargo | `Cargo.toml` 存在 → `cargo build --release` |
| Go | `go.mod` 存在 → `go build` |
| Custom Makefile | `Makefile` 存在 + `TERMUX_PKG_BUILD_IN_SRC=true` → `make && make install` |
| Shell script | 纯脚本项目 → 直接安装文件到目标目录 |

若上游仅提供 `configure.ac` 而无预生成的 `configure`，请添加：

```bash
termux_step_pre_configure() {
    autoreconf -fi
}
```

### Patches

将 `.patch` 文件放在包目录下。构建前会自动应用。命名使用描述性名称，如 `fix-paths.patch`、`setdirs.patch`。

<br >

## README Table Format

本项目的 README.md 采用与 [public-apis](https://github.com/ghshhf/public-apis) 一致的 Markdown 表格规范。

### Current API entry format

| Package | Description | Build System | Platforms | Status |
| --- | --- | --- | --- | --- |

Example entry:

```
| [zlib](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/zlib) | 无损数据压缩库 | configure | All | ✅ |
```

### Formatting Rules

- 每列表头前后各保留一个空格：`| Header |`
- 描述不超过 100 字符
- 描述不以标点符号结尾（`)`、`]` 除外）
- 描述首字母大写
- 包名称按字母顺序排列在各分类下
- 每个 `### Category` 下的条目数不少于 3 条
- Package 名称不包含 `-glibc` 后缀（表格中省略，但实际目录名需要）
- Status 列使用：✅（已实现）、📋（计划中）、🚧（进行中）、⚠️（有问题）

### Index

所有分类标题必须在顶部的 `## Index` 部分列出，使用如下格式：

```
* [分类标题](#分类标题的-anchor-名称)
```

<br >

## Pull Request Guidelines

- **每个 PR 添加或更新一个包**
- **PR 标题格式**：`Add <package-name>` 或 `Update <package-name>`
- **提交信息**使用简短描述：`Add zlib to 基础系统库` 而非 `Update README.md`
- **检查重复**：在提交前搜索现有 Issues 和 PR
- **通过格式校验**：在提交前本地运行 `python3 scripts/validate/format.py README.md`
- **表格按字母顺序**：新包插入到分类下的正确位置
- **目标分支**：`master`

### PR Checklist

提交 PR 前请确认：

- [ ] `build.sh` 遵循上方的字段规范
- `TERMUX_PKG_DESCRIPTION` 不超过 100 字符，不以标点结尾
- `TERMUX_PKG_SHA256` 与源码 tarball 匹配
- 依赖项使用正确的 `-glibc` 命名
- `README.md` 中对应条目已添加或更新
- 表格按字母顺序排列
- 每个表格列前后各有一个空格
- [ ] `python3 scripts/validate/format.py README.md` 无报错
- [ ] Patch 文件（如有）有清晰的命名

<br >

## Link Validation

README.md 中的所有外部链接会在以下情况自动验证：

- **Push 到 master 分支**：运行重复链接检测
- **Scheduled（每日 UTC 00:00）**：全量链接可用性检查

在本地手动检查：

```bash
python3 scripts/validate/links.py README.md --only_duplicate_links_checker
```

<br >

## Reporting Issues

- **Bug 报告**：发现某个包构建失败或存在问题时，请开启 Issue
- **包请求**：请求添加新的软件包
- **文档改进**：发现文档有误或可以改进

<br >

## Security Issues

安全漏洞请参考 [SECURITY.md](SECURITY.md)。
