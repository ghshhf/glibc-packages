# SkyNet 包目录

> 共 **335** 个标准组件包定义。每个包位于 `gpkg/<name>/` 目录下，遵循 [CONTRIBUTING.md](CONTRIBUTING.md) 规范构建。

## 索引

* [📚 基础 C 库](#基础-C-库) (8)
* [🛠️ 编译工具链](#编译工具链) (30)
* [🗜️ 压缩与归档](#压缩与归档) (16)
* [🔐 加密与安全](#加密与安全) (18)
* [🌍 网络协议与工具](#网络协议与工具) (24)
* [🗄️ 数据库](#数据库) (9)
* [🎨 图形与 GPU](#图形与-GPU) (37)
* [🎵 多媒体与音频](#多媒体与音频) (21)
* [⌨️ 编辑器与终端](#编辑器与终端) (16)
* [⚙️ 系统工具](#系统工具) (58)
* [📦 开发库](#开发库) (35)
* [🖥️ 桌面与 GUI](#桌面与-GUI) (46)
* [🌐 AI-TP 协议栈](#AI-TP-协议栈) (13)
* [🤖 AI 工具](#AI-工具) (4)

---

## 📚 基础 C 库

| Package | Description |
|---------|-------------|
| [gcc-libs](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/gcc-libs) | GCC runtime libraries |
| [gcc-libs-dev-glibc32](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/gcc-libs-dev-glibc32) | GCC 32-bit compatibility libraries |
| [glibc](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/glibc) | GNU C Library |
| [glibc-runner](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/glibc-runner) | glibc runtime environment |
| [libnsl](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libnsl) | Network Services Library |
| [libtirpc](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libtirpc) | Transport-independent RPC library |
| [libxcrypt](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxcrypt) | Extended crypt library |
| [linux-api-headers](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/linux-api-headers) | Linux kernel API header files |

## 🛠️ 编译工具链

| Package | Description |
|---------|-------------|
| [autoconf](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/autoconf) | GNU Autoconf configuration tool |
| [automake](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/automake) | GNU Automake Makefile generator |
| [binutils-libs](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/binutils-libs) | GNU binary utilities libraries |
| [bison](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/bison) | GNU Bison parser generator |
| [clang](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/clang) | C/C++/Objective-C compiler |
| [cmake](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/cmake) | Cross-platform build system |
| [compiler-rt](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/compiler-rt) | LLVM compiler runtime libraries |
| [doxygen](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/doxygen) | Documentation generator |
| [flex](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/flex) | Fast lexical analyzer generator |
| [gdb](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/gdb) | GNU Debugger |
| [libclc](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libclc) | OpenCL C library for LLVM |
| [libtool](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libtool) | GNU Libtool shared library support |
| [lld](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/lld) | LLVM linker |
| [llvm](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/llvm) | LLVM compiler infrastructure |
| [lua](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/lua) | Lua scripting language |
| [m4](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/m4) | GNU M4 macro processor |
| [make](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/make) | GNU Make build automation |
| [nodejs](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/nodejs) | JavaScript runtime (V8) |
| [patch](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/patch) | Apply diff patches |
| [perl](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/perl) | Perl programming language |
| [pkgconf](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/pkgconf) | Package compiler/linker metadata tool |
| [python](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/python) | Python programming language |
| [python-pip](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/python-pip) | Python package installer |
| [python-py3c](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/python-py3c) | Python 2/3 compatibility library |
| [ruby](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ruby) | Ruby programming language |
| [rust](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/rust) | Rust systems programming language |
| [swig](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/swig) | Wrapper and Interface Generator |
| [tcl](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/tcl) | Tool Command Language |
| [valgrind](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/valgrind) | Dynamic analysis tools |
| [wasm-runtime](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/wasm-runtime) | WASM runtime for SkyNet SSI-KRN |

## 🗜️ 压缩与归档

| Package | Description |
|---------|-------------|
| [brotli](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/brotli) | Generic-purpose lossless compression algorithm |
| [gzip](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/gzip) | GNU compression utility |
| [libarchive](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libarchive) | Multi-format archive and compression library |
| [libbz2](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libbz2) | Bzip2 compression library |
| [liblz4](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/liblz4) | LZ4 compression library |
| [liblzma](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/liblzma) | LZMA compression library |
| [liblzo](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/liblzo) | LZO compression library |
| [p7zip](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/p7zip) | 7-Zip compression utility |
| [tar](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/tar) | GNU tar tape archiver |
| [unrar](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/unrar) | RAR archive extraction utility |
| [unzip](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/unzip) | Info-ZIP decompression utility |
| [xz](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/xz) | XZ/LZMA compression utility |
| [zip](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/zip) | Info-ZIP compression utility |
| [zlib](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/zlib) | Compression library implementing the deflate compression method |
| [zlib-wasm](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/zlib-wasm) | WASM-compiled zlib compression library |
| [zstd](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/zstd) | Zstandard fast compression algorithm |

## 🔐 加密与安全

| Package | Description |
|---------|-------------|
| [ca-certificates](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ca-certificates) | Common CA certificate bundle |
| [krb5](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/krb5) | MIT Kerberos 5 authentication |
| [libgcrypt](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libgcrypt) | General purpose cryptographic library |
| [libgmp](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libgmp) | GNU Multiple Precision arithmetic library |
| [libgnutls](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libgnutls) | GNU TLS security library |
| [libgpg-error](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libgpg-error) | GnuPG error codes library |
| [libnettle](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libnettle) | Low-level cryptographic library |
| [libpam](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libpam) | Pluggable Authentication Modules |
| [libsasl](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libsasl) | SASL authentication library |
| [libseccomp](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libseccomp) | Linux seccomp filter library |
| [libsecret](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libsecret) | Secret service store library |
| [libsodium](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libsodium) | Modern easy-to-use crypto library |
| [libtasn1](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libtasn1) | ASN.1 library |
| [libtpms](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libtpms) | TPM library |
| [openldap](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/openldap) | OpenLDAP directory services |
| [openssl](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/openssl) | SSL/TLS cryptography library and tools |
| [p11-kit](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/p11-kit) | PKCS#11 module manager |
| [tpm2-tss](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/tpm2-tss) | TPM 2.0 Software Stack |

## 🌍 网络协议与工具

| Package | Description |
|---------|-------------|
| [apache2](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/apache2) | Apache HTTP server |
| [c-ares](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/c-ares) | Async DNS resolution library |
| [curl](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/curl) | URL transfer utility |
| [git](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/git) | Distributed version control system |
| [httpie](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/httpie) | Modern HTTP command-line client |
| [libcurl](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libcurl) | Client-side URL transfer library |
| [libevent](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libevent) | Event notification library |
| [libidn2](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libidn2) | Libidn2 |
| [libmicrohttpd](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libmicrohttpd) | Embedded HTTP server library |
| [libnghttp2](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libnghttp2) | HTTP/2 protocol library |
| [libp2p](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libp2p) | libp2p networking library |
| [libpcap](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libpcap) | Packet capture library |
| [libpsl](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libpsl) | Public Suffix List library |
| [libquic](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libquic) | Libquic |
| [libssh2](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libssh2) | SSH2 protocol library |
| [libstun](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libstun) | STUN/TURN protocol library |
| [nat-pmp](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/nat-pmp) | NAT Port Mapping Protocol |
| [net-tools](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/net-tools) | Network tools (ifconfig, route) |
| [openssh](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/openssh) | OpenSSH client and server |
| [publicsuffix-list](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/publicsuffix-list) | Public Suffix List data |
| [serf](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/serf) | HTTP client library |
| [socat](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/socat) | Port forwarding and relay tool |
| [subversion](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/subversion) | Apache Subversion version control |
| [wget](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/wget) | GNU network file download utility |

## 🗄️ 数据库

| Package | Description |
|---------|-------------|
| [leveldb](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/leveldb) | Google LevelDB key-value store |
| [libdb](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libdb) | Oracle Berkeley DB |
| [libsqlite](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libsqlite) | SQLite embedded database library |
| [mariadb](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/mariadb) | MariaDB relational database |
| [memcached](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/memcached) | High-performance distributed cache |
| [postgresql](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/postgresql) | PostgreSQL relational database |
| [redis](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/redis) | Redis in-memory data store |
| [sqlite3](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/sqlite3) | Embedded SQL database engine |
| [unixodbc](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/unixodbc) | ODBC database connectivity |

## 🎨 图形与 GPU

| Package | Description |
|---------|-------------|
| [assimp](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/assimp) | Open Asset Import Library |
| [fontconfig](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/fontconfig) | Font configuration library |
| [freeglut](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/freeglut) | Free OpenGL implementation |
| [freetype](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/freetype) | FreeType font rendering |
| [fribidi](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/fribidi) | Unicode BIDI algorithm |
| [ggml](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ggml) | Tensor machine learning library |
| [giflib](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/giflib) | GIF image library |
| [gifsicle](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/gifsicle) | GIF manipulation tool |
| [glm](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/glm) | OpenGL Mathematics library |
| [glmark2](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/glmark2) | OpenGL 2.0 benchmark |
| [glslang](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/glslang) | GLSL to SPIR-V compiler |
| [glu](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/glu) | OpenGL Utility library |
| [harfbuzz](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/harfbuzz) | Text shaping library |
| [jbigkit](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/jbigkit) | JBIG1 compression library |
| [libcairo](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libcairo) | Cairo 2D graphics library |
| [libdav1d](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libdav1d) | AV1 decoder library |
| [libdrm](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libdrm) | Direct Rendering Manager |
| [libepoxy](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libepoxy) | GL dispatch library |
| [libglvnd](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libglvnd) | GL Vendor-Neutral Dispatch |
| [libgraphite](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libgraphite) | Graphite font rendering |
| [libjpeg-turbo](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libjpeg-turbo) | JPEG codec with SIMD |
| [libpixman](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libpixman) | Pixel manipulation library |
| [libpng](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libpng) | PNG image library |
| [libtiff](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libtiff) | TIFF image library |
| [mangohud](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/mangohud) | Vulkan/OpenGL overlay monitor |
| [mesa](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/mesa) | Open-source GPU drivers |
| [mesa-demos](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/mesa-demos) | Mesa demo programs |
| [opengl](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/opengl) | OpenGL graphics system |
| [pango](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/pango) | Text layout engine |
| [spirv-headers](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/spirv-headers) | SPIR-V header files |
| [spirv-llvm-translator](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/spirv-llvm-translator) | SPIR-V to LLVM translator |
| [spirv-tools](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/spirv-tools) | SPIR-V optimizer/validator |
| [vkmark](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/vkmark) | Vulkan benchmark |
| [vulkan-headers](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/vulkan-headers) | Vulkan API headers |
| [vulkan-icd-loader](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/vulkan-icd-loader) | Vulkan ICD loader |
| [vulkan-tools](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/vulkan-tools) | Vulkan tools |
| [vulkan-volk](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/vulkan-volk) | Vulkan meta-loader |

## 🎵 多媒体与音频

| Package | Description |
|---------|-------------|
| [alsa-lib](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/alsa-lib) | Advanced Linux Sound Architecture |
| [gst-plugins-bad](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/gst-plugins-bad) | GStreamer experimental plugins |
| [gst-plugins-base](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/gst-plugins-base) | GStreamer base plugins |
| [gst-plugins-good](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/gst-plugins-good) | GStreamer good plugins |
| [gst-plugins-ugly](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/gst-plugins-ugly) | GStreamer legacy plugins |
| [gstreamer](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/gstreamer) | Multimedia pipeline framework |
| [libaom](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libaom) | AV1 codec implementation |
| [libflac](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libflac) | FLAC library |
| [libisl](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libisl) | Integer Set Library |
| [libmp3lame](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libmp3lame) | LAME MP3 encoder library |
| [libmpc](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libmpc) | Multiprecision complex library |
| [libmpfr](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libmpfr) | Multiple-precision floating-point |
| [libmpg123](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libmpg123) | MPEG audio decoder library |
| [libogg](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libogg) | Ogg bitstream container |
| [libomxil-bellagio](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libomxil-bellagio) | OpenMAX IL implementation |
| [libopus](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libopus) | Opus audio codec library |
| [libpulse](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libpulse) | PulseAudio client library |
| [libsndfile](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libsndfile) | Audio file format library |
| [libva](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libva) | Video Acceleration library |
| [libvdpau](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libvdpau) | Video decode/presentation library |
| [libvorbis](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libvorbis) | Ogg Vorbis audio codec |

## ⌨️ 编辑器与终端

| Package | Description |
|---------|-------------|
| [bash](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/bash) | Bourne-Again SHell |
| [bash-completion](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/bash-completion) | Programmable bash completion scripts |
| [cmatrix](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/cmatrix) | Matrix-style terminal animation |
| [ed](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ed) | GNU ed line editor |
| [figlet](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/figlet) | ASCII art text banner generator |
| [less](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/less) | Terminal pager program |
| [libedit](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libedit) | BSD editline library |
| [mc](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/mc) | Midnight Commander file manager |
| [nano](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/nano) | GNU nano text editor |
| [ncurses](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ncurses) | Terminal control library |
| [readline](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/readline) | GNU readline library for line editing |
| [rlwrap](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/rlwrap) | Readline wrapper |
| [screen](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/screen) | Terminal window manager |
| [tmux](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/tmux) | Terminal multiplexer |
| [vim](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/vim) | Vi IMproved text editor |
| [zsh](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/zsh) | Z shell |

## ⚙️ 系统工具

| Package | Description |
|---------|-------------|
| [attr](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/attr) | Extended attributes library |
| [bat](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/bat) | Cat with syntax highlighting |
| [bc](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/bc) | Arbitrary precision calculator |
| [box64](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/box64) | x86_64 emulator for ARM64 |
| [box86](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/box86) | x86 emulator for ARM |
| [btop](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/btop) | Modern resource monitor |
| [coreutils](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/coreutils) | GNU core utilities |
| [delta](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/delta) | Git diff viewer |
| [diffutils](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/diffutils) | GNU diff/patch utilities |
| [dos2unix](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/dos2unix) | Text file format converter |
| [dust](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/dust) | du with intuitive output |
| [e2fsprogs](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/e2fsprogs) | ext2/3/4 filesystem utilities |
| [eza](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/eza) | Modern ls replacement |
| [fakechroot](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/fakechroot) | Fake chroot environment |
| [fakehardlink](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/fakehardlink) | Fake hardlink support |
| [fakeroot](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/fakeroot) | Fake root for package builds |
| [fd](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/fd) | Simple find alternative |
| [file](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/file) | File type identification tool |
| [findutils](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/findutils) | GNU find/locate utilities |
| [fzf](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/fzf) | Fuzzy finder |
| [gawk](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/gawk) | GNU awk language |
| [grep](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/grep) | GNU pattern matching utility |
| [groff](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/groff) | GNU troff formatting system |
| [htop](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/htop) | Interactive process viewer |
| [jq](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/jq) | Command-line JSON processor |
| [libacl](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libacl) | Access control list library |
| [libcap](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libcap) | POSIX capability library |
| [libcap-ng](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libcap-ng) | POSIX capability library (NG) |
| [libpipeline](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libpipeline) | Pipeline manipulation library |
| [lnav](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/lnav) | Log file navigator |
| [lsof](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/lsof) | List open files tool |
| [man-db](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/man-db) | Manual page database viewer |
| [ncdu](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ncdu) | Disk usage analyzer |
| [neofetch](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/neofetch) | System info fetch tool |
| [nmap](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/nmap) | Network discovery scanner |
| [patchelf](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/patchelf) | ELF binary patching tool |
| [procps-ng](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/procps-ng) | New generation proc tools |
| [procs](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/procs) | Modern ps replacement |
| [psmisc](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/psmisc) | Process management utilities |
| [pv](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/pv) | Pipe viewer progress monitor |
| [ranger](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ranger) | Terminal file manager |
| [rhash](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/rhash) | Recursive hash calculator |
| [ripgrep](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ripgrep) | Ultra-fast text search |
| [rsync](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/rsync) | Fast incremental file transfer |
| [sd](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/sd) | Intuitive find/replace CLI |
| [sed](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/sed) | GNU stream editor |
| [strace](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/strace) | System call tracer |
| [syncthing](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/syncthing) | Peer-to-peer file sync |
| [tcpdump](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/tcpdump) | Network packet capture tool |
| [termux-exec](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/termux-exec) | Termux exec wrapper |
| [texinfo](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/texinfo) | GNU documentation system |
| [time](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/time) | GNU time command timer |
| [tree](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/tree) | Directory tree listing |
| [units](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/units) | GNU units conversion tool |
| [util-linux](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/util-linux) | Linux system utilities |
| [which](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/which) | Show full path of commands |
| [yq](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/yq) | Command-line YAML processor |
| [zoxide](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/zoxide) | Smarter cd command |

## 📦 开发库

| Package | Description |
|---------|-------------|
| [apr](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/apr) | Apache Portable Runtime |
| [apr-util](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/apr-util) | Apache Portable Runtime utilities |
| [asciidoc](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/asciidoc) | Text-based document format |
| [boost](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/boost) | Boost C++ libraries |
| [cmocka](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/cmocka) | Unit testing framework for C |
| [cppdap](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/cppdap) | DAP C++ library |
| [docbook-xml](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/docbook-xml) | DocBook XML schema |
| [docbook-xsl](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/docbook-xsl) | DocBook XSL stylesheets |
| [gdbm](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/gdbm) | GNU dbm database library |
| [gettext](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/gettext) | GNU internationalization |
| [glib](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/glib) | GLib core utility library |
| [gobject-introspection](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/gobject-introspection) | GObject introspection |
| [json-c](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/json-c) | C JSON library |
| [jsoncpp](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/jsoncpp) | C++ JSON library |
| [libai-storage](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libai-storage) | AI-TP storage library |
| [libaitp-common](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libaitp-common) | AI-TP common utilities |
| [libelf](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libelf) | ELF file access library |
| [libexpat](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libexpat) | XML parser library |
| [libffi](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libffi) | Foreign Function Interface |
| [libgc](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libgc) | Boehm garbage collector |
| [libiconv](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libiconv) | Character encoding conversion |
| [libicu](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libicu) | International Components for Unicode |
| [libjansson](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libjansson) | C JSON library (Jansson) |
| [libunistring](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libunistring) | Unicode string library |
| [libunwind](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libunwind) | Stack unwinding library |
| [libuv](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libuv) | Cross-platform async I/O library |
| [libverto](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libverto) | Event loop abstraction |
| [libxml2](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxml2) | XML C parser toolkit |
| [libxslt](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxslt) | XSLT transformation library |
| [mpdecimal](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/mpdecimal) | Decimal floating point library |
| [nlohmann-json](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/nlohmann-json) | JSON for Modern C++ |
| [oniguruma](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/oniguruma) | Regular expression library |
| [pcre2](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/pcre2) | Pcre2 |
| [utf8proc](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/utf8proc) | UTF-8 processing library |
| [xmlto](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/xmlto) | DocBook to other formats |

## 🖥️ 桌面与 GUI

| Package | Description |
|---------|-------------|
| [dbus](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/dbus) | D-Bus message bus system |
| [dconf](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/dconf) | DConf configuration database |
| [gtk-doc](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/gtk-doc) | GTK+ documentation tool |
| [libcloudproviders](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libcloudproviders) | Cloud provider integration |
| [libdatrie](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libdatrie) | Double-array trie library |
| [libice](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libice) | X11 Inter-Client Exchange |
| [libpciaccess](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libpciaccess) | PCI device access library |
| [libsm](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libsm) | X11 Session Management |
| [libthai](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libthai) | Thai language support |
| [libwayland](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libwayland) | Wayland protocol library |
| [libwayland-protocols](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libwayland-protocols) | Wayland protocol extensions |
| [libx11](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libx11) | X11 client-side library |
| [libxau](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxau) | X11 authorization library |
| [libxaw](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxaw) | X Athena Widget Set |
| [libxcb](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxcb) | X protocol C-binding |
| [libxcomposite](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxcomposite) | X11 Composite extension |
| [libxcursor](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxcursor) | X11 cursor library |
| [libxdmcp](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxdmcp) | X11 DMCP library |
| [libxext](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxext) | X11 extensions library |
| [libxfixes](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxfixes) | X11 fixes extension |
| [libxft](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxft) | X FreeType library |
| [libxi](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxi) | X11 input extension |
| [libxinerama](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxinerama) | X11 Xinerama extension |
| [libxkbcommon](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxkbcommon) | XKB keymap library |
| [libxkbfile](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxkbfile) | X11 keyboard file library |
| [libxmu](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxmu) | X11 miscellaneous utilities |
| [libxpm](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxpm) | X11 pixmap library |
| [libxrandr](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxrandr) | X11 RandR extension |
| [libxrender](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxrender) | X11 Render extension |
| [libxshmfence](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxshmfence) | X11 shared memory fence |
| [libxss](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxss) | X11 Screen Saver extension |
| [libxt](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxt) | X11 toolkit intrinsics |
| [libxtst](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxtst) | X11 Test extension |
| [libxxf86vm](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/libxxf86vm) | X11 video mode extension |
| [mtdev](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/mtdev) | Multitouch transformation library |
| [tk](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/tk) | Tk GUI toolkit |
| [ttf-dejavu](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ttf-dejavu) | DejaVu fonts |
| [xcb-proto](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/xcb-proto) | XCB protocol descriptions |
| [xcb-util-wm](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/xcb-util-wm) | XCB window manager utils |
| [xkeyboard-config](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/xkeyboard-config) | X keyboard configuration |
| [xorg-util-macros](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/xorg-util-macros) | X.Org utility macros |
| [xorg-xclock](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/xorg-xclock) | X analog/digital clock |
| [xorg-xkbcomp](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/xorg-xkbcomp) | XKB keyboard compiler |
| [xorg-xmessage](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/xorg-xmessage) | X message display utility |
| [xorgproto](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/xorgproto) | X11 protocol headers |
| [xtrans](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/xtrans) | X11 transport layer |

## 🌐 AI-TP 协议栈

| Package | Description |
|---------|-------------|
| [ai-tp-address](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ai-tp-address) | AI-TP distributed addressing |
| [ai-tp-consensus](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ai-tp-consensus) | AI-TP consensus protocol |
| [ai-tp-crypto](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ai-tp-crypto) | AI-TP cryptographic primitives |
| [ai-tp-discovery](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ai-tp-discovery) | AI-TP peer discovery |
| [ai-tp-gateway](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ai-tp-gateway) | AI-TP gateway service |
| [ai-tp-integration](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ai-tp-integration) | AI-TP integration framework |
| [ai-tp-lan](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ai-tp-lan) | AI-TP local area network |
| [ai-tp-nat](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ai-tp-nat) | AI-TP NAT traversal |
| [ai-tp-pow](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ai-tp-pow) | AI-TP proof of work |
| [ai-tp-scheduler](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ai-tp-scheduler) | AI-TP task scheduler |
| [ai-tp-sync](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ai-tp-sync) | AI-TP data synchronization |
| [ai-tp-worker](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ai-tp-worker) | AI-TP distributed worker |
| [ai-tp-zkp](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ai-tp-zkp) | AI-TP zero knowledge proof |

## 🤖 AI 工具

| Package | Description |
|---------|-------------|
| [localai](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/localai) | Local AI model serving platform |
| [ollama](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/ollama) | Local LLM serving tool |
| [scrapegraphai](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/scrapegraphai) | Graph-based web scraping AI |
| [scrapegraphai-core](https://github.com/ghshhf/glibc-packages/tree/master/gpkg/scrapegraphai-core) | ScrapeGraphAI core library |
