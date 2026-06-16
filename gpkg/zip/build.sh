TERMUX_PKG_HOMEPAGE=https://infozip.sourceforge.net/Zip.html
TERMUX_PKG_DESCRIPTION="Tools for working with ZIP archives"
TERMUX_PKG_LICENSE="BSD"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=3.0
TERMUX_PKG_SRCURL=https://deb.debian.org/debian/pool/main/z/zip/zip_${TERMUX_PKG_VERSION}.orig.tar.gz
TERMUX_PKG_SHA256=f0e8bb1f9b7eb0b01285495a2699df3a4b766784c1765a8f1aeedf63c0806369
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_configure() {
	cd unix
	./configure "${CC}" "${CFLAGS} ${CPPFLAGS}"
}

termux_step_make() {
	cd "$TERMUX_PKG_SRCDIR"
	make -f unix/Makefile generic CC="$CC" CF="$CFLAGS $CPPFLAGS -Os" LDFLAGS="$LDFLAGS"
}

termux_step_make_install() {
	install -Dm700 "zip" "${TERMUX_PREFIX}/bin/zip"
	install -Dm700 "zipnote" "${TERMUX_PREFIX}/bin/zipnote"
	install -Dm700 "zipsplit" "${TERMUX_PREFIX}/bin/zipsplit"
	install -Dm700 "zipcloak" "${TERMUX_PREFIX}/bin/zipcloak"
	for manpage in man/*.1; do
		install -Dm600 "$manpage" "${TERMUX_PREFIX}/share/man/man1/$(basename "$manpage")"
	done
}
