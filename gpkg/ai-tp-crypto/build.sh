TERMUX_PKG_HOMEPAGE=https://github.com/ghshhf/glibc-packages
TERMUX_PKG_DESCRIPTION="AI-TP crypto library"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION=0.1.0
# 无 SRCURL：源码在项目根目录 ai-tp-crypto/ 下
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_configure() {
	# 从项目根目录复制源码到构建目录
	local src_root="$(cd "${CROSS_PKG_DIR}/../ai-tp-crypto" && pwd)"
	cp -r "${src_root}/"* "${TERMUX_PKG_SRCDIR}/"
}

termux_step_make() {
	gcc -Wall -Wextra -Iinclude -c src/ai-tp-crypto.c -o ai-tp-crypto.o
	ar rcs libai-tp-crypto.a ai-tp-crypto.o
}

termux_step_make_install() {
	install -Dm644 libai-tp-crypto.a "${TERMUX_PREFIX}/lib/libai-tp-crypto.a"
	install -Dm644 include/ai-tp-crypto.h "${TERMUX_PREFIX}/include/ai-tp-crypto.h"
}
