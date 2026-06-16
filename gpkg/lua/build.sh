TERMUX_PKG_HOMEPAGE=https://www.lua.org/
TERMUX_PKG_DESCRIPTION="Powerful lightweight programming language designed for extending applications"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=5.5.0
TERMUX_PKG_SRCURL=https://www.lua.org/ftp/lua-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=57ccc32bbbd005cab75bcc52444052535af691789dba2b9016d5c50640d68b3d
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_configure() {
	:
}

termux_step_make() {
	make -j "${TERMUX_PKG_MAKE_PROCESSES}" \
		CC="$CC" CFLAGS="$CFLAGS -fPIC" LDFLAGS="$LDFLAGS" \
		generic
}

termux_step_make_install() {
	make install INSTALL_TOP="${TERMUX_PREFIX}"
}
