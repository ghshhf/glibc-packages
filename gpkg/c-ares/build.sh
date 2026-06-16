TERMUX_PKG_HOMEPAGE=https://c-ares.org/
TERMUX_PKG_DESCRIPTION="Library for asynchronous DNS requests (including name resolves)"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION=1.34.6
TERMUX_PKG_SRCURL=https://github.com/c-ares/c-ares/archive/refs/tags/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=4358939ff800b13b92f37d5fdda003718101faedfbdee792d6b79ddc1a53d890
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_AUTO_UPDATE=true
TERMUX_PKG_BUILD_DEPENDS="cmake"

termux_step_configure() {
	cmake -B $TERMUX_PKG_BUILDDIR \
		-DCMAKE_INSTALL_PREFIX=$TERMUX_PREFIX \
		-DCMAKE_BUILD_TYPE=Release \
		-DCARES_BUILD_TOOLS=OFF \
		-DCARES_SHARED=ON \
		-DCARES_STATIC=OFF
}

termux_step_make() {
	cmake --build $TERMUX_PKG_BUILDDIR --parallel $TERMUX_PKG_MAKE_PROCESSES
}

termux_step_make_install() {
	cmake --install $TERMUX_PKG_BUILDDIR --prefix $TERMUX_PREFIX
}
