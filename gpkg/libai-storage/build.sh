TERMUX_PKG_NAME="libai-storage"
TERMUX_PKG_VERSION="0.1.0"
TERMUX_PKG_DESCRIPTION="AI-TP OS 存储抽象层"
TERMUX_PKG_DEPENDS=""
TERMUX_PKG_BUILD_IN_SRC=true

termux_step_configure() {
    echo "跳过 configure"
}

termux_step_make() {
    cd $TERMUX_PKG_SRCDIR
    gcc -Wall -Wextra -O2 -fPIC -Iinclude -c src/ai-storage.c -o src/ai-storage.o
    gcc -shared -o libai-storage.so src/ai-storage.o
    ar rcs libai-storage.a src/ai-storage.o
}

termux_step_make_install() {
    cd $TERMUX_PKG_SRCDIR
    install -d $TERMUX_PREFIX/include
    install -d $TERMUX_PREFIX/lib
    install -m 644 include/ai-storage.h $TERMUX_PREFIX/include/
    install -m 755 libai-storage.so $TERMUX_PREFIX/lib/
    install -m 644 libai-storage.a $TERMUX_PREFIX/lib/
}
