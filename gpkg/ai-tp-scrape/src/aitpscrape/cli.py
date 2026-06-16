"""
AI-TP Scrape CLI
"""

import argparse
import sys
import json
from typing import Optional

from . import __version__
from .fetch import FetchNode, FetchConfig
from .parse import ParseNode, ParseConfig
from .graph import GraphExecutor, NodeType, BaseNode, ExecutionContext


def main():
    """Main CLI entry point"""
    parser = argparse.ArgumentParser(
        description="AI-TP Scrape - Lightweight AI-powered web scraping",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    
    parser.add_argument("--version", action="version", version=f"ai-tp-scrape {__version__}")
    
    subparsers = parser.add_subparsers(dest="command", help="Available commands")
    
    # Fetch command
    fetch_parser = subparsers.add_parser("fetch", help="Fetch content from URL")
    fetch_parser.add_argument("url", help="URL to fetch")
    fetch_parser.add_argument("--timeout", type=int, default=30, help="Request timeout")
    fetch_parser.add_argument("--output", "-o", help="Output file")
    fetch_parser.add_argument("--json", action="store_true", help="Output as JSON")
    
    # Parse command
    parse_parser = subparsers.add_parser("parse", help="Parse content into chunks")
    parse_parser.add_argument("input", help="Input file or URL")
    parse_parser.add_argument("--chunk-size", type=int, default=4096, help="Chunk size")
    parse_parser.add_argument("--extract-links", action="store_true", help="Extract links")
    parse_parser.add_argument("--output", "-o", help="Output file")
    parse_parser.add_argument("--json", action="store_true", help="Output as JSON")
    
    # Scrape command (fetch + parse)
    scrape_parser = subparsers.add_parser("scrape", help="Fetch and parse in one command")
    scrape_parser.add_argument("url", help="URL to scrape")
    scrape_parser.add_argument("--chunk-size", type=int, default=4096, help="Chunk size")
    scrape_parser.add_argument("--timeout", type=int, default=30, help="Request timeout")
    scrape_parser.add_argument("--output", "-o", help="Output file")
    scrape_parser.add_argument("--json", action="store_true", help="Output as JSON")
    
    args = parser.parse_args()
    
    if args.command == "fetch":
        cmd_fetch(args)
    elif args.command == "parse":
        cmd_parse(args)
    elif args.command == "scrape":
        cmd_scrape(args)
    else:
        parser.print_help()
        sys.exit(1)


def cmd_fetch(args):
    """Handle fetch command"""
    config = FetchConfig(timeout=args.timeout)
    fetcher = FetchNode(config)
    
    result = fetcher.fetch(args.url, "url")
    
    if not result.is_success:
        print(f"Error: {result.error}", file=sys.stderr)
        sys.exit(1)
    
    output = result.content
    if args.json:
        output = json.dumps({
            "url": result.url,
            "status_code": result.status_code,
            "content": result.content,
            "headers": dict(result.headers),
        }, indent=2)
    
    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(output)
    else:
        print(output)


def cmd_parse(args):
    """Handle parse command"""
    if args.input.startswith(("http://", "https://")):
        config = FetchConfig(timeout=30)
        fetcher = FetchNode(config)
        result = fetcher.fetch(args.input, "url")
        if not result.is_success:
            print(f"Error fetching URL: {result.error}", file=sys.stderr)
            sys.exit(1)
        content = result.content
    else:
        with open(args.input, "r", encoding="utf-8") as f:
            content = f.read()
    
    config = ParseConfig(
        chunk_size=args.chunk_size,
        extract_links=args.extract_links,
    )
    parser = ParseNode(config)
    chunks = parser.parse(content)
    
    if args.json:
        output = json.dumps([
            {
                "content": chunk.content,
                "index": chunk.index,
                "metadata": chunk.metadata,
            }
            for chunk in chunks
        ], indent=2, ensure_ascii=False)
    else:
        lines = []
        for i, chunk in enumerate(chunks):
            lines.append(f"=== Chunk {i+1}/{len(chunks)} ===")
            lines.append(chunk.content)
            lines.append("")
        output = "\n".join(lines)
    
    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(output)
    else:
        print(output)


def cmd_scrape(args):
    """Handle scrape command (fetch + parse)"""
    config = FetchConfig(timeout=args.timeout)
    fetcher = FetchNode(config)
    result = fetcher.fetch(args.url, "url")
    
    if not result.is_success:
        print(f"Error: {result.error}", file=sys.stderr)
        sys.exit(1)
    
    parse_config = ParseConfig(chunk_size=args.chunk_size)
    parser = ParseNode(parse_config)
    chunks = parser.parse(result.content, args.url)
    
    if args.json:
        output = json.dumps({
            "url": args.url,
            "chunks": [
                {
                    "content": chunk.content,
                    "index": chunk.index,
                    "metadata": chunk.metadata,
                }
                for chunk in chunks
            ]
        }, indent=2, ensure_ascii=False)
    else:
        lines = [f"URL: {args.url}", f"Total chunks: {len(chunks)}", ""]
        for i, chunk in enumerate(chunks):
            lines.append(f"=== Chunk {i+1}/{len(chunks)} ===")
            lines.append(chunk.content)
            lines.append("")
        output = "\n".join(lines)
    
    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(output)
    else:
        print(output)


if __name__ == "__main__":
    main()
