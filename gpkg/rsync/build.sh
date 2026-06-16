TERMUX_PKG_HOMEPAGE=https://rsync.samba.org/
TERMUX_PKG_DESCRIPTION="Fast, versatile, remote (and local) file-copying tool"
TERMUX_PKG_LICENSE="GPL-3.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=3.4.4
TERMUX_PKG_SRCURL=https://rsync.samba.org/ftp/rsync/rsync-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=3ca57887470fdf223a8e8aae4559cd3a26787bea93b94336c90ee8062e29e352
TERMUX_PKG_DEPENDS="zlib-glibc, openssl-glibc, liblz4-glibc, liblzma-glibc, zstd-glibc, libacl-glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--disable-xxhash
--disable-md2man
"
