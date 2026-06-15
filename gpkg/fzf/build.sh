TERMUX_PKG_HOMEPAGE=https://github.com/junegunn/fzf
TERMUX_PKG_DESCRIPTION="A command-line fuzzy finder"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=0.73.1
TERMUX_PKG_SRCURL=https://github.com/junegunn/fzf/archive/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=ae4f49f8606a7d28498208fa1b93c5d3b890719eea97e02559e66160138b750c
TERMUX_PKG_DEPENDS="glibc"
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_make() {
	go build -o fzf .
}

termux_step_make_install() {
	install -Dm755 fzf "$TERMUX_PREFIX/bin/fzf"
	install -Dm644 man/man1/fzf.1 "$TERMUX_PREFIX/share/man/man1/fzf.1"
	install -Dm644 man/man1/fzf-tmux.1 "$TERMUX_PREFIX/share/man/man1/fzf-tmux.1"
}
