TERMUX_PKG_HOMEPAGE=https://ranger.github.io/
TERMUX_PKG_DESCRIPTION="VIM-inspired filemanager for the console"
TERMUX_PKG_LICENSE="GPL-3.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=1.9.4
TERMUX_PKG_SRCURL=https://pypi.org/packages/source/r/ranger-fm/ranger-fm-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=bee308b636137b9135111fc795a57cdbb95257f2670101042ac3d7747dec32c8
TERMUX_PKG_DEPENDS="python-glibc, ncurses-glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make_install() {
	pip install . --prefix="$TERMUX_PREFIX"
}
