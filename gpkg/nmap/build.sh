TERMUX_PKG_HOMEPAGE=https://nmap.org/
TERMUX_PKG_DESCRIPTION="Network exploration tool and security scanner"
TERMUX_PKG_LICENSE="GPL-2.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=7.99
TERMUX_PKG_SRCURL=https://nmap.org/dist/nmap-${TERMUX_PKG_VERSION}.tar.bz2
TERMUX_PKG_SHA256=df512492ffd108e53a27a06f26d8635bbe89e0e569455dc8ffef058c035d51b2
TERMUX_PKG_DEPENDS="libpcap, openssl, zlib, glibc"
TERMUX_PKG_BUILD_IN_SRC=true
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--without-liblua
--without-zenmap
--with-libpcap=$TERMUX_PREFIX
--with-openssl=$TERMUX_PREFIX
--with-zlib=$TERMUX_PREFIX
"
