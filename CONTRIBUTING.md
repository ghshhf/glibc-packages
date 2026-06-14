# Contributing

Thank you for your interest in contributing to `glibc-packages`! There are several ways to help.

## Report a bug

If you find a bug in a package or the build system, open a [bug report](https://github.com/termux-pacman/glibc-packages/issues/new?assignees=&labels=bug&projects=&template=bug-report.yml&title=%5BBug%5D%3A+).

## Request a package

Open a [package request](https://github.com/termux-pacman/glibc-packages/issues/new?assignees=&labels=package+request%2Cgpkg&projects=&template=package_request.yml&title=%5BPackage%5D%3A+).

## Add or update a package

### Build recipe format (`build.sh`)

Each package lives in `gpkg/<name>/build.sh`. Here are the common variables:

```bash
TERMUX_PKG_HOMEPAGE=          # Project homepage URL
TERMUX_PKG_DESCRIPTION=       # Short description
TERMUX_PKG_LICENSE=           # SPDX license identifier (e.g., "GPL-3.0", "MIT")
TERMUX_PKG_MAINTAINER=        # "@termux-pacman" for official packages
TERMUX_PKG_VERSION=           # Upstream version number
TERMUX_PKG_SRCURL=            # Source tarball URL (use ${TERMUX_PKG_VERSION})
TERMUX_PKG_SHA256=            # SHA256 checksum of the source tarball
TERMUX_PKG_DEPENDS=           # Comma-separated runtime dependencies
TERMUX_PKG_BUILD_DEPENDS=     # Comma-separated build-time-only dependencies
TERMUX_PKG_EXTRA_CONFIGURE_ARGS=  # Extra flags for ./configure
TERMUX_PKG_FORCE_CMAKE=true    # Use CMake instead of autotools
TERMUX_PKG_BUILD_IN_SRC=true   # Build in source directory (no separate build dir)
```

### Dependency naming

- Internal packages use the `-glibc` suffix: `ncurses-glibc`, `readline-glibc`, `libevent-glibc`
- Only `glibc` itself has no suffix
- Example: `TERMUX_PKG_DEPENDS="ncurses-glibc, libevent-glibc"`

### Build systems

The build system auto-detects the build method (from most to least common):

| Method | Detection |
|--------|-----------|
| Autotools | `./configure` exists → `./configure && make && make install` |
| CMake | `TERMUX_PKG_FORCE_CMAKE=true` → CMake build |
| Meson | `meson.build` exists → Meson build |
| Custom Makefile | `TERMUX_PKG_BUILD_IN_SRC=true` → override `termux_step_make()` |

If the source ships only `configure.ac` without a pre-generated `configure`, add:

```bash
termux_step_pre_configure() {
    autoreconf -fi
}
```

### Patches

Place `.patch` files in the package directory. They are applied automatically before the build.
Name them descriptively (e.g., `fix-paths.patch`, `setdirs.patch`).

### Subpackages

To split a package into subpackages, create `<name>.subpackage.sh` files in the package directory.
These are automatically detected and built. See `gpkg/mesa/` for examples.

### Testing before submitting

```bash
# Clone the repo and install build scripts
./get-build-package.sh

# Build your package locally (replace x86_64 with your arch)
./build-package.sh -I -a x86_64 --format pacman --library glibc <package-name>
```

### Pull request checklist

- [ ] `build.sh` follows the format above
- [ ] `TERMUX_PKG_SHA256` matches the source tarball
- [ ] Dependencies use the correct `-glibc` naming
- [ ] Patches are minimal and have clear filenames
- [ ] Package builds successfully (CI will verify on all architectures)

## Security issues

Report security vulnerabilities as described in [SECURITY.md](SECURITY.md).

## Donate

Support the project at [termux-pacman.dev/donate](https://termux-pacman.dev/donate/).
