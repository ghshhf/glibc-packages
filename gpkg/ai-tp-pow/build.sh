TERMUX_PKG_HOMEPAGE=https://github.com/ghshhf/glibc-packages
TERMUX_PKG_DESCRIPTION="AI-TP pow library"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION=0.1.0
# 无 SRCURL：源码在项目根目录 ai-tp-pow/ 下
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_configure() {
	# 从项目根目录复制源码到构建目录
	local src_root="$(cd "${CROSS_PKG_DIR}/../ai-tp-pow" && pwd)"
	cp -r "${src_root}/"* "${TERMUX_PKG_SRCDIR}/"
}

termux_step_make() {
	gcc -Wall -Wextra -Iinclude -c src/ai-tp-pow.c -o ai-tp-pow.o
	ar rcs libai-tp-pow.a ai-tp-pow.o
}

termux_step_make_install() {
	install -Dm644 libai-tp-pow.a "${TERMUX_PREFIX}/lib/libai-tp-pow.a"
	install -Dm644 include/ai-tp-pow.h "${TERMUX_PREFIX}/include/ai-tp-pow.h"
}
