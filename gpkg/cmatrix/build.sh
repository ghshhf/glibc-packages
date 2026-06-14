TERMUX_PKG_HOMEPAGE=https://github.com/abishekvashok/cmatrix
TERMUX_PKG_DESCRIPTION="Matrix-style digital rain terminal screensaver"
TERMUX_PKG_LICENSE="GPL-3.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=2.0
TERMUX_PKG_SRCURL=https://github.com/abishekvashok/cmatrix/archive/refs/tags/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=ad93ba39acd383696ab6a9ebbed1259ecf2d3cf9f49d6b97038c66f80749e99a
TERMUX_PKG_DEPENDS="ncurses-glibc"

termux_step_pre_configure() {
	autoreconf -fi
}
