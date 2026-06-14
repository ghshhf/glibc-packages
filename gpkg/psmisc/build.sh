TERMUX_PKG_HOMEPAGE=https://gitlab.com/psmisc/psmisc
TERMUX_PKG_DESCRIPTION="Miscellaneous utilities that use the proc filesystem"
TERMUX_PKG_LICENSE="GPL-2.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=23.7
TERMUX_PKG_SRCURL=https://gitlab.com/psmisc/psmisc/-/archive/v${TERMUX_PKG_VERSION}/psmisc-v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=8f2526ce7ac6ef4976454cd63095fa10e467ef745cf33dc4f91df0bd7b10b905
TERMUX_PKG_DEPENDS="ncurses-glibc, glibc"

termux_step_pre_configure() {
	autoreconf -fi
}
