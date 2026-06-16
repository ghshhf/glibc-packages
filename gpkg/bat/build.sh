TERMUX_PKG_HOMEPAGE=https://github.com/sharkdp/bat
TERMUX_PKG_DESCRIPTION="A cat(1) clone with wings"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=0.26.1
TERMUX_PKG_SRCURL=https://github.com/sharkdp/bat/archive/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=4474de87e084953eefc1120cf905a79f72bbbf85091e30cf37c9214eafcaa9c9
TERMUX_PKG_DEPENDS="gcc-libs-glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	termux_setup_rust
	cargo build --jobs "${TERMUX_PKG_MAKE_PROCESSES}" --target "${CARGO_TARGET_NAME}" --release
}

termux_step_make_install() {
	install -Dm700 "target/${CARGO_TARGET_NAME}/release/bat" "${TERMUX_PREFIX}/bin/bat"
	sed 's/{{PROJECT_EXECUTABLE}}/bat/g; s/{{PROJECT_EXECUTABLE_UPPERCASE}}/BAT/g' \
		"assets/manual/bat.1.in" > "bat.1"
	install -Dm600 "bat.1" "${TERMUX_PREFIX}/share/man/man1/bat.1"
	install -Dm600 "assets/completions/bat.bash.in" "${TERMUX_PREFIX}/share/bash-completion/completions/bat"
	install -Dm600 "assets/completions/bat.zsh.in" "${TERMUX_PREFIX}/share/zsh/site-functions/_bat"
	install -Dm600 "assets/completions/bat.fish.in" "${TERMUX_PREFIX}/share/fish/vendor_completions.d/bat.fish"
}
