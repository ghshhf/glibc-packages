TERMUX_PKG_HOMEPAGE=https://www.sqlite.org/
TERMUX_PKG_DESCRIPTION="Command-line shell for SQLite"
TERMUX_PKG_LICENSE="Public Domain"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=3.53.2
TERMUX_PKG_SRCURL=https://github.com/sqlite/sqlite/archive/refs/tags/version-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=7fe547e2870e470f4891ff71f71b701e312d44e4413faabf5958afa0c789d3ca
TERMUX_PKG_DEPENDS="libsqlite-glibc, readline-glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_configure() {
	./configure --prefix="${TERMUX_PREFIX}" \
		--host="${TERMUX_HOST_PLATFORM}" \
		--disable-static \
		--enable-editline
}

termux_step_make_install() {
	make install DESTDIR="${TERMUX_PREFIX}"
	# Remove library and headers (provided by libsqlite-glibc)
	rm -f "${TERMUX_PREFIX}/lib/libsqlite3"*
	rm -rf "${TERMUX_PREFIX}/include"
	rm -f "${TERMUX_PREFIX}/lib/pkgconfig/sqlite3.pc"
}
