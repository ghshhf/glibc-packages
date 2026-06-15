TERMUX_PKG_HOMEPAGE=https://libpipeline.gitlab.io/libpipeline/
TERMUX_PKG_DESCRIPTION="C library for manipulating pipelines of subprocesses"
TERMUX_PKG_LICENSE="BSD"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=1.5.8
TERMUX_PKG_SRCURL=https://gitlab.com/libpipeline/libpipeline/-/archive/${TERMUX_PKG_VERSION}/libpipeline-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=ada309694ee87ed564a798e7f6ceab70f577f922e0c0e2fe81018ddfe218ca51
TERMUX_PKG_DEPENDS="glibc"

termux_step_pre_configure() {
	autoreconf -fi
}
