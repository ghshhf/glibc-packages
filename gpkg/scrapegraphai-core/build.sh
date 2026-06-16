#!/bin/bash
# =============================================================================
# scrapegraphai-core — 核心爬虫组件
# =============================================================================
# 从 ScrapeGraphAI 中提取的核心爬虫能力（SmartScraperGraph 的精髓）。
#
# 定位：
#   这是 glibc-packages 生态中"最厉害的那个爬虫能力"的组件化封装。
#   它比全量 scrapegraphai 更轻量、更专注：
#     - 依赖仅 requests + beautifulsoup4 + openai
#     - 无需 Playwright / Chromium
#     - 直接复用 ScrapeGraphAI 的提示词工程（核心秘密武器）
#     - 提供 CLI: ai-scrape <url> "<prompt>"
#
# 两种模式：
#   [独立]  bash build.sh              → pip install + 脚本部署
#   [系统]  ./build-cross.sh gpkg/scrapegraphai-core  → 6步流水线
#
# 依赖链：
#   python-glibc ← python-pip ← scrapegraphai-core
#                                        ↑
#                             scrapegraphai (全量库，可选增强)
# =============================================================================

set -e

# ---------------------------------------------------------------------------
# 包元数据
# ---------------------------------------------------------------------------
TERMUX_PKG_HOMEPAGE=https://scrapegraphai.com/
TERMUX_PKG_DESCRIPTION="[核心爬虫] 从 ScrapeGraphAI 提取的轻量智能爬虫引擎 — 一句话从网页提取结构化数据"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION=2.1.3
TERMUX_PKG_SRCURL=https://github.com/ghshhf/Scrapegraph-ai/archive/refs/tags/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=87a30ca7d3d8eaab32349ae768090ddfa1abbe2f9f7084043c2b2fd8c8e6f8b3
TERMUX_PKG_DEPENDS="python-glibc, python-pip"
TERMUX_PKG_BUILD_IN_SRC=true
TERMUX_PKG_PLATFORM_INDEPENDENT=true

# ============================================================================
# 组件文件清单
# 安装时不部署全量 scrapegraphai，而是部署精简版爬虫脚本
# ============================================================================
_CORE_SCRIPT="${TERMUX_PKG_DIR}/src/ai_scrape.py"

# ---------------------------------------------------------------------------
# 准备阶段
# ---------------------------------------------------------------------------
termux_step_pre_configure() {
	echo "  [scrapegraphai-core] 检测 Python 环境..."
	python3 --version
	pip3 --version
}

# ---------------------------------------------------------------------------
# 配置阶段
# ---------------------------------------------------------------------------
termux_step_configure() {
	echo "  [scrapegraphai-core] 纯脚本组件，跳过编译配置"
}

# ---------------------------------------------------------------------------
# 构建阶段
# ---------------------------------------------------------------------------
termux_step_make() {
	echo "  [scrapegraphai-core] 安装运行时依赖..."
	# 仅安装最小依赖（不用全量 scrapegraphai）
	pip install requests beautifulsoup4 openai tiktoken --quiet
}

# ---------------------------------------------------------------------------
# 安装阶段
# ---------------------------------------------------------------------------
termux_step_make_install() {
	local _prefix="${TERMUX_PREFIX:-/usr/local}"
	local _bindir="${_prefix}/bin"
	local _libdir="${_prefix}/lib/scrapegraphai-core"

	echo "  [scrapegraphai-core] 部署核心脚本 → ${_libdir}"
	mkdir -p "${_libdir}" "${_bindir}"

	# 部署核心爬虫库
	cp "${_CORE_SCRIPT}" "${_libdir}/ai_scrape.py"
	chmod 644 "${_libdir}/ai_scrape.py"

	# 创建 CLI 入口
	cat > "${_bindir}/ai-scrape" <<- 'CLI_EOF'
	#!/bin/bash
	exec python3 "$(dirname "$0")/../lib/scrapegraphai-core/ai_scrape.py" "$@"
	CLI_EOF
	chmod 755 "${_bindir}/ai-scrape"

	echo "  [scrapegraphai-core] CLI 入口 → ${_bindir}/ai-scrape"
}

# ---------------------------------------------------------------------------
# 安装后处理
# ---------------------------------------------------------------------------
termux_step_post_make_install() {
	local _prefix="${TERMUX_PREFIX:-/usr/local}"
	# 复制许可证（从 scrapegraph-ai 源码）
	if [[ -f LICENSE ]]; then
		install -vDm 644 LICENSE -t "${_prefix}/share/licenses/scrapegraphai-core/" 2>/dev/null || true
	fi
}

# ============================================================================
# 独立模式（直接 bash build.sh）
# ============================================================================
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
	echo ""
	echo "╔══════════════════════════════════════════════════════════════╗"
	echo "║  scrapegraphai-core — 核心爬虫组件独立安装                   ║"
	echo "║  版本: ${TERMUX_PKG_VERSION}                                ║"
	echo "╚══════════════════════════════════════════════════════════════╝"
	echo ""

	if ! command -v python3 &>/dev/null; then
		echo "错误: 需要 Python 3.8+"
		exit 1
	fi

	echo ">> 安装运行时依赖..."
	pip install requests beautifulsoup4 openai tiktoken --quiet

	local _bindir="${HOME}/.local/bin"
	local _libdir="${HOME}/.local/lib/scrapegraphai-core"
	mkdir -p "${_bindir}" "${_libdir}"

	cp "$(dirname "$0")/src/ai_scrape.py" "${_libdir}/"
	cat > "${_bindir}/ai-scrape" <<- CLI
	#!/bin/bash
	exec python3 "${_libdir}/ai_scrape.py" "\$@"
	CLI
	chmod 755 "${_bindir}/ai-scrape"

	echo ""
	echo "╔══════════════════════════════════════════════════════════════╗"
	echo "║  安装成功！                                                  ║"
	echo "╠══════════════════════════════════════════════════════════════╣"
	echo "║  CLI: ai-scrape <url> \"<prompt>\"                            ║"
	echo "║                                                              ║"
	echo "║  示例:                                                       ║"
	echo "║    export OPENAI_API_KEY=sk-...                              ║"
	echo "║    ai-scrape https://example.com \"提取标题\"                  ║"
	echo "║                                                              ║"
	echo "║  查看帮助:                                                   ║"
	echo "║    ai-scrape --help                                          ║"
	echo "║    ai-scrape --list-models                                   ║"
	echo "╚══════════════════════════════════════════════════════════════╝"
fi
