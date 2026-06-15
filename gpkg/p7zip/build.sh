TERMUX_PKG_HOMEPAGE=https://p7zip.sourceforge.net/
TERMUX_PKG_DESCRIPTION="7-Zip file archiver with high compression ratio"
TERMUX_PKG_LICENSE="LGPL-2.1+ AND BSD-3-Clause AND License-7-Zip"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=16.02
TERMUX_PKG_SRCURL=https://sourceforge.net/projects/p7zip/files/p7zip/${TERMUX_PKG_VERSION}/p7zip_${TERMUX_PKG_VERSION}_src_all.tar.bz2
TERMUX_PKG_SHA256=5eb20ac0e2944f6cb9c2d51dd6c4518941c185347d4089ea89087ffdd6e2341f
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	make CC="$CC" CXX="$CXX" CFLAGS="${CFLAGS} ${CPPFLAGS}" CXXFLAGS="${CXXFLAGS} ${CPPFLAGS}" LDFLAGS="${LDFLAGS}"
}

termux_step_make_install() {
	DEST_HOME="$TERMUX_PREFIX" ./install.sh
}
