# Glibc packages for termux

![Build gpkg](https://github.com/termux-pacman/glibc-packages/workflows/Build%20gpkg/badge.svg)
![Build cgct](https://github.com/termux-pacman/glibc-packages/workflows/Build%20cgct/badge.svg)
![GitHub repo size](https://img.shields.io/github/repo-size/termux-pacman/glibc-packages)

This repository stores and compiles **glibc-based packages** for Termux, providing an alternative to the default bionic libc environment.

## Quick start

Add the repository to your Termux pacman configuration:

```
[gpkg]
Server = https://service.termux-pacman.dev/gpkg/$arch
```

Then install packages:

```bash
pacman -S htop tmux python mesa
```

## Package structure

```
gpkg/              — Main package repository (~250 packages)
  <package>/       — Each package has its own directory
    build.sh       — Package build recipe (required)
    *.patch        — Optional patches
    *.subpackage.sh — Optional subpackage definitions
cgct/              — Cross-compilation toolchain (binutils, GCC, glibc, headers)
scripts-cgct/      — Build scripts for the cross toolchain
.github/workflows/ — CI workflows for building and publishing packages
```

### build.sh format

```bash
TERMUX_PKG_HOMEPAGE=https://example.com
TERMUX_PKG_DESCRIPTION="Description of the package"
TERMUX_PKG_LICENSE="GPL-3.0"
TERMUX_PKG_MAINTAINER="@termux-pacman"
TERMUX_PKG_VERSION=1.2.3
TERMUX_PKG_SRCURL=https://example.com/pkg-${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=abc123...
TERMUX_PKG_DEPENDS="glibc, ncurses-glibc"
```

Key points:
- Dependency names use the `-glibc` suffix (e.g., `ncurses-glibc`, `readline-glibc`)
- Build systems supported: autotools, CMake (`TERMUX_PKG_FORCE_CMAKE=true`), Meson
- See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed guidelines

## Supported architectures

| Architecture | Support |
|-------------|---------|
| aarch64     | ✅ |
| arm         | ✅ |
| x86_64      | ✅ |
| i686        | ✅ |

## Build locally

```bash
# Install build dependencies
./get-build-package.sh

# Build a single package for your architecture
./build-package.sh -I -a x86_64 --format pacman --library glibc <package-name>
```

## Links

- [Termux Pacman](https://termux-pacman.dev/) — package repository and build infrastructure
- [Contribution guide](CONTRIBUTING.md)
- [Security policy](SECURITY.md)
- [Wiki](https://github.com/termux-pacman/glibc-packages/wiki)

## License

[MIT](LICENSE.md) © termux-pacman
