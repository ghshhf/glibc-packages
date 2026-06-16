#!/usr/bin/env python3
"""
ai-scrape — 核心爬虫引擎（提取自 ScrapeGraphAI SmartScraperGraph）

这是整个爬虫生态中最强的能力：给定一个 URL 和一个自然语言问题，
用 AI 自动提取结构化信息。

工作方式（三步骤）：
  1. fetch   → 用 requests 获取页面 HTML
  2. parse   → 清理 HTML、转文本、按 token 分块
  3. extract → 调用 LLM，用专业爬虫提示词提取 JSON

依赖极轻：requests + beautifulsoup4 + tiktoken + 任意 OpenAI 兼容 API

CLI 用法：
  export OPENAI_API_KEY=sk-...
  ai-scrape https://example.com "提取所有产品名称和价格"
  ai-scrape https://example.com "找到文章标题和作者" --model ollama/llama3.2

也支持作为 Python 库导入：
  from ai_scrape import SmartScraper
  scraper = SmartScraper(model="openai/gpt-4o-mini")
  result = scraper.run("https://...", "提取...")
"""

import argparse
import json
import os
import re
import sys
import textwrap
from typing import Optional


# ============================================================================
# 一、提示词模板（ScrapeGraphAI 的核心秘密武器）
# ============================================================================

PROMPT_NO_CHUNKS = """You are a website scraper and you have just scraped the
following content from a website converted in markdown format.
You are now asked to answer a user question about the content.
If you don't find the answer put as value "NA".
Make sure the output is a valid json format without any errors.
Don't put any words before or after the json.

OUTPUT INSTRUCTIONS: {format_instructions}

USER QUESTION: {question}

WEBSITE CONTENT: {content}"""

PROMPT_CHUNK = """You are a website scraper and you have just scraped the
following content from a website converted in markdown format.
You are now asked to answer a user question based on the content.
Pay attention that the content is part of a larger document so
the answer may not be exhaustive — just extract what you can.
If you don't find the answer put as value "NA".
Make sure the output is a valid json format without any errors.
Don't put any words before or after the json.

OUTPUT INSTRUCTIONS: {format_instructions}

USER QUESTION: {question}

WEBSITE CONTENT (PARTIAL): {content}"""

PROMPT_MERGE = """You are a website scraper and you have just scraped the
following content from a website in multiple chunks.
Each chunk has been processed individually.
You are now asked to merge the results into a single comprehensive answer.
Remove any duplicates and NA values.
Make sure the output is a valid json format without any errors.
Don't put any words before or after the json.

OUTPUT INSTRUCTIONS: {format_instructions}

USER QUESTION: {question}

CHUNK RESULTS: {content}"""


# ============================================================================
# 二、工具函数
# ============================================================================

def fetch_page(url: str, timeout: int = 30) -> str:
    """用 requests 获取页面 HTML（轻量，无需 Playwright）"""
    import requests as req
    headers = {
        "User-Agent": (
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
            "AppleWebKit/537.36 (KHTML, like Gecko) "
            "Chrome/120.0.0.0 Safari/537.36"
        )
    }
    resp = req.get(url, headers=headers, timeout=timeout)
    resp.raise_for_status()
    return resp.text


def html_to_text(html: str) -> str:
    """将 HTML 转为纯文本（用 html2text 风格，但纯 bs4 实现避免额外依赖）"""
    from bs4 import BeautifulSoup
    soup = BeautifulSoup(html, "html.parser")

    # 移除脚本和样式
    for tag in soup(["script", "style", "noscript", "meta", "link"]):
        tag.decompose()

    # 提取文本，保留段落结构
    lines = []
    for el in soup.find_all(["p", "h1", "h2", "h3", "h4", "h5", "h6",
                             "li", "td", "th", "div", "section", "article",
                             "blockquote", "pre"]):
        text = el.get_text(separator=" ", strip=True)
        if text:
            # 标题加标记
            tag = el.name
            if tag.startswith("h") and len(tag) == 2:
                lines.append(f"\n## {text}")
            elif tag == "li":
                lines.append(f"  - {text}")
            else:
                lines.append(text)

    return "\n".join(lines)


def count_tokens(text: str, model: str = "gpt-4o") -> int:
    """估算 token 数（用 tiktoken，如果不可用则用字符估算）"""
    try:
        import tiktoken
        encoding = tiktoken.encoding_for_model(model)
        return len(encoding.encode(text))
    except Exception:
        # 回退：中文 ≈ 1.5 token/字，英文 ≈ 0.25 token/字
        en_chars = len(re.findall(r'[a-zA-Z0-9,.;!?\'\" ]', text))
        cn_chars = len(text) - en_chars
        return int(en_chars * 0.25 + cn_chars * 1.5)


