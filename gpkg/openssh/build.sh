TERMUX_PKG_HOMEPAGE=https://www.openssh.com/
TERMUX_PKG_DESCRIPTION="OpenBSD SSH implementation - secure shell client and server"
TERMUX_PKG_LICENSE="BSD"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=10.2p1
TERMUX_PKG_SRCURL=https://cdn.openbsd.org/pub/OpenBSD/OpenSSH/portable/openssh-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=ccc42c0419937959263fa1dbd16dafc18c56b984c03562d2937ce56a60f798b2
TERMUX_PKG_DEPENDS="zlib-glibc, openssl-glibc, glibc"
TERMUX_PKG_BUILD_IN_SRC=true
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--with-ssl-dir=$TERMUX_PREFIX
--with-zlib=$TERMUX_PREFIX
--disable-pam
--sysconfdir=$TERMUX_PREFIX/etc/ssh
--localstatedir=$TERMUX_PREFIX/var
"
