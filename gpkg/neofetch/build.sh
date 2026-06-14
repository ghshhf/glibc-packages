TERMUX_PKG_HOMEPAGE=https://github.com/dylanaraps/neofetch
TERMUX_PKG_DESCRIPTION="A command-line system information tool written in bash 3.2+"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=7.1.0
TERMUX_PKG_SRCURL=https://github.com/dylanaraps/neofetch/archive/${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=58a95e6b714e41efc804eca389a223309169b2def35e57fa934482a6b47c27e7
TERMUX_PKG_DEPENDS="glibc, bash-glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	: # No compilation needed - neofetch is a pure bash script.
}

termux_step_make_install() {
	make PREFIX=$TERMUX_PREFIX install
}
