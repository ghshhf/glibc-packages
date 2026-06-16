TERMUX_PKG_HOMEPAGE=https://infozip.sourceforge.net/UnZip.html
TERMUX_PKG_DESCRIPTION="Tools for working with ZIP archives"
TERMUX_PKG_LICENSE="BSD"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=6.0
TERMUX_PKG_SRCURL=https://deb.debian.org/debian/pool/main/u/unzip/unzip_${TERMUX_PKG_VERSION}.orig.tar.gz
TERMUX_PKG_SHA256=036d96991646d0449ed0aa952e4fbe21b476ce994abc276e49d30e686708bd37
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
	install -Dm700 "unzip" "${TERMUX_PREFIX}/bin/unzip"
	install -Dm700 "funzip" "${TERMUX_PREFIX}/bin/funzip"
	install -Dm700 "unzipsfx" "${TERMUX_PREFIX}/bin/unzipsfx"
	install -Dm700 "zipgrep" "${TERMUX_PREFIX}/bin/zipgrep"
	install -Dm700 "zipinfo" "${TERMUX_PREFIX}/bin/zipinfo"
	for manpage in man/*.1; do
		install -Dm600 "$manpage" "${TERMUX_PREFIX}/share/man/man1/$(basename "$manpage")"
	done
}
