TERMUX_PKG_HOMEPAGE=https://github.com/ghshhf/ai-tp-scrape
TERMUX_PKG_DESCRIPTION="Lightweight AI-powered web scraping component for AI-TP OS"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION=0.1.0
TERMUX_PKG_GITCLONE="https://github.com/ghshhf/ai-tp-scrape.git"
TERMUX_PKG_DEPENDS="python-glibc, requests-glibc, beautifulsoup4-glibc"
TERMUX_PKG_BUILD_DEPENDS="python-pip-glibc"
TERMUX_PKG_PYTHON_DEPS="requests>=2.25.0, beautifulsoup4>=4.9.0"

termux_step_pre_configure() {
    # Use Python from termux
    export PYTHON=${TERMUX_PREFIX}/bin/python3
}

termux_step_configure() {
    # No configure step needed for pure Python package
    return 0
}

termux_step_build() {
    # Install package in-place for installation step
    cd ${TERMUX_PKG_SRCDIR}
    ${PYTHON} -m pip install --upgrade pip
    ${PYTHON} -m pip wheel . -w dist --no-deps --prefer-binary
}

termux_step_install() {
    # Install the wheel package
    cd ${TERMUX_PKG_SRCDIR}
    ${PYTHON} -m pip install dist/*.whl --force-reinstall --no-deps -t ${TERMUX_PREFIX}/lib/python3.14/site-packages/
    
    # Install CLI script
    mkdir -p ${TERMUX_PREFIX}/bin
    cat > ${TERMUX_PREFIX}/bin/ai-tp-scrape << 'EOFPYTHON'
#!/bin/sh
exec python3 -m aitpscrape.cli "$@"
EOFPYTHON
    chmod +x ${TERMUX_PREFIX}/bin/ai-tp-scrape
}

termux_step_post_massage() {
    # Clean up wheel files
    rm -rf ${TERMUX_PREFIX}/lib/python3.14/site-packages/*.whl
    rm -rf ${TERMUX_PREFIX}/lib/python3.14/site-packages/__pycache__/AITPSCRAPE*
}
