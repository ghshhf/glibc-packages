TERMUX_PKG_HOMEPAGE=https://curl.se/
TERMUX_PKG_DESCRIPTION="Command line tool for transferring data with URL syntax"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=8.20.0
TERMUX_PKG_SRCURL=https://github.com/curl/curl/releases/download/curl-${TERMUX_PKG_VERSION//./_}/curl-${TERMUX_PKG_VERSION}.tar.xz
TERMUX_PKG_SHA256=63fe2dc148ba0ceae89922ef838f7e5c946272c2e78b7c59fab4b79d3ce2b896
TERMUX_PKG_DEPENDS="libcurl-glibc, libnghttp2-glibc, libssh2-glibc, openssl-glibc (>= 3.0.3), krb5-glibc, brotli-glibc, libpsl-glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--with-ca-bundle=$TERMUX_PREFIX/etc/ssl/certs/ca-certificates.crt
--with-ca-path=$TERMUX_PREFIX/etc/ssl/certs
--disable-ldap
--disable-ldaps
--disable-manual
--enable-ipv6
--enable-threaded-resolver
--with-libssh2
--with-openssl
--enable-versioned-symbols
--with-random=/dev/urandom
"

termux_step_pre_configure() {
	sed -i 's/cross_compiling=no/cross_compiling=yes/' ${TERMUX_PKG_SRCDIR}/configure
}

termux_step_make_install() {
	# Only install the curl binary and man page (library is provided by libcurl-glibc)
	install -Dm700 "src/curl" "${TERMUX_PREFIX}/bin/curl"
	install -Dm600 "docs/curl.1" "${TERMUX_PREFIX}/share/man/man1/curl.1"
}
