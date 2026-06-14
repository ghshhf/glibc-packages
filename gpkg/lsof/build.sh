TERMUX_PKG_HOMEPAGE=https://github.com/lsof-org/lsof
TERMUX_PKG_DESCRIPTION="List open files"
TERMUX_PKG_LICENSE="BSD-2-Clause"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=4.91
TERMUX_PKG_SRCURL=https://www.mirrorservice.org/sites/lsof.itap.purdue.edu/pub/tools/unix/lsof/lsof_${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=9fd672b1c8425fac2fa67fa0477b990987268b90ff36d5f016dae57be0d6b52e
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_configure() {
	cd "$TERMUX_PKG_SRCDIR"
	./Configure -n linux
}

termux_step_make() {
	cd "$TERMUX_PKG_SRCDIR/lib"
	make -j "${TERMUX_PKG_MAKE_PROCESSES}"
	cd "$TERMUX_PKG_SRCDIR"
	make -j "${TERMUX_PKG_MAKE_PROCESSES}"
}

termux_step_make_install() {
	install -Dm700 "lsof" "${TERMUX_PREFIX}/bin/lsof"
	install -Dm600 "Lsof.8" "${TERMUX_PREFIX}/share/man/man8/lsof.8"
}
