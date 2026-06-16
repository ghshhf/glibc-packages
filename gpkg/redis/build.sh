TERMUX_PKG_HOMEPAGE=https://redis.io/
TERMUX_PKG_DESCRIPTION="In-memory data structure store, used as database, cache and message broker"
TERMUX_PKG_LICENSE="BSD 3-Clause"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=7.2.7
TERMUX_PKG_SRCURL=https://github.com/redis/redis/archive/${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=fdbdee0a19a12e61c2eca46f4162f6d3ede3a01e0baa8a12cee1f35f3391f1e9
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	make MALLOC=libc
}

termux_step_make_install() {
	make PREFIX=$TERMUX_PREFIX install
}
