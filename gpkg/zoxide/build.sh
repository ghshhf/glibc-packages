TERMUX_PKG_HOMEPAGE=https://github.com/ajeetdsouza/zoxide
TERMUX_PKG_DESCRIPTION="A smarter cd command for your terminal"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=0.9.9
TERMUX_PKG_SRCURL=https://github.com/ajeetdsouza/zoxide/archive/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=eddc76e94db58567503a3893ecac77c572f427f3a4eabdfc762f6773abf12c63
TERMUX_PKG_DEPENDS="gcc-libs-glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	termux_setup_rust
	cargo build --jobs "${TERMUX_PKG_MAKE_PROCESSES}" --target "${CARGO_TARGET_NAME}" --release
}

termux_step_make_install() {
	install -Dm700 "target/${CARGO_TARGET_NAME}/release/zoxide" "${TERMUX_PREFIX}/bin/zoxide"
	install -Dm600 "contrib/completions/zoxide.bash" "${TERMUX_PREFIX}/share/bash-completion/completions/zoxide"
	install -Dm600 "contrib/completions/_zoxide" "${TERMUX_PREFIX}/share/zsh/site-functions/_zoxide"
	install -Dm600 "contrib/completions/zoxide.fish" "${TERMUX_PREFIX}/share/fish/vendor_completions.d/zoxide.fish"
	for manpage in man/man1/*.1; do
		install -Dm600 "$manpage" "${TERMUX_PREFIX}/share/man/man1/$(basename "$manpage")"
	done
}
