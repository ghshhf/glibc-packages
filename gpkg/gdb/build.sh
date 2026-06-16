TERMUX_PKG_HOMEPAGE=https://www.gnu.org/software/gdb/
TERMUX_PKG_DESCRIPTION="GNU debugger"
TERMUX_PKG_LICENSE="GPL-3.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=17.2
TERMUX_PKG_SRCURL=https://mirrors.kernel.org/gnu/gdb/gdb-${TERMUX_PKG_VERSION}.tar.xz
TERMUX_PKG_SHA256=2c172aa28dcaca89c4a453edb26e0ed9fe16675d584675604bb842df4db68a2a
TERMUX_PKG_DEPENDS="ncurses-glibc, readline-glibc, libexpat-glibc, python-glibc, libgmp-glibc, libmpfr-glibc, zlib-glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
--disable-gdbserver
--disable-ld
--disable-gas
--disable-binutils
--disable-gprof
--with-system-gdbinit=${TERMUX_PREFIX}/etc/gdb/gdbinit
--with-python=${TERMUX_PREFIX}/bin/python3
"
