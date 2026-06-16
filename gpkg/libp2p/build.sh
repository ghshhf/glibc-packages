TERMUX_PKG_HOMEPAGE=https://github.com/libp2p/go-libp2p
TERMUX_PKG_DESCRIPTION="libp2p networking stack for peer-to-peer applications"
TERMUX_PKG_LICENSE="Apache-2.0"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION=0.48.0
TERMUX_PKG_SRCURL=https://github.com/libp2p/go-libp2p/archive/refs/tags/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true
TERMUX_PKG_AUTO_UPDATE=true

termux_step_make() {
	cd $TERMUX_PKG_SRCDIR
	go build -buildmode=c-shared -o libp2p.so ./...
}

termux_step_make_install() {
	install -Dm755 libp2p.so $TERMUX_PREFIX/lib/libp2p.so
}
