TERMUX_PKG_HOMEPAGE=https://www.tcpdump.org/
TERMUX_PKG_DESCRIPTION="System interface for user-level packet capture"
TERMUX_PKG_LICENSE="BSD"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=1.10.5
TERMUX_PKG_SRCURL=https://www.tcpdump.org/release/libpcap-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=37ced90a19a302a7f32e458224a00c365c117905c2cd35ac544b6880a81488f0
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="--disable-shared"

termux_step_pre_configure() {
	autoreconf -fi
}
