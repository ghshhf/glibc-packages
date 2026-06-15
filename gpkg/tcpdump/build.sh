TERMUX_PKG_HOMEPAGE=https://www.tcpdump.org/
TERMUX_PKG_DESCRIPTION="Powerful command-line packet analyzer"
TERMUX_PKG_LICENSE="BSD"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=4.99.6
TERMUX_PKG_SRCURL=https://www.tcpdump.org/release/tcpdump-${TERMUX_PKG_VERSION}.tar.xz
TERMUX_PKG_SHA256=40a8cefd45f0d2a06827e6658efb830d484868c449ad80f7efb33516af44f3da
TERMUX_PKG_DEPENDS="libpcap, openssl, glibc"
TERMUX_PKG_BUILD_IN_SRC=true
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--with-libpcap=$TERMUX_PREFIX
"

termux_step_pre_configure() {
	autoreconf -fi
}
