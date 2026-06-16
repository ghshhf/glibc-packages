TERMUX_PKG_HOMEPAGE=https://midnight-commander.org/
TERMUX_PKG_DESCRIPTION="Midnight Commander - visual file manager"
TERMUX_PKG_LICENSE="GPL-3.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=4.8.33
TERMUX_PKG_SRCURL=https://ftp.osuosl.org/pub/midnightcommander/mc-${TERMUX_PKG_VERSION}.tar.xz
TERMUX_PKG_SHA256=cae149d42f844e5185d8c81d7db3913a8fa214c65f852200a9d896b468af164c
TERMUX_PKG_DEPENDS="ncurses-glibc, glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--without-x
--with-ncurses=$TERMUX_PREFIX
"

termux_step_pre_configure() {
	autoreconf -fi
}
