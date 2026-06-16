"""
FetchNode Module - Core web fetching functionality
"""

import json
from typing import List, Optional, Dict, Any
from urllib.parse import urljoin, urlparse
from dataclasses import dataclass
import re

try:
    import requests
except ImportError:
    requests = None

try:
    from bs4 import BeautifulSoup
except ImportError:
    BeautifulSoup = None


@dataclass
class FetchConfig:
    """Configuration for FetchNode"""
    headless: bool = True
    timeout: int = 30
    user_agent: str = "AI-TP-Scrape/0.1"
    headers: Optional[Dict[str, str]] = None
    cookies: Optional[Dict[str, str]] = None
    proxy: Optional[str] = None
    verify_ssl: bool = True


@dataclass
class FetchResult:
    """Result of a fetch operation"""
    url: str
    content: str
    status_code: int
    headers: Dict[str, str]
    error: Optional[str] = None
    
    @property
    def is_success(self) -> bool:
        return self.error is None and self.status_code == 200
    
    @property
    def content_type(self) -> str:
        return self.headers.get("Content-Type", "").split(";")[0].strip()


class FetchNode:
    """
    A node responsible for fetching content from various sources.
    Supports URLs, local files, JSON, XML, and HTML content.
    
    Attributes:
        config (FetchConfig): Configuration for fetching
        chunk_size (int): Size of chunks for splitting content
        
    Example:
        >>> node = FetchNode()
        >>> result = node.fetch("https://example.com")
        >>> print(result.content)
    """
    
    URL_PATTERN = re.compile(
        r"https?://[^\s<>\"]+|www\.[^\s<>\"]+"
    )
    
    def __init__(self, config: Optional[FetchConfig] = None):
        if requests is None:
            raise ImportError("requests is required. Install with: pip install requests")
        
        self.config = config or FetchConfig()
        self.logger = self._setup_logger()
    
    def _setup_logger(self):
        """Setup simple logger"""
        import logging
        logger = logging.getLogger("aitpscrape.fetch")
        if not logger.handlers:
            handler = logging.StreamHandler()
            handler.setFormatter(logging.Formatter(
                "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
            ))
            logger.addHandler(handler)
            logger.setLevel(logging.INFO)
        return logger
    
    def fetch(self, source: str, source_type: str = "url") -> FetchResult:
        """
        Fetch content from a source.
        
        Args:
            source: The source to fetch from (URL, file path, or content)
            source_type: Type of source ("url", "html", "json", "xml", "file")
            
        Returns:
            FetchResult: The fetched content and metadata
        """
        handlers = {
            "url": self._fetch_url,
            "html": self._fetch_html,
            "json": self._fetch_json,
            "xml": self._fetch_xml,
            "file": self._fetch_file,
        }
        
        handler = handlers.get(source_type)
        if handler is None:
            return FetchResult(
                url=source,
                content="",
                status_code=0,
                headers={},
                error=f"Unsupported source type: {source_type}"
            )
        
        return handler(source)
    
    def _fetch_url(self, url: str) -> FetchResult:
        """Fetch content from a URL"""
        headers = {
            "User-Agent": self.config.user_agent,
        }
        if self.config.headers:
            headers.update(self.config.headers)
        
        kwargs = {
            "headers": headers,
            "timeout": self.config.timeout,
            "verify": self.config.verify_ssl,
        }
        
        if self.config.cookies:
            kwargs["cookies"] = self.config.cookies
        
        if self.config.proxy:
            kwargs["proxies"] = {"http": self.config.proxy, "https": self.config.proxy}
        
        try:
            response = requests.get(url, **kwargs)
            return FetchResult(
                url=url,
                content=response.text,
                status_code=response.status_code,
                headers=dict(response.headers),
            )
        except requests.exceptions.Timeout:
            return FetchResult(
                url=url,
                content="",
                status_code=0,
                headers={},
                error=f"Request timeout after {self.config.timeout} seconds"
            )
        except requests.exceptions.RequestException as e:
            return FetchResult(
                url=url,
                content="",
                status_code=0,
                headers={},
                error=f"Request failed: {str(e)}"
            )
    
    def _fetch_html(self, html: str) -> FetchResult:
        """Process HTML content"""
        return FetchResult(
            url="",
            content=html,
            status_code=200,
            headers={"Content-Type": "text/html"},
        )
    
    def _fetch_json(self, source: str) -> FetchResult:
        """Fetch and parse JSON"""
        if source.startswith("{") or source.startswith("["):
            try:
                json.loads(source)
                return FetchResult(
                    url="",
                    content=source,
                    status_code=200,
                    headers={"Content-Type": "application/json"},
                )
            except json.JSONDecodeError:
                return FetchResult(
                    url="",
                    content="",
                    status_code=0,
                    headers={},
                    error="Invalid JSON content"
                )
        
        try:
            data = json.loads(source)
            return FetchResult(
                url=source,
                content=json.dumps(data, indent=2),
                status_code=200,
                headers={"Content-Type": "application/json"},
            )
        except (json.JSONDecodeError, FileNotFoundError) as e:
            return FetchResult(
                url=source,
                content="",
                status_code=0,
                headers={},
                error=f"Failed to load JSON: {str(e)}"
            )
    
    def _fetch_xml(self, source: str) -> FetchResult:
        """Fetch and validate XML"""
        if source.startswith("<?xml") or source.startswith("<"):
            return FetchResult(
                url="",
                content=source,
                status_code=200,
                headers={"Content-Type": "application/xml"},
            )
        
        try:
            with open(source, "r", encoding="utf-8") as f:
                content = f.read()
            return FetchResult(
                url=source,
                content=content,
                status_code=200,
                headers={"Content-Type": "application/xml"},
            )
        except FileNotFoundError as e:
            return FetchResult(
                url=source,
                content="",
                status_code=0,
                headers={},
                error=f"File not found: {str(e)}"
            )
    
    def _fetch_file(self, file_path: str) -> FetchResult:
        """Fetch content from a local file"""
        try:
            with open(file_path, "r", encoding="utf-8") as f:
                content = f.read()
            
            content_type = "text/plain"
            if file_path.endswith(".html"):
                content_type = "text/html"
            elif file_path.endswith(".json"):
                content_type = "application/json"
            elif file_path.endswith(".xml"):
                content_type = "application/xml"
            
            return FetchResult(
                url=file_path,
                content=content,
                status_code=200,
                headers={"Content-Type": content_type},
            )
        except FileNotFoundError:
            return FetchResult(
                url=file_path,
                content="",
                status_code=0,
                headers={},
                error=f"File not found: {file_path}"
            )
        except Exception as e:
            return FetchResult(
                url=file_path,
                content="",
                status_code=0,
                headers={},
                error=f"Failed to read file: {str(e)}"
            )
    
    def extract_urls(self, content: str, base_url: Optional[str] = None) -> List[str]:
        """
        Extract URLs from HTML content.
        
        Args:
            content: HTML content to extract URLs from
            base_url: Base URL for resolving relative URLs
            
        Returns:
            List of extracted URLs
        """
        urls = set()
        
        if BeautifulSoup is None:
            for match in self.URL_PATTERN.finditer(content):
                url = match.group()
                if base_url:
                    url = urljoin(base_url, url)
                urls.add(url)
            return list(urls)
        
        soup = BeautifulSoup(content, "html.parser")
        
        for tag in soup.find_all("a", href=True):
            href = tag["href"]
            if base_url:
                href = urljoin(base_url, href)
            if href.startswith("http"):
                urls.add(href)
        
        for tag in soup.find_all("img", src=True):
            src = tag["src"]
            if base_url:
                src = urljoin(base_url, src)
            if src.startswith("http"):
                urls.add(src)
        
        return list(urls)
    
    def clean_html(self, content: str) -> str:
        """
        Clean HTML content by removing scripts, styles, and comments.
        
        Args:
            content: HTML content to clean
            
        Returns:
            Cleaned HTML content
        """
        if BeautifulSoup is None:
            return content
        
        soup = BeautifulSoup(content, "html.parser")
        
        for tag in soup(["script", "style", "noscript"]):
            tag.decompose()
        
        for comment in soup.find_all(string=lambda text: isinstance(text, type(soup.string and ""))):
            if comment.parent.name in ["script", "style"]:
                continue
            if str(comment).strip().startswith("<!--"):
                comment.extract()
        
        return str(soup)
    
    def extract_text(self, html: str, preserve_formatting: bool = False) -> str:
        """
        Extract text content from HTML.
        
        Args:
            html: HTML content
            preserve_formatting: Whether to preserve some formatting (newlines, etc.)
            
        Returns:
            Extracted text content
        """
        if BeautifulSoup is None:
            return html
        
        soup = BeautifulSoup(html, "html.parser")
        
        for tag in soup(["script", "style", "nav", "header", "footer"]):
            tag.decompose()
        
        text = soup.get_text(separator="\n" if preserve_formatting else " ")
        
        if preserve_formatting:
            lines = [line.strip() for line in text.split("\n")]
            text = "\n".join(line for line in lines if line)
        
        return text
