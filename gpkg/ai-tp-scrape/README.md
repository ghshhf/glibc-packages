# AI-TP Scrape

Lightweight AI-powered web scraping component for AI-TP OS.

## Features

- **FetchNode**: Fetch content from URLs, local files, JSON, XML, and HTML
- **ParseNode**: Parse and chunk content with intelligent text extraction
- **GraphExecutor**: Node-based execution engine for complex scraping workflows

## Installation

```bash
pip install ai-tp-scrape
```

## Quick Start

```python
from aitpscrape import FetchNode, ParseNode

# Fetch content
fetcher = FetchNode()
result = fetcher.fetch("https://example.com")

# Parse into chunks
if result.is_success:
    parser = ParseNode()
    chunks = parser.parse(result.content)
    for chunk in chunks:
        print(chunk.content)
```

## Components

### FetchNode

```python
from aitpscrape import FetchNode, FetchConfig

config = FetchConfig(timeout=30, user_agent="MyBot/1.0")
fetcher = FetchNode(config)
result = fetcher.fetch("https://example.com", "url")
```

### ParseNode

```python
from aitpscrape import ParseNode, ParseConfig

config = ParseConfig(chunk_size=4096, extract_links=True)
parser = ParseNode(config)
chunks = parser.parse(html_content)
```

### GraphExecutor

```python
from aitpscrape import GraphExecutor, NodeType, BaseNode, ExecutionContext

# Define custom node
class MyNode(BaseNode):
    def _execute(self, ctx: ExecutionContext):
        content = ctx.get("content")
        return content.upper()

# Build graph
executor = GraphExecutor("my_scraper")
executor.add_node(MyNode("transform", NodeType.TRANSFORM, ["content"], "transformed"))
executor.set_entry_point("transform")
result = executor.execute({"content": "hello"})
```

## License

MIT License
