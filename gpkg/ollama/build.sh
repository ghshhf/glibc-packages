TERMUX_PKG_HOMEPAGE=https://ollama.ai/
TERMUX_PKG_DESCRIPTION="Local LLM inference server and CLI"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION=0.30.0
TERMUX_PKG_SRCURL=https://github.com/ollama/ollama/archive/refs/tags/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true
TERMUX_PKG_AUTO_UPDATE=true

termux_step_make() {
	cd $TERMUX_PKG_SRCDIR
	go generate ./...
	go build .
}

termux_step_make_install() {
	install -Dm755 ollama $TERMUX_PREFIX/bin/ollama
}
