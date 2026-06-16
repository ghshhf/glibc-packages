TERMUX_PKG_HOMEPAGE=https://github.com/ngtcp2/ngtcp2
TERMUX_PKG_DESCRIPTION="QUIC protocol implementation based on ngtcp2"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION=1.22.1
TERMUX_PKG_SRCURL=https://github.com/ngtcp2/ngtcp2/releases/download/v${TERMUX_PKG_VERSION}/ngtcp2-${TERMUX_PKG_VERSION}.tar.xz
TERMUX_PKG_SHA256=
TERMUX_PKG_DEPENDS="glibc, openssl-glibc"
TERMUX_PKG_AUTO_UPDATE=true

termux_step_pre_configure() {
	autoreconf -fi
}
