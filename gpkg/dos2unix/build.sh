TERMUX_PKG_HOMEPAGE=https://waterlan.home.xs4all.nl/dos2unix.html
TERMUX_PKG_DESCRIPTION="Convert text files between DOS, Unix and Mac formats"
TERMUX_PKG_LICENSE="BSD-2-Clause"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=7.5.5
TERMUX_PKG_SRCURL=https://waterlan.home.xs4all.nl/dos2unix/dos2unix-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=75f692b8484c8c24579a2ffd87df16b9c9428ed95497e3393a21d1ba0697ac33
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	make -j "${TERMUX_PKG_MAKE_PROCESSES}" \
		CC="$CC" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
		LFLAGS_STRIP=""
}
