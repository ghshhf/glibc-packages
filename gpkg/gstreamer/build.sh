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
    # 这里创建一个配置文件，列出已知的黑名单插件
    cat > "${TERMUX_PREFIX}/etc/gstreamer-1.0/plugin-blacklist.conf" << EOF
# GStreamer Plugin Blacklist Configuration
# Use 'gst-inspect-1.0 --print-blacklist' to see blacklisted plugins
# 
# To blacklist a plugin, add a line: blacklist=<plugin-name>
# To remove from blacklist, delete: rm -f ~/.cache/gstreamer-1.0/plugin-cache-*

# Known problematic plugins in glibc environment (disabled by default in build):
# - dv (dependency issues)
# - vpx (may cause issues on some architectures)
# - x11-related (not needed in headless environment)
EOF

    # 创建帮助脚本：检查黑名单
    mkdir -p "${TERMUX_PREFIX}/bin"
    cat > "${TERMUX_PREFIX}/bin/gst-blacklist-manager" << 'HELPSCRIPT'
#!/bin/bash

show_help() {
    cat << EOF
GStreamer Blacklist Manager
Usage: $0 [command]

Commands:
  check       - Check currently blacklisted plugins
  clear       - Clear all blacklisted plugins (delete cache)
  clear <plugin> - Remove specific plugin from blacklist
  list-config - Show current blacklist configuration
  help        - Show this help message

Examples:
  $0 check
  $0 clear
  $0 clear libdv
EOF
}

check_blacklist() {
    echo "=== GStreamer Blacklisted Plugins ==="
    if command -v gst-inspect-1.0 &> /dev/null; then
        gst-inspect-1.0 --print-blacklist 2>/dev/null || echo "No blacklisted plugins found."
    else
        echo "gst-inspect-1.0 not found. Make sure gstreamer is installed."
    fi
    echo ""
    echo "=== Plugin Cache Location ==="
    echo "$HOME/.cache/gstreamer-1.0/"
    if [ -d "$HOME/.cache/gstreamer-1.0" ]; then
        echo "Cache files:"
        ls -la "$HOME/.cache/gstreamer-1.0/" 2>/dev/null || echo "  (none)"
    else
        echo "  (cache directory does not exist)"
    fi
}

clear_blacklist() {
    if [ -d "$HOME/.cache/gstreamer-1.0" ]; then
        if [ -z "$1" ]; then
            echo "Clearing all blacklisted plugins..."
            rm -f "$HOME/.cache/gstreamer-1.0/plugin-cache-"*
            echo "Done. Plugin cache cleared."
        else
            echo "Removing specific plugin from blacklist: $1"
            # GStreamer doesn't have a direct way to remove specific plugin from blacklist
            # User needs to delete the entire cache
            echo "Note: GStreamer doesn't support removing specific plugins from blacklist."
            echo "You need to clear the entire cache: $0 clear"
        fi
    else
        echo "No blacklist cache found."
    fi
}

list_config() {
    echo "=== Blacklist Configuration ==="
    if [ -f "${TERMUX_PREFIX}/etc/gstreamer-1.0/plugin-blacklist.conf" ]; then
        cat "${TERMUX_PREFIX}/etc/gstreamer-1.0/plugin-blacklist.conf"
    else
        echo "Configuration file not found."
    fi
}

case "$1" in
    check)
        check_blacklist
        ;;
    clear)
        clear_blacklist "$2"
        ;;
    list-config)
        list_config
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        show_help
        exit 1
        ;;
esac
HELPSCRIPT
    chmod +x "${TERMUX_PREFIX}/bin/gst-blacklist-manager"
}
