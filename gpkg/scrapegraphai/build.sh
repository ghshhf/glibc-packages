#!/bin/bash
# =============================================================================
# scrapegraphai — AI 驱动的网页抓取组件
# 属于 glibc-packages 跨平台生态的"爬虫"插件/组件
#
# 定位：
#   这是一个标准组件/插件，对接 glibc-packages 底层系统。
#   同时它也能脱离系统独立使用（纯 Python 库，pip install 即用）。
#
# 工作模式（二合一）：
#   [独立模式]  bash build.sh          → 直接 pip install 到当前环境
#   [系统模式]  ./build-cross.sh gpkg/scrapegraphai → 走 6 步标准流水线
#
# 组件化设计：
#   ┌──────────────────────────────────────────────────┐
#   │  scrapegraphai 组件                               │
#   │  ├── 核心层: scrapegraphai (Python 爬虫库)        │
#   │  ├── 接口层: build.sh (对接 glibc-packages 构建)   │
#   │  ├── 元数据: component.yaml (组件清单/能力声明)   │
#   │  └── 依赖链: python-glibc → python-pip → 本组件  │
#   └──────────────────────────────────────────────────┘
# =============================================================================

set -e

# ---------------------------------------------------------------------------
# 一、组件元数据（也是 glibc-packages 的包定义）
# ---------------------------------------------------------------------------
TERMUX_PKG_HOMEPAGE=https://scrapegraphai.com/
TERMUX_PKG_DESCRIPTION="[插件/组件] AI网页爬虫 — 基于LangChain + LLM图逻辑的智能抓取管道，支持OpenAI/Groq/Gemini/Ollama"
TERMUX_PKG_LICENSE="MIT"
TERMUX_PKG_MAINTAINER="@ghshhf"
TERMUX_PKG_VERSION=2.1.3
TERMUX_PKG_SRCURL=https://github.com/ghshhf/Scrapegraph-ai/archive/refs/tags/v${TERMUX_PKG_VERSION}.tar.gz
TERMUX_PKG_SHA256=87a30ca7d3d8eaab32349ae768090ddfa1abbe2f9f7084043c2b2fd8c8e6f8b3
TERMUX_PKG_DEPENDS="python-glibc, python-pip"
TERMUX_PKG_BUILD_IN_SRC=true
TERMUX_PKG_PLATFORM_INDEPENDENT=true

# 可选的建议组件（按需安装）
# - ollama:  本地大模型推理（运行 ollama pull llama3.2）
# - playwright: 浏览器自动化（安装后执行 python -m playwright install chromium）

# ---------------------------------------------------------------------------
# 二、构建流程 — 这是组件的核心行为定义
# ---------------------------------------------------------------------------

# 2a. 准备阶段：校验环境、安装系统级构建依赖
termux_step_pre_configure() {
	echo "  [scrapegraphai] 检测 Python 环境..."
	python3 --version
	pip3 --version

	echo "  [scrapegraphai] 确保 pip 是最新..."
	pip3 install --upgrade pip --quiet 2>/dev/null || true
}

# 2b. 配置阶段：Python 纯包，无需配置
termux_step_configure() {
	echo "  [scrapegraphai] 纯 Python 包，跳过编译配置"
}

# 2c. 构建阶段：Python 包无需编译
termux_step_make() {
	echo "  [scrapegraphai] 纯 Python 包，跳过编译步骤"
}

# 2d. 安装阶段：pip 安装到目标前缀
#     — 在独立模式下，$TERMUX_PREFIX 不存在，回退到系统 Python
#     — 在系统模式下，框架自动设为 ${CROSS_DESTDIR}${CROSS_PREFIX}
termux_step_make_install() {
	local _pip_target="${TERMUX_PREFIX:-/usr/local}"

	echo "  [scrapegraphai] pip install → ${_pip_target}"
	pip install . --prefix="${_pip_target}" --no-cache-dir

	echo "  [scrapegraphai] ✓ 安装完成"
}

# 2e. 安装后处理
termux_step_post_make_install() {
	local _prefix="${TERMUX_PREFIX:-/usr/local}"

	# 安装许可证文件
	if [[ -f LICENSE ]]; then
		install -vDm 644 LICENSE -t "${_prefix}/share/licenses/scrapegraphai/" 2>/dev/null || true
	fi

	echo "  [scrapegraphai] 许可证已安装"
}

# ---------------------------------------------------------------------------
# 三、独立模式入口（不通过 build-cross.sh 时的直接调用）
#     同时也是 glibc-packages 插件的 standalone 能力证明
# ---------------------------------------------------------------------------
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
	echo ""
	echo "╔══════════════════════════════════════════════════════════════╗"
	echo "║  scrapegraphai 组件 — 独立安装模式                           ║"
	echo "║  版本: ${TERMUX_PKG_VERSION}                                ║"
	echo "╚══════════════════════════════════════════════════════════════╝"
	echo ""

	# 检查 Python
	if ! command -v python3 &>/dev/null; then
		echo "错误: 需要 Python 3.12+，请先安装 Python"
		exit 1
	fi

	# 直接安装到系统
	pip install .

	# 安装完成提示
	echo ""
	echo "╔══════════════════════════════════════════════════════════════╗"
	echo "║  scrapegraphai 组件安装成功！                                ║"
	echo "╠══════════════════════════════════════════════════════════════╣"
	echo "║  [独立使用] 直接作为爬虫库调用:                              ║"
	echo "║    python -c \"from scrapegraphai.graphs import SmartScraperGraph\" ║"
	echo "║                                                              ║"
	echo "║  [系统集成] 作为 glibc-packages 组件使用:                    ║"
	echo "║    ./build-cross.sh gpkg/scrapegraphai                      ║"
	echo "║                                                              ║"
	echo "║  [安装浏览器] (抓取 JS 页面需要):                            ║"
	echo "║    python -m playwright install chromium                     ║"
	echo "║                                                              ║"
	echo "║  [可选] 本地 LLM 支持:                                      ║"
	echo "║    pip install ollama  # 或通过 glibc-packages 安装          ║"
	echo "╚══════════════════════════════════════════════════════════════╝"
fi
