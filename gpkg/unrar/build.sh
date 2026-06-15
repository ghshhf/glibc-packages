TERMUX_PKG_HOMEPAGE=https://www.rarlab.com/
TERMUX_PKG_DESCRIPTION="Unarchiver for .rar files"
TERMUX_PKG_LICENSE="Freeware"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=6.2.9
TERMUX_PKG_SRCURL=https://www.rarlab.com/rar/unrarsrc-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=477aecdad08062b20eb63e14f60dc70293dcb92ec7b1d80ab3ea8acec33c2cb4
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	make -f makefile
}

termux_step_make_install() {
	install -Dm755 unrar "$TERMUX_PREFIX/bin/unrar"
}
