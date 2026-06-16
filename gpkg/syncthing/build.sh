TERMUX_PKG_HOMEPAGE=https://syncthing.net/
TERMUX_PKG_DESCRIPTION="Open Source continuous file synchronization"
TERMUX_PKG_LICENSE="MPL-2.0"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION=2.1.1
TERMUX_PKG_SRCURL=https://github.com/syncthing/syncthing/archive/refs/tags/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true
TERMUX_PKG_AUTO_UPDATE=true

termux_step_make() {
	cd $TERMUX_PKG_SRCDIR
	go run build.go
}

termux_step_make_install() {
	install -Dm755 syncthing $TERMUX_PREFIX/bin/syncthing
}
