TERMUX_PKG_HOMEPAGE=https://github.com/eza-community/eza
TERMUX_PKG_DESCRIPTION="A modern, maintained replacement for ls"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=0.23.4
TERMUX_PKG_SRCURL=https://github.com/eza-community/eza/archive/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=9fbcad518b8a2095206ac385329ca62d216bf9fdc652dde2d082fcb37c309635
TERMUX_PKG_DEPENDS="gcc-libs-glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	termux_setup_rust
	cargo build --jobs "${TERMUX_PKG_MAKE_PROCESSES}" --target "${CARGO_TARGET_NAME}" --release
}

termux_step_make_install() {
	install -Dm700 "target/${CARGO_TARGET_NAME}/release/eza" "${TERMUX_PREFIX}/bin/eza"
	install -Dm600 "completions/bash/eza" "${TERMUX_PREFIX}/share/bash-completion/completions/eza"
	install -Dm600 "completions/zsh/_eza" "${TERMUX_PREFIX}/share/zsh/site-functions/_eza"
	install -Dm600 "completions/fish/eza.fish" "${TERMUX_PREFIX}/share/fish/vendor_completions.d/eza.fish"
}
