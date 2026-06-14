TERMUX_PKG_HOMEPAGE=https://www.gnu.org/software/bison/
TERMUX_PKG_DESCRIPTION="General-purpose parser generator"
TERMUX_PKG_LICENSE="GPL-3.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=3.8.2
TERMUX_PKG_SRCURL=https://mirrors.kernel.org/gnu/bison/bison-${TERMUX_PKG_VERSION}.tar.xz
TERMUX_PKG_SHA256=9bba0214ccf7f1079c5d59210045227bcf619519840ebfa80cd3849cff5a5bf2
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--enable-relocatable
"
