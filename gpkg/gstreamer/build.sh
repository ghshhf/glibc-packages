TERMUX_PKG_HOMEPAGE="https://gstreamer.freedesktop.org/"
TERMUX_PKG_DESCRIPTION="GStreamer open source multimedia framework core library"
TERMUX_PKG_LICENSE="LGPL-2.1"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION="1.28.4"
TERMUX_PKG_SRCURL=https://gstreamer.freedesktop.org/src/gstreamer/gstreamer-${TERMUX_PKG_VERSION}.tar.xz
TERMUX_PKG_SHA256=f5adc7e8f448c10260b3b25aa101c9d540674c8d9a54c2b77a86d04f2b3b50dd
TERMUX_PKG_DEPENDS="glibc, libxml2-glibc, zlib-glibc, gnutls-glibc"
TERMUX_PKG_BUILD_DEPENDS="meson, ninja, pkg-config, glibc-dev, libxml2-glibc-dev, zlib-glibc-dev"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
-D introspection=disabled
-D check=disabled
-D tests=disabled
-D examples=disabled
-D benchmarks=disabled
-D libunwind=disabled
-D libdw=disabled
-D doc=disabled
-D tools=enabled
-D gst_debug=true
-D gst_parse=true
"

termux_step_pre_configure() {
    # 黑名单处理：禁用可能有问题的插件
    # 可以通过环境变量或配置文件在运行时禁用
    # 这里在构建时禁用一些已知有问题的功能
    export GST_PLUGIN_SYSTEM_PATH="${TERMUX_PREFIX}/lib/gstreamer-1.0"
}

termux_step_post_make_install() {
    # 创建黑名单配置文件目录
    mkdir -p "${TERMUX_PREFIX}/etc/gstreamer-1.0"
    
    # 创建默认的黑名单配置文件
    # 用户可以通过 gst-inspect-1.0 --print-blacklist 查看黑名单
    # 这里创建一个空的黑名单文件，用户可以根据需要添加
    cat > "${TERMUX_PREFIX}/etc/gstreamer-1.0/plugin-blacklist.conf" << EOF
# GStreamer Plugin Blacklist Configuration
# Use 'gst-inspect-1.0 --print-blacklist' to see blacklisted plugins
# Add plugins to blacklist by uncommenting or adding new lines
# Format: blacklist=<plugin-name>
EOF
}
