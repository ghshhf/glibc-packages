TERMUX_PKG_HOMEPAGE=https://man-db.gitlab.io/man-db/
TERMUX_PKG_DESCRIPTION="An implementation of the standard Unix documentation system"
TERMUX_PKG_LICENSE="GPL-2.0-or-later"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=2.13.1
TERMUX_PKG_SRCURL=https://gitlab.com/man-db/man-db/-/archive/${TERMUX_PKG_VERSION}/man-db-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=8d14b10f8e295fa40d6a1975817ead1d80c2f085794d2d9e2e1179fa0cb54132
TERMUX_PKG_DEPENDS="libpipeline-glibc, gdbm-glibc, less-glibc, zlib-glibc, glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--disable-setuid
--disable-cache-owner
--disable-nls
--disable-seccomp
--without-groff
"

termux_step_pre_configure() {
	autoreconf -fi
}
