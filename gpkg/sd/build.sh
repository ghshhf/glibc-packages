TERMUX_PKG_HOMEPAGE=https://github.com/chmln/sd
TERMUX_PKG_DESCRIPTION="Intuitive find & replace CLI, sed alternative"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=1.0.0
TERMUX_PKG_SRCURL=https://github.com/chmln/sd/archive/refs/tags/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=2adc1dec0d2c63cbffa94204b212926f2735a59753494fca72c3cfe4001d472f
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	cargo build --release
}

termux_step_make_install() {
	install -Dm755 target/release/sd "$TERMUX_PREFIX/bin/sd"
}
