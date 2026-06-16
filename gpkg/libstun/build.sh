TERMUX_PKG_HOMEPAGE=https://github.com/NATTools/stunlib
TERMUX_PKG_DESCRIPTION="STUN and TURN client library for NAT traversal"
TERMUX_PKG_LICENSE="Apache-2.0"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION=0.0.1
TERMUX_PKG_SRCURL=https://github.com/NATTools/stunlib/archive/refs/tags/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_DEPENDS="cmake"
TERMUX_PKG_AUTO_UPDATE=true

termux_step_configure() {
	cmake -B $TERMUX_PKG_BUILDDIR \
		-DCMAKE_INSTALL_PREFIX=$TERMUX_PREFIX \
		-DCMAKE_BUILD_TYPE=Release
}

termux_step_make() {
	cmake --build $TERMUX_PKG_BUILDDIR --parallel $TERMUX_PKG_MAKE_PROCESSES
}

termux_step_make_install() {
	cmake --install $TERMUX_PKG_BUILDDIR --prefix $TERMUX_PREFIX
}
