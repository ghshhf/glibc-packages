TERMUX_PKG_HOMEPAGE=https://github.com/dandavison/delta
TERMUX_PKG_DESCRIPTION="A syntax-highlighting pager for git, diff, and grep output"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=0.19.2
TERMUX_PKG_SRCURL=https://github.com/dandavison/delta/archive/${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=f59b86f8c8dda4d76a3ba34b8553777a20c3b461646917d8e480fac6531bba9f
TERMUX_PKG_DEPENDS="gcc-libs-glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	termux_setup_rust
	cargo build --jobs "${TERMUX_PKG_MAKE_PROCESSES}" --target "${CARGO_TARGET_NAME}" --release
}

termux_step_make_install() {
	install -Dm700 "target/${CARGO_TARGET_NAME}/release/delta" "${TERMUX_PREFIX}/bin/delta"
	install -Dm600 "etc/completion/completion.bash" "${TERMUX_PREFIX}/share/bash-completion/completions/delta"
	install -Dm600 "etc/completion/completion.zsh" "${TERMUX_PREFIX}/share/zsh/site-functions/_delta"
	install -Dm600 "etc/completion/completion.fish" "${TERMUX_PREFIX}/share/fish/vendor_completions.d/delta.fish"
}
