TERMUX_PKG_NAME="gst-plugins-bad-glibc"
TERMUX_PKG_HOMEPAGE="https://gstreamer.freedesktop.org/"
TERMUX_PKG_DESCRIPTION="GStreamer Bad Plugins - extra plugins"
TERMUX_PKG_LICENSE="LGPL-2.1"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION="1.28.4"
TERMUX_PKG_SRCURL=https://gstreamer.freedesktop.org/src/gst-plugins-bad/gst-plugins-bad-${TERMUX_PKG_VERSION}.tar.xz
TERMUX_PKG_SHA256=97d79d5a073b6cb718b4ce75d2a7dc8e79448754e8bca1bbb6b4716816f804
TERMUX_PKG_DEPENDS="glibc, gstreamer-glibc, gst-plugins-base-glibc, libx11-glibc, libwayland-glibc, libdrm-glibc, mesa-glibc"
TERMUX_PKG_BUILD_DEPENDS="meson, ninja, pkg-config, gstreamer-glibc, gst-plugins-base-glibc"
TERMUX_PKG_EXTRA_CONFIGURE_ARGS="
-D aom=disabled
-D assrender=disabled
-D avtp=disabled
-D bluez5=disabled
-D bz2=disabled
-D chromaprint=disabled
-D closedcaption=disabled
-D colormanagement=disabled
-D curl=disabled
-D curlwrapper=disabled
-D dash=disabled
-D dc1394=disabled
-D decklink=disabled
-D directfb=disabled
-D dtls=disabled
-D faac=disabled
-D faad=disabled
-D fdk_aac=disabled
-D fluidsynth=disabled
-D gme=disabled
-D gsm=disabled
-D hls=enabled
-D iqa=disabled
-D kate=disabled
-D ladspa=disabled
-D libde265=disabled
-D libmms=disabled
-D libopenh264=disabled
-D libsrtp2=disabled
-D libxml2=enabled
-D lv2=disabled
-D mediafoundation=disabled
-D modplug=disabled
-D mpeg2enc=disabled
-D mplex=disabled
-D msdk=disabled
-D neon=disabled
-D nvcodec=disabled
-D openal=disabled
-D opencv=disabled
-D openexr=disabled
-D openh264=disabled
-D openjpeg=disabled
-D openmpt=disabled
-D openni2=disabled
-D opus=disabled
-D resindvd=disabled
-D rsvg=disabled
-D rtmp=disabled
-D sbc=disabled
-D sctp=disabled
-D smoothstreaming=disabled
-D sndfile=disabled
-D soundtouch=disabled
-D spandsp=disabled
-D srt=disabled
-D srtp=disabled
-D svthevcenc=disabled
-D teletext=disabled
-D tinyalsa=disabled
-D transcode=disabled
-D va=disabled
-D voaacenc=disabled
-D voamrwbenc=disabled
-D vulkan=disabled
-D wasapi=disabled
-D wayland=disabled
-D webp=disabled
-D webrtc=disabled
-D wildmidi=disabled
-D wpe=disabled
-D x11=disabled
-D x265=disabled
-D zbar=disabled
-D examples=disabled
-D tests=disabled
-D doc=disabled
-D nls=disabled
-D orc=disabled
"

termux_step_pre_configure() {
    # 禁用可能有问题的插件
    export GST_PLUGIN_SYSTEM_PATH="${TERMUX_PREFIX}/lib/gstreamer-1.0"
    echo "Disabling problematic plugins for glibc environment..."
}

termux_step_post_make_install() {
    # 更新黑名单配置
    mkdir -p "${TERMUX_PREFIX}/etc/gstreamer-1.0"
    
    # 添加 bad 插件中的黑名单项
    cat >> "${TERMUX_PREFIX}/etc/gstreamer-1.0/plugin-blacklist.conf" << EOF

# Bad plugins - disabled by default due to issues in glibc environment
# Uncomment to enable if needed and working
# blacklist=x265 - encoding issues
# blacklist=openh264 - may cause issues
EOF
}
