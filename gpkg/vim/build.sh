TERMUX_PKG_HOMEPAGE=https://www.vim.org/
TERMUX_PKG_DESCRIPTION="Vi IMproved - enhanced vi editor"
TERMUX_PKG_LICENSE="Vim"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=9.2.0000
TERMUX_PKG_SRCURL=https://github.com/vim/vim/archive/refs/tags/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=875875fb5988af3db0726bef9b048a559a9563aa0ecca8240e82057e8e5941c3
TERMUX_PKG_DEPENDS="ncurses-glibc, glibc"
TERMUX_PKG_BUILD_IN_SRC=true
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--with-features=normal
--enable-multibyte
--enable-xim
--disable-gui
--disable-rubyinterp
--disable-perlinterp
--disable-tclinterp
"

termux_step_pre_configure() {
	cd src
}

termux_step_make_install() {
	cd src
	make install
}
