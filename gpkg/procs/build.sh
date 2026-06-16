TERMUX_PKG_HOMEPAGE=https://github.com/dalance/procs
TERMUX_PKG_DESCRIPTION="A modern replacement for ps"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=0.14.11
TERMUX_PKG_SRCURL=https://github.com/dalance/procs/archive/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=3d6b3561ce05362a092ea8488458f552d6636d3a280290e21f841c432cadf91a
TERMUX_PKG_DEPENDS="gcc-libs-glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	termux_setup_rust
	cargo build --jobs "${TERMUX_PKG_MAKE_PROCESSES}" --target "${CARGO_TARGET_NAME}" --release
}

termux_step_make_install() {
	install -Dm700 "target/${CARGO_TARGET_NAME}/release/procs" "${TERMUX_PREFIX}/bin/procs"
}
