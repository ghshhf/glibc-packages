TERMUX_PKG_HOMEPAGE=https://www.figlet.org/
TERMUX_PKG_DESCRIPTION="Program for making large letters out of ordinary text"
TERMUX_PKG_LICENSE="BSD-3-Clause"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=2.2.5
TERMUX_PKG_SRCURL=https://github.com/cmatsuoka/figlet/archive/refs/tags/${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=4d366c4a618ecdd6fdb81cde90edc54dbff9764efb635b3be47a929473f13930
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	make CC="$CC" CFLAGS="$CFLAGS $CPPFLAGS -Os" LDFLAGS="$LDFLAGS" prefix="$TERMUX_PREFIX" DEFAULTFONTDIR="$TERMUX_PREFIX/share/figlet"
}

termux_step_make_install() {
	make install prefix="$TERMUX_PREFIX" BINDIR="$TERMUX_PREFIX/bin" MANDIR="$TERMUX_PREFIX/share/man" DEFAULTFONTDIR="$TERMUX_PREFIX/share/figlet"
	install -Dm644 fonts/*.flf "$TERMUX_PREFIX/share/figlet/"
}
