TERMUX_PKG_HOMEPAGE=https://github.com/kislyuk/yq
TERMUX_PKG_DESCRIPTION="Command-line YAML/XML/TOML processor - jq wrapper"
TERMUX_PKG_LICENSE="Apache-2.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=3.4.3
TERMUX_PKG_SRCURL=https://pypi.org/packages/source/y/yq/yq-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=ba586a1a6f30cf705b2f92206712df2281cd320280210e7b7b80adcb8f256e3b
TERMUX_PKG_DEPENDS="python-glibc, python-pip, jq"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make_install() {
	pip install . --prefix="$TERMUX_PREFIX"
}