def split_chunks(text: str, chunk_size: int = 4000) -> list[str]:
    """按 token 限制分块（保留段落完整性）"""
    paragraphs = text.split("\n")
    chunks = []
    current = []
    current_tokens = 0

    for para in paragraphs:
        para_tokens = count_tokens(para)
        if current_tokens + para_tokens > chunk_size and current:
            chunks.append("\n".join(current))
            current = [para]
            current_tokens = para_tokens
        else:
            current.append(para)
            current_tokens += para_tokens

    if current:
        chunks.append("\n".join(current))

    return chunks if chunks else [text]


# ============================================================================
# 三、LLM 调用器
# ============================================================================

def call_llm(prompt: str, model: str = "openai/gpt-4o-mini",
             api_key: Optional[str] = None, base_url: Optional[str] = None,
             temperature: float = 0.0) -> str:
    """
    调用 LLM（兼容 OpenAI API 格式）。
    支持模型路径：
      - openai/gpt-4o       → OpenAI
      - ollama/llama3.2     → Ollama 本地
      - deepseek/deepseek-chat → DeepSeek
      - groq/llama-70b      → Groq
    """
    from openai import OpenAI

    provider = model.split("/")[0] if "/" in model else "openai"
    model_name = model.split("/", 1)[1] if "/" in model else model

    # 根据 provider 设置 base_url 和 api_key
    if provider == "ollama":
        client = OpenAI(
            base_url=base_url or os.getenv("OLLAMA_BASE_URL", "http://localhost:11434/v1"),
            api_key=api_key or "ollama",
        )
    elif provider == "deepseek":
        client = OpenAI(
            base_url=base_url or "https://api.deepseek.com/v1",
            api_key=api_key or os.getenv("DEEPSEEK_API_KEY", os.getenv("OPENAI_API_KEY", "")),
        )
    elif provider == "groq":
        client = OpenAI(
            base_url=base_url or "https://api.groq.com/openai/v1",
            api_key=api_key or os.getenv("GROQ_API_KEY", os.getenv("OPENAI_API_KEY", "")),
        )
    else:
        # 默认 OpenAI 兼容
        client = OpenAI(
            base_url=base_url or os.getenv("OPENAI_BASE_URL", "https://api.openai.com/v1"),
            api_key=api_key or os.getenv("OPENAI_API_KEY", ""),
        )

    if not client.api_key:
        print("错误: 未设置 API Key。通过 --api-key 参数或环境变量设置。", file=sys.stderr)
        sys.exit(1)

    resp = client.chat.completions.create(
        model=model_name,
        messages=[{"role": "user", "content": prompt}],
        temperature=temperature,
        response_format={"type": "json_object"},
    )
    return resp.choices[0].message.content


# ============================================================================
# 四、核心爬虫类（SmartScraper 的精髓）
# ============================================================================

class SmartScraper:
    """
    智能爬虫 —— ScrapeGraphAI SmartScraperGraph 的核心提取。

    用法:
      scraper = SmartScraper(model="openai/gpt-4o-mini")
      result = scraper.run("https://example.com", "提取所有文章标题")
    """

    def __init__(self, model: str = "openai/gpt-4o-mini",
                 api_key: Optional[str] = None,
                 base_url: Optional[str] = None,
                 timeout: int = 30,
                 verbose: bool = False):
        self.model = model
        self.api_key = api_key
        self.base_url = base_url
        self.timeout = timeout
        self.verbose = verbose

    def log(self, msg: str):
        if self.verbose:
            print(f"  [scraper] {msg}", file=sys.stderr)

    def run(self, source: str, prompt: str,
            model_override: Optional[str] = None) -> dict:
        """
        执行爬取 + 提取。

        参数:
          source: URL 或文件路径
          prompt: 自然语言描述要提取的内容

        返回:
          结构化 JSON dict
        """
        model = model_override or self.model
        self.log(f"model={model}  source={source}")

        # ── Step 1: 获取内容 ──
        self.log("正在获取页面...")
        html = fetch_page(source, timeout=self.timeout)
        self.log(f"获取完成 ({len(html)} bytes)")

        # ── Step 2: 解析并分块 ──
        self.log("正在解析 HTML → 文本...")
        text = html_to_text(html)
        self.log(f"文本长度: {len(text)} chars")

        # 估算模型 token 限制
        if "gpt-4" in model:
            max_tokens = 8000
        elif "gpt-3.5" in model:
            max_tokens = 4000
        else:
            max_tokens = 4096

        chunk_size = max_tokens - 500  # 留出提示词的 token 空间
        chunks = split_chunks(text, chunk_size=chunk_size)
        self.log(f"分块: {len(chunks)} 块")

        # ── Step 3: LLM 提取 ──
        json_instructions = 'Return a JSON object with key "content".'

        if len(chunks) == 1:
            # 单块：直接提取
            self.log("LLM 提取中（单块）...")
            final_prompt = PROMPT_NO_CHUNKS.format(
                format_instructions=json_instructions,
                question=prompt,
                content=chunks[0],
            )
            raw = call_llm(final_prompt, model=model,
                           api_key=self.api_key, base_url=self.base_url)

        else:
            # 多块：map-reduce
            self.log(f"LLM 提取中（{len(chunks)} 块，map-reduce）...")
            chunk_results = []
            for i, chunk in enumerate(chunks):
                chunk_prompt = PROMPT_CHUNK.format(
                    format_instructions=json_instructions,
                    question=prompt,
                    content=chunk,
                )
                raw = call_llm(chunk_prompt, model=model,
                               api_key=self.api_key, base_url=self.base_url)
                chunk_results.append(raw)
                self.log(f"  块 {i+1}/{len(chunks)} 完成")

            # 合并
            merge_prompt = PROMPT_MERGE.format(
                format_instructions=json_instructions,
                question=prompt,
                content=json.dumps(chunk_results, ensure_ascii=False),
            )
            raw = call_llm(merge_prompt, model=model,
                           api_key=self.api_key, base_url=self.base_url)

        # 解析 JSON 响应
        # 清理可能的 markdown 代码块包裹
        clean = re.sub(r'^```(?:json)?\s*|\s*```$', '', raw.strip())
        try:
            return json.loads(clean)
        except json.JSONDecodeError as e:
            self.log(f"JSON 解析失败: {e}")
            self.log(f"原始响应: {raw[:200]}")
            return {"error": "JSON 解析失败", "raw": raw[:500]}


