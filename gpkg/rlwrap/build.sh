TERMUX_PKG_HOMEPAGE=https://github.com/hanslub42/rlwrap
TERMUX_PKG_DESCRIPTION="Readline wrapper - add command history and line editing to any command"
TERMUX_PKG_LICENSE="GPL-2.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=0.48
TERMUX_PKG_SRCURL=https://github.com/hanslub42/rlwrap/archive/refs/tags/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=b2721b1c0147aaafc98e6a31d875316ba032ad336bec7f2a8bc538f9e3c6db60
TERMUX_PKG_DEPENDS="readline-glibc, ncurses-glibc"

termux_step_pre_configure() {
	autoreconf -fi
}
