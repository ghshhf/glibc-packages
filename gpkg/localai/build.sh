TERMUX_PKG_HOMEPAGE=https://localai.io/
TERMUX_PKG_DESCRIPTION="Local AI inference engine for self-hosted LLMs"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION=4.4.3
TERMUX_PKG_SRCURL=https://github.com/mudler/LocalAI/archive/refs/tags/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true
TERMUX_PKG_AUTO_UPDATE=true

termux_step_make() {
	cd $TERMUX_PKG_SRCDIR
	make build
}

termux_step_make_install() {
	install -Dm755 local-ai $TERMUX_PREFIX/bin/local-ai
}
