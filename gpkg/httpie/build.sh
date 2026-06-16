TERMUX_PKG_HOMEPAGE=https://httpie.io/
TERMUX_PKG_DESCRIPTION="User-friendly HTTP client for the command line"
TERMUX_PKG_LICENSE="BSD"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=3.2.4
TERMUX_PKG_SRCURL=https://pypi.org/packages/source/h/httpie/httpie-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=302ad436c3dc14fd0d1b19d4572ef8d62b146bcd94b505f3c2521f701e2e7a2a
TERMUX_PKG_DEPENDS="python-glibc, python-pip"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make_install() {
	pip install . --prefix="$TERMUX_PREFIX"
}
