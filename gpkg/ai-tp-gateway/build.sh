TERMUX_PKG_HOMEPAGE=https://github.com/ghshhf/glibc-packages
TERMUX_PKG_DESCRIPTION="AI-TP gateway library"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION=0.1.0
# 无 SRCURL：源码在项目根目录 ai-tp-gateway/ 下
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_configure() {
	# 从项目根目录复制源码到构建目录
	local src_root="$(cd "${CROSS_PKG_DIR}/../ai-tp-gateway" && pwd)"
	cp -r "${src_root}/"* "${TERMUX_PKG_SRCDIR}/"
}

termux_step_make() {
	gcc -Wall -Wextra -Iinclude -c src/ai-tp-gateway.c -o ai-tp-gateway.o
	ar rcs libai-tp-gateway.a ai-tp-gateway.o
}

termux_step_make_install() {
	install -Dm644 libai-tp-gateway.a "${TERMUX_PREFIX}/lib/libai-tp-gateway.a"
	install -Dm644 include/ai-tp-gateway.h "${TERMUX_PREFIX}/include/ai-tp-gateway.h"
}