# ============================================================================
# 五、CLI 入口
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        prog="ai-scrape",
        description="AI 驱动智能爬虫 —— 用自然语言从网页提取结构化信息",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=textwrap.dedent("""\
            示例:
              ai-scrape https://example.com "提取所有产品名称和价格"
              ai-scrape https://news.com "找到文章标题和发布日期" --model ollama/llama3.2
              ai-scrape --list-models
              ai-scrape https://page.com "提取联系方式" --output contacts.json

            环境变量:
              OPENAI_API_KEY    OpenAI / 兼容 API 的 Key
              OPENAI_BASE_URL   API 地址（默认 https://api.openai.com/v1）
              OLLAMA_BASE_URL   Ollama 地址（默认 http://localhost:11434/v1）
        """),
    )
    parser.add_argument("url", nargs="?", help="要爬取的网页 URL")
    parser.add_argument("prompt", nargs="?", help="要提取什么信息（自然语言）")
    parser.add_argument("--model", "-m", default="openai/gpt-4o-mini",
                        help="模型路径，如 openai/gpt-4o, ollama/llama3.2, deepseek/deepseek-chat, groq/llama-70b")
    parser.add_argument("--api-key", help="API Key（默认从环境变量读取）")
    parser.add_argument("--base-url", help="API 地址覆盖")
    parser.add_argument("--timeout", type=int, default=30, help="请求超时（秒）")
    parser.add_argument("--output", "-o", help="输出到文件（默认 stdout）")
    parser.add_argument("--verbose", "-v", action="store_true", help="显示详细日志")
    parser.add_argument("--pretty", "-p", action="store_true", help="格式化 JSON 输出")
    parser.add_argument("--list-models", action="store_true", help="列出支持的模型路径")
    parser.add_argument("--version", action="store_true", help="显示版本")

    args = parser.parse_args()

    if args.version:
        print("ai-scrape v2.1.3-core (提取自 ScrapeGraphAI SmartScraperGraph)")
        return

    if args.list_models:
        print("支持的模型路径（--model 参数）：")
        print()
        models = [
            ("openai/gpt-4o",         "OpenAI GPT-4o（默认需 API Key）"),
            ("openai/gpt-4o-mini",    "OpenAI GPT-4o-mini（推荐，性价比高）"),
            ("openai/gpt-3.5-turbo",  "OpenAI GPT-3.5（旧款）"),
            ("ollama/llama3.2",       "Ollama 本地 LLaMA 3.2"),
            ("ollama/llama3.1",       "Ollama 本地 LLaMA 3.1"),
            ("ollama/qwen2.5",        "Ollama 本地 Qwen 2.5"),
            ("ollama/mistral",        "Ollama 本地 Mistral"),
            ("deepseek/deepseek-chat","DeepSeek Chat"),
            ("groq/llama-70b",        "Groq LLaMA 70B"),
            ("groq/mixtral",          "Groq Mixtral 8x7B"),
        ]
        for model, desc in models:
            print(f"  {model:35s}  {desc}")
        return

    if not args.url or not args.prompt:
        parser.print_help()
        sys.exit(1)

    scraper = SmartScraper(
        model=args.model,
        api_key=args.api_key,
        base_url=args.base_url,
        timeout=args.timeout,
        verbose=args.verbose,
    )

    try:
        result = scraper.run(args.url, args.prompt)
        output = json.dumps(result, ensure_ascii=False,
                            indent=2 if args.pretty else None)

        if args.output:
            with open(args.output, "w", encoding="utf-8") as f:
                f.write(output)
            print(f"结果已写入: {args.output}")
        else:
            print(output)

    except Exception as e:
        print(f"错误: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
