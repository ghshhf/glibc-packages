TERMUX_PKG_HOMEPAGE=https://htop.dev/
TERMUX_PKG_DESCRIPTION="Interactive process viewer for Unix-like systems"
TERMUX_PKG_LICENSE="GPL-2.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=3.5.1
TERMUX_PKG_SRCURL=https://github.com/htop-dev/htop/archive/refs/tags/${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=dfc4a09845e9bc86f466a722e62b8f87d59028ff39689077ff2257a6a605061d
TERMUX_PKG_DEPENDS="ncurses-glibc"
TERMUX_PKG_BUILD_DEPENDS="ncurses-glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--enable-unicode
--enable-affinity
--enable-capabilities
"

termux_step_pre_configure() {
	autoreconf -fi
}
