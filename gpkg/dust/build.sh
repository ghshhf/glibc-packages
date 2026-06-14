TERMUX_PKG_HOMEPAGE=https://github.com/bootandy/dust
TERMUX_PKG_DESCRIPTION="A more intuitive version of du"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=1.2.4
TERMUX_PKG_SRCURL=https://github.com/bootandy/dust/archive/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=2f6768534bd01727234e67f1dd3754c9547aa18c715f6ee52094e881ebac50e3
TERMUX_PKG_DEPENDS="gcc-libs-glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	termux_setup_rust
	cargo build --jobs "${TERMUX_PKG_MAKE_PROCESSES}" --target "${CARGO_TARGET_NAME}" --release
}

termux_step_make_install() {
	install -Dm700 "target/${CARGO_TARGET_NAME}/release/dust" "${TERMUX_PREFIX}/bin/dust"
	install -Dm600 "completions/dust.bash" "${TERMUX_PREFIX}/share/bash-completion/completions/dust"
	install -Dm600 "completions/_dust" "${TERMUX_PREFIX}/share/zsh/site-functions/_dust"
	install -Dm600 "completions/dust.fish" "${TERMUX_PREFIX}/share/fish/vendor_completions.d/dust.fish"
	install -Dm600 "man-page/dust.1" "${TERMUX_PREFIX}/share/man/man1/dust.1"
}
