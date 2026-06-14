TERMUX_PKG_HOMEPAGE=http://www.dest-unreach.org/socat/
TERMUX_PKG_DESCRIPTION="Multipurpose relay tool (SOcket CAT)"
TERMUX_PKG_LICENSE="GPL-2.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=1.8.1.1
TERMUX_PKG_SRCURL=http://www.dest-unreach.org/socat/download/socat-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=f68b602c80e94b4b7498d74ec408785536fe33534b39467977a82ab2f7f01ddb
TERMUX_PKG_DEPENDS="openssl-glibc, readline-glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--disable-fips
"
