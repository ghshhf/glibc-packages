TERMUX_PKG_HOMEPAGE=https://memcached.org
TERMUX_PKG_DESCRIPTION="Distributed memory object caching system"
TERMUX_PKG_LICENSE="BSD"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=1.6.36
TERMUX_PKG_SRCURL=https://memcached.org/files/memcached-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=2eeeadbe8ab6c6848654594733e9da4875be7fc870a2c28f651f5bdb4053d041
TERMUX_PKG_DEPENDS="libevent-glibc, glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--with-libevent=$TERMUX_PREFIX
--disable-sasl
"

termux_step_pre_configure() {
	export ac_cv_libevent_dir="$TERMUX_PREFIX"
}
