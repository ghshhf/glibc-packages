TERMUX_PKG_HOMEPAGE=https://www.ruby-lang.org/
TERMUX_PKG_DESCRIPTION="Dynamic programming language with a focus on simplicity and productivity"
TERMUX_PKG_LICENSE="BSD 2-Clause"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION="3.4.1"
TERMUX_PKG_SRCURL=https://cache.ruby-lang.org/pub/ruby/3.4/ruby-${TERMUX_PKG_VERSION}.tar.xz
TERMUX_PKG_SHA256=018d59ffb52be3c0a6d847e22d3fd7a2c52d0ddfee249d3517a0c8c6dbfa70af
TERMUX_PKG_DEPENDS="glibc, gcc-libs-glibc, libffi-glibc, libgmp-glibc, readline-glibc, openssl-glibc, zlib-glibc"
TERMUX_PKG_RECOMMENDS="make-glibc, pkg-config-glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--enable-rubygems
--disable-install-doc
--disable-install-rdoc
"
