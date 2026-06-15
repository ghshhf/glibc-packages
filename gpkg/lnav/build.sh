TERMUX_PKG_HOMEPAGE=https://lnav.org/
TERMUX_PKG_DESCRIPTION="Log file navigator - curses-based log viewer"
TERMUX_PKG_LICENSE="BSD-2-Clause"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=0.14.0
TERMUX_PKG_SRCURL=https://github.com/tstack/lnav/releases/download/v${TERMUX_PKG_VERSION}/lnav-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=0fd591a2e0488a06b3b44d7b384d3d7c6852d68607efc16ef4dec7a6ed054eea
TERMUX_PKG_DEPENDS="pcre2-glibc, sqlite3-glibc, ncurses-glibc, readline-glibc, zlib-glibc, libbz2-glibc, curl-glibc, glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--without-included-sqlite
--with-ncurses=$TERMUX_PREFIX
"

termux_step_pre_configure() {
	autoreconf -fi
}
