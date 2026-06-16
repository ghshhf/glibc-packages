TERMUX_PKG_HOMEPAGE=https://www.rust-lang.org/
TERMUX_PKG_DESCRIPTION="Systems programming language focused on safety, speed and concurrency"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=1.96.0
TERMUX_PKG_DEPENDS="gcc-libs-glibc, zlib-glibc"
TERMUX_PKG_BUILD_IN_SRC=true
TERMUX_PKG_NO_STATICSPLIT=true
TERMUX_PKG_RM_AFTER_INSTALL="
lib/rustlib/components
lib/rustlib/install.log
lib/rustlib/uninstall.sh
lib/rustlib/rust-installer-version
lib/rustlib/manifest-*
"

case "${TERMUX_ARCH}" in
aarch64)
	RUST_TARGET="aarch64-unknown-linux-gnu"
	TERMUX_PKG_SHA256="371eadcca97062219cbd8593628eb5d2802bc370515d085fedce1b56b2baed57" ;;
arm)
	RUST_TARGET="armv7-unknown-linux-gnueabihf"
	TERMUX_PKG_SHA256="3355f3e75afd40d33c350856a88c39637d711aa0e8541f2aba232da76f2e2b58" ;;
i686)
	RUST_TARGET="i686-unknown-linux-gnu"
	TERMUX_PKG_SHA256="9f1f65f624e4c5e41b45be61d1c7e6cd595b77001f66cb9b830ac0e502d2d301" ;;
x86_64)
	RUST_TARGET="x86_64-unknown-linux-gnu"
	TERMUX_PKG_SHA256="c295047583a56238ea06b43f849f4b877fa12bfd4c7103f8d9a74c94c9c4e108" ;;
esac

TERMUX_PKG_SRCURL="https://static.rust-lang.org/dist/rust-${TERMUX_PKG_VERSION}-${RUST_TARGET}.tar.xz"

termux_step_make_install() {
	./install.sh --prefix="${TERMUX_PREFIX}"
}
