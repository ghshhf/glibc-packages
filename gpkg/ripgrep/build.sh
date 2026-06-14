TERMUX_PKG_HOMEPAGE=https://github.com/BurntSushi/ripgrep
TERMUX_PKG_DESCRIPTION="Recursively searches directories for a regex pattern"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=15.0.0
TERMUX_PKG_SRCURL=https://github.com/BurntSushi/ripgrep/archive/${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=e6b2d35ff79c3327edc0c92a29dc4bb74e82d8ee4b8156fb98e767678716be7a
TERMUX_PKG_DEPENDS="gcc-libs-glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	termux_setup_rust
	cargo build --jobs "${TERMUX_PKG_MAKE_PROCESSES}" --target "${CARGO_TARGET_NAME}" --release
}

termux_step_make_install() {
	install -Dm700 "target/${CARGO_TARGET_NAME}/release/rg" "${TERMUX_PREFIX}/bin/rg"
	install -Dm600 "doc/rg.1" "${TERMUX_PREFIX}/share/man/man1/rg.1"
	install -Dm600 "complete/_rg" "${TERMUX_PREFIX}/share/zsh/site-functions/_rg"
	install -Dm600 "complete/rg.bash" "${TERMUX_PREFIX}/share/bash-completion/completions/rg"
}
