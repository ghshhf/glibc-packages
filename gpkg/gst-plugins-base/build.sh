TERMUX_PKG_HOMEPAGE="https://gstreamer.freedesktop.org/"
TERMUX_PKG_DESCRIPTION="GStreamer Base Plugins"
TERMUX_PKG_LICENSE="LGPL-2.1"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION="1.28.4"
TERMUX_PKG_SRCURL=https://gstreamer.freedesktop.org/src/gst-plugins-base/gst-plugins-base-${TERMUX_PKG_VERSION}.tar.xz
TERMUX_PKG_SHA256=a898afd5766172b0049e6781558e0689098bf87b9d82b846c652e571c01d60d8
TERMUX_PKG_DEPENDS="glibc, gstreamer-glibc, libogg-glibc, libvorbis-glibc, libtheora-glibc, alsa-lib-glibc, pango-glibc, cairo-glibc, libjpeg-turbo-glibc, libpng-glibc, zlib-glibc"
TERMUX_PKG_BUILD_DEPENDS="meson, ninja, pkg-config, glibc-dev, gstreamer-glibc-dev"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
-D alsa=enabled
-D cdparanoia=disabled
-D glib2=enabled
-D libvisual=disabled
-D ogg=enabled
-D oggvorbis=enabled
-D pango=enabled
-D theora=disabled
-D vorbis=enabled
-D x11=disabled
-D xcb=disabled
-D xshm=disabled
-D xvideo=disabled
-D examples=disabled
-D tests=disabled
-D doc=disabled
-D nls=disabled
"

termux_step_pre_configure() {
    # 禁用可能有问题的插件
    export GST_PLUGIN_SYSTEM_PATH="${TERMUX_PREFIX}/lib/gstreamer-1.0"
}

termux_step_post_make_install() {
    # 创建插件黑名单配置
    mkdir -p "${TERMUX_PREFIX}/etc/gstreamer-1.0"
    
    # 添加基础插件中的黑名单项（如果有）
    cat >> "${TERMUX_PREFIX}/etc/gstreamer-1.0/plugin-blacklist.conf" << EOF
# Base plugins - disabled by default due to issues
# Uncomment to enable if needed
# blacklist=cdparanoia
EOF
}
