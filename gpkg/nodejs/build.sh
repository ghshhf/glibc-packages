TERMUX_PKG_HOMEPAGE=https://nodejs.org/
TERMUX_PKG_DESCRIPTION="Open Source, cross-platform JavaScript runtime environment"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION=26.3.0
TERMUX_PKG_SRCURL=https://nodejs.org/dist/v${TERMUX_PKG_VERSION}/node-v${TERMUX_PKG_VERSION}.tar.xz
TERMUX_PKG_SHA256=319ad5d7d20cc622e55eb75b9f1a2546b77a08bd462b67030d0c89316c2c2349
TERMUX_PKG_AUTO_UPDATE=false
TERMUX_PKG_DEPENDS="glibc, gcc-libs-glibc, openssl-glibc, libicu-glibc, libsqlite-glibc, zlib-glibc, libffi-glibc, c-ares-glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_configure() {
	./configure \
		--prefix=$TERMUX_PREFIX \
		--shared-openssl \
		--shared-zlib \
		--with-intl=system-icu \
		--experimental-http3
}

termux_step_make() {
	make -j${TERMUX_PKG_MAKE_PROCESSES}
}

termux_step_make_install() {
	make install
}
