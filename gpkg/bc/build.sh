TERMUX_PKG_HOMEPAGE=https://git.gavinhoward.com/gavin/bc
TERMUX_PKG_DESCRIPTION="Arbitrary precision numeric processing language"
TERMUX_PKG_LICENSE="BSD 2-Clause"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=7.0.3
TERMUX_PKG_SRCURL=https://github.com/gavinhoward/bc/releases/download/${TERMUX_PKG_VERSION}/bc-${TERMUX_PKG_VERSION}.tar.xz
TERMUX_PKG_SHA256=91eb74caed0ee6655b669711a4f350c25579778694df248e28363318e03c7fc4
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--disable-dynamic
"
