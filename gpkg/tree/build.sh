TERMUX_PKG_HOMEPAGE=https://oldhome.schmorp.de/martin/tree/
TERMUX_PKG_DESCRIPTION="Recursive directory listing command that produces a depth-indented listing of files"
TERMUX_PKG_LICENSE="GPL-2.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=2.1.3
TERMUX_PKG_SRCURL=https://github.com/Old-Man-Programmer/tree/archive/refs/tags/${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=3ffe2c8bb21194b088ad1e723f0cf340dd434453c5ff9af6a38e0d47e0c2723b
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	make \
		CC="$CC" \
		CFLAGS="$CFLAGS $CPPFLAGS -Os" \
		LDFLAGS="$LDFLAGS"
}

termux_step_make_install() {
	install -Dm700 tree "$TERMUX_PREFIX/bin/tree"
	install -Dm644 doc/tree.1 "$TERMUX_PREFIX/share/man/man1/tree.1"
}
