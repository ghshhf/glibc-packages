TERMUX_PKG_NAME="gst-plugins-ugly-glibc"
TERMUX_PKG_HOMEPAGE="https://gstreamer.freedesktop.org/"
TERMUX_PKG_DESCRIPTION="GStreamer Ugly Plugins - potentially problematic plugins"
TERMUX_PKG_LICENSE="LGPL-2.1"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION="1.28.4"
TERMUX_PKG_SRCURL=https://gstreamer.freedesktop.org/src/gst-plugins-ugly/gst-plugins-ugly-${TERMUX_PKG_VERSION}.tar.xz
TERMUX_PKG_SHA256= classic disabled - need to check actual hash
TERMUX_PKG_DEPENDS="glibc, gstreamer-glibc, gst-plugins-base-glibc, libdvdcss-glibc, libx264-glibc"
TERMUX_PKG_BUILD_DEPENDS="meson, ninja, pkg-config, gstreamer-glibc, gst-plugins-base-glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
-D a52dec=disabled
-D amrnb=disabled
-D amrwbdec=disabled
-D asif=disabled
-D cdio=disabled
-D dillo=disabled
-D dvdnav=disabled
-D dvdread=disabled
-D gstvideo-marshal=disabled
-D lame=disabled
-D mad=disabled
-D mpeg2dec=disabled
-D sidplay=disabled
-D twolame=disabled
-D x264=disabled
-D examples=disabled
-D tests=disabled
-D doc=disabled
"

termux_step_pre_configure() {
    # 禁用可能有版权问题的插件
    export GST_PLUGIN_SYSTEM_PATH="${TERMUX_PREFIX}/lib/gstreamer-1.0"
    echo "Disabling potentially problematic plugins for glibc environment..."
}

termux_step_post_make_install() {
    # 更新黑名单配置
    mkdir -p "${TERMUX_PREFIX}/etc/gstreamer-1.0"
    
    # 添加 ugly 插件中的黑名单项
    cat >> "${TERMUX_PREFIX}/etc/gstreamer-1.0/plugin-blacklist.conf" << EOF

# Ugly plugins - disabled by default due to potential issues
# Uncomment to enable if needed and legally allowed
# blacklist=x264 - encoding issues
# blacklist=mad - may have licensing issues in some regions
EOF
}
