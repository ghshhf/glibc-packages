TERMUX_PKG_HOMEPAGE="https://gstreamer.freedesktop.org/"
TERMUX_PKG_DESCRIPTION="GStreamer Good Plugins - high-quality plugins"
TERMUX_PKG_LICENSE="LGPL-2.1"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION="1.24.12"
TERMUX_PKG_SRCURL=https://gstreamer.freedesktop.org/src/gst-plugins-good/gst-plugins-good-${TERMUX_PKG_VERSION}.tar.xz
TERMUX_PKG_SHA256=placeholder_sha256_hash_good
TERMUX_PKG_DEPENDS="gstreamer, gst-plugins-base, glib, libxml2, zlib, libsoup, libpng, libjpeg, speex, cairo, pango, alsa-lib, gnutls"
TERMUX_PKG_BUILD_DEPENDS="meson, ninja, pkg-config, gstreamer-dev, gst-plugins-base-dev, glib-dev, libsoup-dev, libpng-dev, libjpeg-dev"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
-D alpha=enabled
-D apetag=enabled
-D audiofx=enabled
-D audioparsers=enabled
-D auparse=enabled
-D autodetect=enabled
-D avi=enabled
-D cairo=enabled
-D cutter=enabled
-D debugutils=enabled
-D deinterlace=enabled
-D dtmf=enabled
-D dv=disabled
-D effectv=enabled
-D equalizer=enabled
-D flac=enabled
-D flv=enabled
-D gdk-pixbuf=disabled
-D goom=enabled
-D goom2k1=enabled
-D icydemux=enabled
-D imagefreeze=enabled
-D interleave=enabled
-D jpeg=enabled
-D level=enabled
-D matroska=enabled
-D monoscope=enabled
-D multifile=enabled
-D multipart=enabled
-D navigationtest=enabled
-D oss4=disabled
-D ossaudio=disabled
-D png=enabled
-D qml6=disabled
-D rawparse=enabled
-D rtp=enabled
-D rtpmanager=enabled
-D rtsp=enabled
-D shapewipe=enabled
-D smpte=enabled
-D soup=enabled
-D spectrum=enabled
-D speex=enabled
-D taglib=disabled
-D udp=enabled
-D videobox=enabled
-D videocrop=enabled
-D videofilter=enabled
-D videomixer=enabled
-D vpx=disabled
-D wavenc=enabled
-D wavparse=enabled
-D xingmux=enabled
-D y4m=enabled
-D examples=disabled
-D tests=disabled
-D doc=disabled
-D nls=disabled
-D orc=disabled
"

termux_step_pre_configure() {
    # 禁用可能有问题的插件
    export GST_PLUGIN_SYSTEM_PATH="${TERMUX_PREFIX}/lib/gstreamer-1.0"
    
    # 黑名单处理：在构建时禁用已知有问题的插件
    # 这些插件可能在运行时被加入黑名单，所以我们直接禁用它们
    echo "Disabling problematic plugins for glibc environment..."
}

termux_step_post_make_install() {
    # 更新黑名单配置文件
    mkdir -p "${TERMUX_PREFIX}/etc/gstreamer-1.0"
    
    # 添加 good 插件中的黑名单项
    cat >> "${TERMUX_PREFIX}/etc/gstreamer-1.0/plugin-blacklist.conf" << EOF
# Good plugins - disabled by default due to issues in glibc environment
# Uncomment to enable if needed and working
# blacklist=dv - dependency issues
# blacklist=vpx - may cause issues on some architectures
EOF
    
    # 创建帮助脚本：检查黑名单
    cat > "${TERMUX_PREFIX}/bin/gst-check-blacklist" << 'EOF'
#!/bin/bash
echo "=== GStreamer Blacklisted Plugins ==="
gst-inspect-1.0 --print-blacklist 2>/dev/null || echo "No blacklisted plugins or gst-inspect-1.0 not found"
echo ""
echo "=== Plugin Cache Location ==="
echo "$HOME/.cache/gstreamer-1.0/"
echo ""
echo "=== To remove a plugin from blacklist ==="
echo "Delete the plugin cache file: rm -f ~/.cache/gstreamer-1.0/plugin-cache-*"
EOF
    chmod +x "${TERMUX_PREFIX}/bin/gst-check-blacklist"
}
