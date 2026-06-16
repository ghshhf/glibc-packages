TERMUX_PKG_HOMEPAGE=https://github.com/ggml-org/ggml
TERMUX_PKG_DESCRIPTION="Lightweight tensor computation library (llama.cpp backend)"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION=0.15.1
TERMUX_PKG_SRCURL=https://github.com/ggml-org/ggml/archive/refs/tags/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=
TERMUX_PKG_DEPENDS="glibc, gcc-libs-glibc"
TERMUX_PKG_BUILD_DEPENDS="cmake"
TERMUX_PKG_AUTO_UPDATE=true
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
-DGGML_BUILD_TESTS=OFF
-DGGML_BUILD_EXAMPLES=OFF
-DBUILD_SHARED_LIBS=ON
"

termux_step_configure() {
	cmake -B $TERMUX_PKG_BUILDDIR \
		-DCMAKE_INSTALL_PREFIX=$TERMUX_PREFIX \
		-DCMAKE_BUILD_TYPE=Release \
		$TERMUX_PKG_EXTRA_CONFIGURE_ARGS
}

termux_step_make() {
	cmake --build $TERMUX_PKG_BUILDDIR --parallel $TERMUX_PKG_MAKE_PROCESSES
}

termux_step_make_install() {
	cmake --install $TERMUX_PKG_BUILDDIR --prefix $TERMUX_PREFIX
}
