"""
ParseNode Module - Core content parsing and chunking functionality
"""

import re
from typing import List, Optional, Dict, Any, Tuple
from dataclasses import dataclass
from urllib.parse import urljoin, urlparse

try:
    from bs4 import BeautifulSoup
except ImportError:
    BeautifulSoup = None


@dataclass
class ParseConfig:
    """Configuration for ParseNode"""
    chunk_size: int = 4096
    chunk_overlap: int = 200
    extract_links: bool = True
    extract_images: bool = True
    min_chunk_size: int = 100
    split_by: str = "paragraph"  # "paragraph", "sentence", "word", "fixed"


@dataclass
class Chunk:
    """Represents a chunk of content"""
    content: str
    index: int
    start_pos: int
    end_pos: int
    metadata: Dict[str, Any]
    
    def __len__(self) -> int:
        return len(self.content)
    
    def __str__(self) -> str:
        return self.content


class ParseNode:
    """
    A node responsible for parsing and chunking content.
    
    Features:
    - HTML to text conversion
    - Content chunking with overlap
    - URL and image extraction
    - Smart text cleaning
    
    Attributes:
        config (ParseConfig): Configuration for parsing
        
    Example:
        >>> node = ParseNode()
        >>> chunks = node.parse(html_content)
        >>> for chunk in chunks:
        ...     print(chunk.content)
    """
    
    IMAGE_EXTENSIONS = {
        ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp",
        ".svg", ".ico", ".tiff", ".tif"
    }
    
    BLOCK_ELEMENTS = {
        "p", "div", "section", "article", "header", "footer",
        "main", "aside", "nav", "blockquote", "pre", "ul", "ol", "li",
        "h1", "h2", "h3", "h4", "h5", "h6", "table", "tr", "td", "th"
    }
    
    def __init__(self, config: Optional[ParseConfig] = None):
        self.config = config or ParseConfig()
        self.logger = self._setup_logger()
    
    def _setup_logger(self):
        """Setup simple logger"""
        import logging
        logger = logging.getLogger("aitpscrape.parse")
        if not logger.handlers:
            handler = logging.StreamHandler()
            handler.setFormatter(logging.Formatter(
                "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
            ))
            logger.addHandler(handler)
            logger.setLevel(logging.INFO)
        return logger
    
    def parse(self, content: str, source_url: Optional[str] = None) -> List[Chunk]:
        """
        Parse content into chunks.
        
        Args:
            content: Content to parse (HTML, text, etc.)
            source_url: Optional source URL for resolving relative links
            
        Returns:
            List of Chunk objects
        """
        text = self._extract_text(content)
        text = self._clean_text(text)
        
        chunks = self._split_text(text)
        
        for i, chunk in enumerate(chunks):
            chunk.metadata = {
                "source_url": source_url,
                "chunk_index": i,
                "total_chunks": len(chunks),
                "char_count": len(chunk.content),
                "word_count": len(chunk.content.split()),
            }
        
        if source_url:
            if self.config.extract_links:
                urls = self._extract_urls(text, source_url)
                for chunk in chunks:
                    chunk.metadata["urls"] = urls
            
            if self.config.extract_images:
                images = self._extract_images(text, source_url)
                for chunk in chunks:
                    chunk.metadata["images"] = images
        
        return chunks
    
    def _extract_text(self, content: str) -> str:
        """Extract text from HTML or return as-is"""
        if BeautifulSoup is None:
            return content
        
        if "<" in content and ">" in content:
            soup = BeautifulSoup(content, "html.parser")
            
            for tag in soup(["script", "style", "noscript", "iframe"]):
                tag.decompose()
            
            text_parts = []
            for element in soup.descendants:
                if element.name in self.BLOCK_ELEMENTS:
                    text_parts.append("\n")
                if isinstance(element, str):
                    text = element.strip()
                    if text:
                        text_parts.append(text)
            
            return " ".join(text_parts)
        
        return content
    
    def _clean_text(self, text: str) -> str:
        """Clean and normalize text"""
        text = re.sub(r"\s+", " ", text)
        text = re.sub(r"[\x00-\x08\x0b-\x0c\x0e-\x1f\x7f]", "", text)
        text = text.strip()
        return text
    
    def _split_text(self, text: str) -> List[Chunk]:
        """Split text into chunks based on configuration"""
        if len(text) <= self.config.chunk_size:
            return [Chunk(
                content=text,
                index=0,
                start_pos=0,
                end_pos=len(text),
                metadata={}
            )]
        
        if self.config.split_by == "paragraph":
            return self._split_by_paragraph(text)
        elif self.config.split_by == "sentence":
            return self._split_by_sentence(text)
        elif self.config.split_by == "word":
            return self._split_by_word(text)
        else:
            return self._split_fixed(text)
    
    def _split_by_paragraph(self, text: str) -> List[Chunk]:
        """Split text by paragraphs"""
        paragraphs = re.split(r"\n\s*\n", text)
        chunks = []
        current_chunk = ""
        current_pos = 0
        chunk_index = 0
        
        for para in paragraphs:
            para = para.strip()
            if not para:
                continue
            
            if len(current_chunk) + len(para) + 1 <= self.config.chunk_size:
                if current_chunk:
                    current_chunk += "\n\n"
                current_chunk += para
            else:
                if current_chunk:
                    chunks.append(Chunk(
                        content=current_chunk,
                        index=chunk_index,
                        start_pos=current_pos,
                        end_pos=current_pos + len(current_chunk),
                        metadata={}
                    ))
                    chunk_index += 1
                    current_pos += len(current_chunk) + 2
                
                if len(para) <= self.config.chunk_size:
                    current_chunk = para
                else:
                    sub_chunks = self._split_by_sentence(para)
                    for sub in sub_chunks:
                        sub.index = chunk_index
                        chunks.append(sub)
                        chunk_index += 1
                    current_chunk = ""
        
        if current_chunk:
            chunks.append(Chunk(
                content=current_chunk,
                index=chunk_index,
                start_pos=current_pos,
                end_pos=current_pos + len(current_chunk),
                metadata={}
            ))
        
        return chunks
    
    def _split_by_sentence(self, text: str) -> List[Chunk]:
        """Split text by sentences"""
        sentence_pattern = r'(?<=[.!?])\s+'
        sentences = re.split(sentence_pattern, text)
        
        chunks = []
        current_chunk = ""
        current_pos = 0
        chunk_index = 0
        
        for sentence in sentences:
            sentence = sentence.strip()
            if not sentence:
                continue
            
            if len(current_chunk) + len(sentence) + 1 <= self.config.chunk_size:
                if current_chunk:
                    current_chunk += " "
                current_chunk += sentence
            else:
                if current_chunk:
                    chunks.append(Chunk(
                        content=current_chunk,
                        index=chunk_index,
                        start_pos=current_pos,
                        end_pos=current_pos + len(current_chunk),
                        metadata={}
                    ))
                    chunk_index += 1
                    current_pos += len(current_chunk) + 1
                
                if len(sentence) <= self.config.chunk_size:
                    current_chunk = sentence
                else:
                    sub_chunks = self._split_by_word(sentence)
                    for sub in sub_chunks:
                        sub.index = chunk_index
                        chunks.append(sub)
                        chunk_index += 1
                    current_chunk = ""
        
        if current_chunk:
            chunks.append(Chunk(
                content=current_chunk,
                index=chunk_index,
                start_pos=current_pos,
                end_pos=current_pos + len(current_chunk),
                metadata={}
            ))
        
        return chunks
    
    def _split_by_word(self, text: str) -> List[Chunk]:
        """Split text by words with overlap"""
        words = text.split()
        chunks = []
        
        if not words:
            return [Chunk(content="", index=0, start_pos=0, end_pos=0, metadata={})]
        
        chunk_index = 0
        start_idx = 0
        
        while start_idx < len(words):
            end_idx = start_idx
            chunk_words = []
            chunk_start_pos = 0
            
            for i in range(start_idx, len(words)):
                if sum(len(w) for w in chunk_words) + len(chunk_words) + len(words[i]) <= self.config.chunk_size:
                    chunk_words.append(words[i])
                    end_idx = i
                else:
                    break
            
            chunk_text = " ".join(chunk_words)
            chunks.append(Chunk(
                content=chunk_text,
                index=chunk_index,
                start_pos=chunk_start_pos,
                end_pos=chunk_start_pos + len(chunk_text),
                metadata={}
            ))
            chunk_index += 1
            
            start_idx = end_idx + 1
            if start_idx >= len(words):
                break
            
            start_idx = max(start_idx, end_idx - self._count_words_overlap())
        
        return chunks
    
    def _split_fixed(self, text: str) -> List[Chunk]:
        """Split text into fixed-size chunks"""
        chunks = []
        chunk_index = 0
        
        for i in range(0, len(text), self.config.chunk_size - self.config.chunk_overlap):
            chunk_text = text[i:i + self.config.chunk_size]
            chunks.append(Chunk(
                content=chunk_text,
                index=chunk_index,
                start_pos=i,
                end_pos=min(i + self.config.chunk_size, len(text)),
                metadata={}
            ))
            chunk_index += 1
            
            if i + self.config.chunk_size >= len(text):
                break
        
        return chunks
    
    def _count_words_overlap(self) -> int:
        """Calculate number of words in overlap region"""
        if self.config.chunk_overlap <= 0:
            return 0
        return self.config.chunk_overlap // 5
    
    def _extract_urls(self, text: str, base_url: str) -> List[str]:
        """Extract URLs from text"""
        url_pattern = re.compile(
            r'https?://[^\s<>"{}|\\^`\[\]]+|www\.[^\s<>"{}|\\^`\[\]]+'
        )
        
        urls = set()
        for match in url_pattern.finditer(text):
            url = match.group()
            parsed = urlparse(url)
            if not parsed.scheme:
                url = "https://" + url
            urls.add(url)
        
        rel_pattern = re.compile(r'href=["\']([^"\']+)["\']', re.IGNORECASE)
        for match in rel_pattern.finditer(text):
            url = match.group(1)
            if not url.startswith(("http://", "https://", "#", "javascript:")):
                url = urljoin(base_url, url)
                urls.add(url)
        
        return list(urls)
    
    def _extract_images(self, text: str, base_url: str) -> List[str]:
        """Extract image URLs from text"""
        img_pattern = re.compile(
            r'src=["\']([^"\']+\.(?:jpg|jpeg|png|gif|webp|svg|ico)[^"\']*)["\']',
            re.IGNORECASE
        )
        
        images = set()
        for match in img_pattern.finditer(text):
            url = match.group(1)
            if url.startswith("//"):
                url = "https:" + url
            elif url.startswith("/"):
                parsed = urlparse(base_url)
                url = f"{parsed.scheme}://{parsed.netloc}{url}"
            elif not url.startswith(("http://", "https://")):
                url = urljoin(base_url, url)
            images.add(url)
        
        return list(images)
    
    def to_markdown(self, html: str, preserve_formatting: bool = True) -> str:
        """
        Convert HTML to Markdown.
        
        Args:
            html: HTML content
            preserve_formatting: Whether to preserve formatting
            
        Returns:
            Markdown content
        """
        if BeautifulSoup is None:
            return html
        
        soup = BeautifulSoup(html, "html.parser")
        
        markdown = []
        
        for element in soup.find_all(["h1", "h2", "h3", "h4", "h5", "h6"]):
            level = int(element.name[1])
            text = element.get_text().strip()
            markdown.append(f"{'#' * level} {text}\n")
        
        for element in soup.find_all("p"):
            text = element.get_text().strip()
            if text:
                markdown.append(f"{text}\n")
        
        for element in soup.find_all("a", href=True):
            text = element.get_text().strip()
            href = element["href"]
            if text and href:
                markdown.append(f"[{text}]({href})")
        
        for element in soup.find_all("img", src=True):
            alt = element.get("alt", "")
            src = element["src"]
            markdown.append(f"![{alt}]({src})")
        
        for element in soup.find_all("code"):
            text = element.get_text()
            if element.parent.name != "pre":
                markdown.append(f"`{text}`")
        
        for element in soup.find_all("pre"):
            text = element.get_text()
            markdown.append(f"\n```\n{text}\n```\n")
        
        for element in soup.find_all("ul"):
            for li in element.find_all("li", recursive=False):
                text = li.get_text().strip()
                markdown.append(f"- {text}\n")
        
        for element in soup.find_all("ol"):
            for i, li in enumerate(element.find_all("li", recursive=False), 1):
                text = li.get_text().strip()
                markdown.append(f"{i}. {text}\n")
        
        return "\n".join(markdown)
    
    def extract_structured_data(self, html: str) -> Dict[str, Any]:
        """
        Extract structured data from HTML (JSON-LD, microdata, etc.)
        
        Args:
            html: HTML content
            
        Returns:
            Dictionary of extracted structured data
        """
        if BeautifulSoup is None:
            return {}
        
        soup = BeautifulSoup(html, "html.parser")
        result = {}
        
        json_ld = soup.find_all("script", type="application/ld+json")
        for i, script in enumerate(json_ld):
            try:
                import json
                data = json.loads(script.string)
                result[f"json_ld_{i}"] = data
            except (json.JSONDecodeError, TypeError):
                pass
        
        microdata = soup.find_all(attrs={"itemscope": True})
        for item in microdata:
            item_type = item.get("itemtype", "unknown")
            item_props = {}
            for prop in item.find_all(attrs={"itemprop": True}):
                prop_name = prop.get("itemprop")
                prop_value = prop.get("content") or prop.get_text(strip=True)
                item_props[prop_name] = prop_value
            result[f"microdata_{item_type}"] = item_props
        
        return result
