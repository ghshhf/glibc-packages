TERMUX_PKG_NAME="ai-tp-nat"
TERMUX_PKG_VERSION="0.1.0"
TERMUX_PKG_DESCRIPTION="AI-TP OS NAT 穿透与节点组网工具"
TERMUX_PKG_DEPENDS=""
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_configure() {
    echo "跳过 configure"
}

termux_step_make() {
    cd $TERMUX_PKG_SRCDIR
    gcc -Wall -Wextra -O2 -fPIC -Iinclude -c src/ai-tp-nat.c -o src/ai-tp-nat.o
    gcc -shared -o libai-tp-nat.so src/ai-tp-nat.o
    ar rcs libai-tp-nat.a src/ai-tp-nat.o
}

termux_step_make_install() {
    cd $TERMUX_PKG_SRCDIR
    install -d $TERMUX_PREFIX/include
    install -d $TERMUX_PREFIX/lib
    install -m 644 include/ai-tp-nat.h $TERMUX_PREFIX/include/
    install -m 755 libai-tp-nat.so $TERMUX_PREFIX/lib/
    install -m 644 libai-tp-nat.a $TERMUX_PREFIX/lib/
}
