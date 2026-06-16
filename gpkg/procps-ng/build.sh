TERMUX_PKG_HOMEPAGE=https://gitlab.com/procps-ng/procps
TERMUX_PKG_DESCRIPTION="Utilities that give information about processes using the /proc filesystem"
TERMUX_PKG_LICENSE="GPL-2.0, LGPL-2.1"
TERMUX_PKG_LICENSE_FILE="COPYING, COPYING.LIB"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=4.0.5
TERMUX_PKG_SRCURL=https://gitlab.com/procps-ng/procps/-/archive/v${TERMUX_PKG_VERSION}/procps-v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=2c6d7ed9f2acde1d4dd4602c6172fe56eff86953fe8639bd633dbd22cc18f5db
TERMUX_PKG_DEPENDS="ncurses-glibc, glibc"
TERMUX_PKG_ESSENTIAL=true
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--enable-sigwinch
--disable-pidof
--disable-modern-top
--with-ncurses
ac_cv_func_realloc_0_nonnull=yes
ac_cv_func_malloc_0_nonnull=yes
"

termux_step_pre_configure() {
	autoreconf -fi
}
