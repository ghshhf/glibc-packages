TERMUX_PKG_HOMEPAGE=https://tmux.github.io/
TERMUX_PKG_DESCRIPTION="Terminal multiplexer that enables multiple terminal sessions within a single window"
TERMUX_PKG_LICENSE="ISC"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=3.6b
TERMUX_PKG_SRCURL=https://github.com/tmux/tmux/archive/refs/tags/${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=e71f6a6b87621c1a93b39f7e04e62caac61b4455e5f4cb3ffd1042536b8c47c5
TERMUX_PKG_DEPENDS="libevent-glibc, ncurses-glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--enable-static
"

termux_step_pre_configure() {
	autoreconf -fi
}
