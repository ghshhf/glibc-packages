TERMUX_PKG_HOMEPAGE=https://www.gnu.org/software/gawk/
TERMUX_PKG_DESCRIPTION="GNU awk pattern-matching language"
TERMUX_PKG_LICENSE="GPL-3.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=5.3.2
TERMUX_PKG_SRCURL=https://ftp.gnu.org/gnu/gawk/gawk-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=8639a1a88fb411a1be02663739d03e902a6d313b5c6fe024d0bfeb3341a19a11
TERMUX_PKG_DEPENDS="readline-glibc, libgmp-glibc, libmpfr-glibc, glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--with-readline
--with-mpfr
--disable-extensions
"

termux_step_pre_configure() {
	autoreconf -fi 2>/dev/null || true
}

termux_step_post_make_install() {
	ln -sf gawk $TERMUX_PREFIX/bin/awk
	install -Dm644 doc/gawk.1 $TERMUX_PREFIX/share/man/man1/gawk.1
	ln -sf gawk.1 $TERMUX_PREFIX/share/man/man1/awk.1
}
