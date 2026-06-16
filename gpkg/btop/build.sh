TERMUX_PKG_HOMEPAGE=https://github.com/aristocratos/btop
TERMUX_PKG_DESCRIPTION="Monitor of resources, with a playful CLI that shows usage and stats"
TERMUX_PKG_LICENSE="Apache-2.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=1.4.0
TERMUX_PKG_SRCURL=https://github.com/aristocratos/btop/archive/refs/tags/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=ac0d2371bf69d5136de7e9470c6fb286cbee2e16b4c7a6d2cd48a14796e86650
TERMUX_PKG_DEPENDS="ncurses-glibc, glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	make CXX="${CXX}" CXXFLAGS="${CXXFLAGS} ${CPPFLAGS}" LDFLAGS="${LDFLAGS}"
}

termux_step_make_install() {
	make DESTDIR="$TERMUX_PREFIX" PREFIX="" install
}
