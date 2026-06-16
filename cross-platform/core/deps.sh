#!/bin/bash
# =============================================================================
# Core: Dependency Name Mapping
# =============================================================================
# 依赖名称翻译层。glibc-packages 的包命名可能与各系统原生包管理器不同。
# 此模块提供统一的依赖名称转换。
#
# 主要功能:
#   cross_normalize_dep_name     - 从 glibc-packages 命名转换为平台原生命名
#   cross_map_library_to_dep     - 从库名 (如 ncurses) 映射到平台包名
#   cross_check_dep              - 检查依赖是否已安装
#
# 命名规则:
#   Android (Termux):   ncurses-glibc, libevent-glibc, python-glibc
#   Linux (Debian):     libncurses6, libevent-2.1-7, python3
#   Linux (Arch):       ncurses, libevent, python
#   Windows (MSYS2):    mingw-w64-x86_64-ncurses, mingw-w64-x86_64-libevent
#   macOS (Homebrew):   ncurses, libevent, python
# =============================================================================

# ---------------------------------------------------------------------------
# 1. 从 glibc-packages 命名 (xxx-glibc) 转换为平台原生命名
# ---------------------------------------------------------------------------
cross_normalize_dep_name() {
    local dep="$1"

    # 如果不包含 -glibc 后缀，可能已经是原生名称
    if [[ "$dep" != *"-glibc" ]]; then
        # 如果 glibc 包本身直接返回
        if [[ "$dep" == "glibc" ]]; then
            echo "${CROSS_DEP_GLIBC_NAME:-libc6}"
            return
        fi
        # 已是非 glibc 命名，按原样返回
        echo "$dep"
        return
    fi

    # 去掉 -glibc 后缀，获得库名
    local base_name="${dep%-glibc}"

    # 根据平台转换为原生命名
    case "${CROSS_PLATFORM}" in
        android)
            # Android 保持 -glibc 后缀
            echo "${base_name}-glibc"
            ;;
        linux)
            # Linux 使用标准命名
            echo "$(cross_map_lib_to_linux "$base_name")"
            ;;
        windows)
            # Windows MSYS2 使用 mingw-w64-<arch>-xxx 前缀
            local win_prefix=""
            case "${CROSS_ARCH}" in
                x86_64) win_prefix="mingw-w64-x86_64-" ;;
                i686)   win_prefix="mingw-w64-i686-" ;;
                *)      win_prefix="mingw-w64-x86_64-" ;;
            esac
            echo "${win_prefix}${base_name}"
            ;;
        darwin)
            # macOS Homebrew 使用标准命名
            echo "$(cross_map_lib_to_darwin "$base_name")"
            ;;
        bsd)
            # BSD ports 标准命名
            echo "$(cross_map_lib_to_bsd "$base_name")"
            ;;
        *)
            echo "$base_name"
            ;;
    esac
}

# ---------------------------------------------------------------------------
# 2. Linux 发行版映射表
# ---------------------------------------------------------------------------
cross_map_lib_to_linux() {
    local lib="$1"

    # 常见库的通用映射
    case "$lib" in
        # 核心库
        ncurses)    echo "libncurses6" ;;
        readline)   echo "libreadline8" ;;
        libevent)   echo "libevent-2.1-7" ;;
        openssl)    echo "libssl3" ;;
        zlib)       echo "zlib1g" ;;
        libbz2)     echo "libbz2-1.0" ;;
        liblzma|xz) echo "liblzma5" ;;
        libxml2)    echo "libxml2" ;;
        python)     echo "python3" ;;
        perl)       echo "perl" ;;
        ruby)       echo "ruby" ;;
        lua)        echo "lua5.3" ;;

        # 开发工具
        gcc)        echo "gcc" ;;
        g++)        echo "g++" ;;
        make)       echo "make" ;;
        cmake)      echo "cmake" ;;

        # 默认保持原名
        *)          echo "$lib" ;;
    esac
}

# ---------------------------------------------------------------------------
# 3. macOS Homebrew 映射表
# ---------------------------------------------------------------------------
cross_map_lib_to_darwin() {
    local lib="$1"
    case "$lib" in
        libbz2)     echo "bzip2" ;;
        liblzma|xz) echo "xz" ;;
        openssl)    echo "openssl@3" ;;
        python)     echo "python@3" ;;
        *)          echo "$lib" ;;
    esac
}

# ---------------------------------------------------------------------------
# 4. BSD ports 映射表
# ---------------------------------------------------------------------------
cross_map_lib_to_bsd() {
    local lib="$1"
    case "$lib" in
        libbz2)     echo "bzip2" ;;
        liblzma|xz) echo "xz" ;;
        openssl)    echo "openssl" ;;
        python)     echo "python3" ;;
        ncurses)    echo "ncurses" ;;
        *)          echo "$lib" ;;
    esac
}

# ---------------------------------------------------------------------------
# 5. 检查依赖是否已安装
# ---------------------------------------------------------------------------
cross_check_dep_installed() {
    local dep="$1"
    local native_dep
    native_dep="$(cross_normalize_dep_name "$dep")"

    case "${CROSS_PLATFORM}" in
        android)
            # Termux 使用 dpkg 查询
            if command -v dpkg &>/dev/null; then
                dpkg -s "$native_dep" &>/dev/null && return 0
            fi
            # 或使用 pacman (glibc-packages)
            if command -v pacman &>/dev/null; then
                pacman -Q "$native_dep" &>/dev/null && return 0
            fi
            return 1
            ;;
        linux)
            # Debian/Ubuntu
            if command -v dpkg &>/dev/null; then
                dpkg -s "$native_dep" &>/dev/null && return 0
            fi
            # Arch Linux
            if command -v pacman &>/dev/null; then
                pacman -Q "$native_dep" &>/dev/null && return 0
            fi
            # RHEL/Fedora
            if command -v rpm &>/dev/null; then
                rpm -q "$native_dep" &>/dev/null && return 0
            fi
            return 1
            ;;
        windows)
            # MSYS2 pacman
            if command -v pacman &>/dev/null; then
                pacman -Q "$native_dep" &>/dev/null && return 0
            fi
            return 1
            ;;
        darwin)
            # Homebrew
            if command -v brew &>/dev/null; then
                brew list --versions "$native_dep" &>/dev/null && return 0
            fi
            return 1
            ;;
        bsd)
            # FreeBSD pkg
            if command -v pkg &>/dev/null; then
                pkg info "$native_dep" &>/dev/null && return 0
            fi
            # OpenBSD pkg_info
            if command -v pkg_info &>/dev/null; then
                pkg_info -e "$native_dep" &>/dev/null && return 0
            fi
            return 1
            ;;
        *)
            return 1
            ;;
    esac
}

# ---------------------------------------------------------------------------
# 6. 获取交叉编译需要的 sysroot 路径（如果适用）
# ---------------------------------------------------------------------------
cross_get_sysroot() {
    case "${CROSS_PLATFORM}" in
        android)
            echo "${CROSS_PREFIX}"  # 在 Android 上 prefix 本身就是 sysroot
            ;;
        linux)
            if [[ "${CROSS_CROSS_COMPILING}" == "true" ]]; then
                echo "${CROSS_PREFIX}"
            else
                echo "/"
            fi
            ;;
        windows)
            echo "${CROSS_PREFIX}"
            ;;
        *)
            echo "/"
            ;;
    esac
}
