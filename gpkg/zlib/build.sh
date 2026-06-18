# =============================================================================
# zlib — 压缩库 (标准模板包)
# =============================================================================
#
# 本文件是 gpkg 包的标准模板，新包请以此文件为蓝本。
# 每个字段的含义见 CONTRIBUTING.md → Package Definition Format。
#
# 字段规范速查:
#   TERMUX_PKG_HOMEPAGE        ✅ 必填 — 项目主页
#   TERMUX_PKG_DESCRIPTION     ✅ 必填 — ≤100 字符，首字母大写，不以标点结尾
#   TERMUX_PKG_LICENSE         ✅ 必填 — SPDX 标识符
#   TERMUX_PKG_MAINTAINER     ✅ 必填 — @ghshhf (本 fork)
#   TERMUX_PKG_VERSION         ✅ 必填 — 上游版本
#   TERMUX_PKG_SRCURL          ✅ 必填 — 使用 ${TERMUX_PKG_VERSION} 变量
#   TERMUX_PKG_SHA256          ✅ 必填 — 与源码 tarball 匹配
#   TERMUX_PKG_DEPENDS         ⭕ 可选 — 逗号分隔运行时依赖
#   TERMUX_PKG_BUILD_DEPENDS   ⭕ 可选 — 逗号分隔构建时依赖
#   TERMUX_PKG_BUILD_IN_SRC    ⭕ 可选 — true = 在源码目录内构建
#   TERMUX_PKG_FORCE_CMAKE     ⭕ 可选 — true = 强制 CMake
#   TERMUX_PKG_EXTRA_CONFIGURE_ARGS  ⭕ 可选 — 额外 configure 参数
# =============================================================================

TERMUX_PKG_HOMEPAGE=https://zlib.net/
TERMUX_PKG_DESCRIPTION="Compression library implementing the deflate compression method found in gzip and PKZIP"
TERMUX_PKG_LICENSE="ZLIB"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION=1.3.2
TERMUX_PKG_SRCURL=https://github.com/madler/zlib/releases/download/v${TERMUX_PKG_VERSION}/zlib-${TERMUX_PKG_VERSION}.tar.xz
TERMUX_PKG_SHA256=d7a0654783a4da529d1bb793b7ad9c3318020af77667bcae35f95d0e42a792f3
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_AUTO_UPDATE=true

# =============================================================================
# 构建前配置
# =============================================================================
termux_step_pre_configure() {
	# aarch64: 启用 CRC 指令加速
	if [ "$TERMUX_ARCH" = "aarch64" ]; then
		CFLAGS+=" -march=armv8-a+crc"
		CXXFLAGS+=" -march=armv8-a+crc"
	fi

	# 静态库使用 -fPIC，避免链接重定位错误
	CFLAGS+=" -fPIC"
	CXXFLAGS+=" -fPIC"

	# zlib 1.3+ 链接器脚本兼容性
	LDFLAGS+=" -Wl,--undefined-version"
}

# =============================================================================
# 配置
# =============================================================================
termux_step_configure() {
	"$TERMUX_PKG_SRCDIR/configure" \
		--prefix="$TERMUX_PREFIX" \
		--shared
}
