TERMUX_PKG_HOMEPAGE=https://www.zsh.org/
TERMUX_PKG_DESCRIPTION="Shell designed for interactive use, although it is also a powerful scripting language"
TERMUX_PKG_LICENSE="custom"
TERMUX_PKG_LICENSE_FILE="LICENCE"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=5.9
TERMUX_PKG_SRCURL=https://sourceforge.net/projects/zsh/files/zsh/${TERMUX_PKG_VERSION}/zsh-${TERMUX_PKG_VERSION}.tar.xz/download
TERMUX_PKG_SHA256=9b8d1ecedd5b5e81fbf1918e876752a7dd948e05c1a0dba10ab863842d45acd5
TERMUX_PKG_DEPENDS="ncurses-glibc, glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--enable-etcdir=$TERMUX_PREFIX/etc
--enable-fndir=$TERMUX_PREFIX/share/zsh/$TERMUX_PKG_VERSION/functions
--enable-site-fndir=$TERMUX_PREFIX/share/zsh/site-functions
--enable-function-subdirs
--enable-runhelpdir=$TERMUX_PREFIX/share/zsh/$TERMUX_PKG_VERSION/help
--enable-tcsetpgrp
--with-tcsetpgrp
--disable-gdbm
--disable-locale
--disable-pcre
"

termux_step_pre_configure() {
	# Cache variables for features needing runtime detection
	export zsh_cv_shared_environ=yes
	export zsh_cv_sys_dynamic_clash_ok=yes
	export zsh_cv_sys_dynamic_execsyms=yes
	export zsh_cv_sys_dynamic_open=yes
	export zsh_cv_sys_dynamic_strip=yes
	export zsh_cv_func_dlsym_needs_underscore=no
}
