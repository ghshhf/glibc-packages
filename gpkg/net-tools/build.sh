TERMUX_PKG_HOMEPAGE=https://sourceforge.net/projects/net-tools/
TERMUX_PKG_DESCRIPTION="Net-tools provide ifconfig, netstat, route, arp and other network tools"
TERMUX_PKG_LICENSE="GPL-2.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=2.10
TERMUX_PKG_SRCURL=https://deb.debian.org/debian/pool/main/n/net-tools/net-tools_${TERMUX_PKG_VERSION}.orig.tar.xz
TERMUX_PKG_SHA256=b262435a5241e89bfa51c3cabd5133753952f7a7b7b93f32e08cb9d96f580d69
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_configure() {
	cd "$TERMUX_PKG_SRCDIR"
	# Generate config.h non-interactively
	yes "" | make config
}
