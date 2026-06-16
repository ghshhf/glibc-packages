TERMUX_PKG_HOMEPAGE=https://github.com/sharkdp/fd
TERMUX_PKG_DESCRIPTION="Simple, fast and user-friendly alternative to find"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=10.4.2
TERMUX_PKG_SRCURL=https://github.com/sharkdp/fd/archive/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=3a7e027af8c8e91c196ac259c703d78cd55c364706ddafbc66d02c326e57a456
TERMUX_PKG_DEPENDS="gcc-libs-glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	termux_setup_rust
	cargo build --jobs "${TERMUX_PKG_MAKE_PROCESSES}" --target "${CARGO_TARGET_NAME}" --release
}

termux_step_make_install() {
	install -Dm700 "target/${CARGO_TARGET_NAME}/release/fd" "${TERMUX_PREFIX}/bin/fd"
	install -Dm600 "doc/fd.1" "${TERMUX_PREFIX}/share/man/man1/fd.1"
	install -Dm600 "contrib/completion/_fd" "${TERMUX_PREFIX}/share/zsh/site-functions/_fd"
}
