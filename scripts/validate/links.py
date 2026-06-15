# -*- coding: utf-8 -*-

import os
import re
import sys
from typing import List, Tuple, Set


def find_links(lines: List[str]) -> List[Tuple[int, str]]:
    """Extract all markdown links from the given lines.

    Returns a list of (line_number, url) tuples.
    """
    link_re = re.compile(r'\[([^\]]+)\]\((https?://[^)\s]+)\)')
    found = []
    for i, line in enumerate(lines):
        for m in link_re.finditer(line):
            found.append((i + 1, m.group(2)))
    return found


def find_duplicate_links(lines: List[str]) -> List[str]:
    """Report links that appear multiple times.

    (Some projects consider duplicate links an indication of stale content.)
    """
    seen: Set[str] = set()
    dups: List[str] = []
    for line_num, url in find_links(lines):
        if url in seen:
            dups.append(f'(L{line_num:03d}) duplicate link: {url}')
        else:
            seen.add(url)
    return dups


def main(filename: str, only_duplicate: bool = False) -> None:
    with open(filename, mode='r', encoding='utf-8') as f:
        lines = list(line.rstrip() for line in f)

    duplicate_errs = find_duplicate_links(lines)
    if duplicate_errs:
        for msg in duplicate_errs:
            print(msg)
        sys.exit(1)

    if not only_duplicate:
        # Simple connectivity check: report the unique links that were
        # found, so that a human / a future HTTP-head checker can use
        # the output.  We deliberately do not make network requests in
        # this offline mode.
        unique = {url for _, url in find_links(lines)}
        print(f'Found {len(unique)} unique links in {filename}')
        for url in sorted(unique)[:20]:
            print(f'  - {url}')
        if len(unique) > 20:
            print(f'  ... and {len(unique) - 20} more')


if __name__ == '__main__':
    args = sys.argv[1:]
    only_duplicate = '--only_duplicate_links_checker' in args
    files = [a for a in args if not a.startswith('--')]

    if not files:
        print('Usage: python3 scripts/validate/links.py <file.md> [--only_duplicate_links_checker]')
        sys.exit(1)

    for f in files:
        main(f, only_duplicate=only_duplicate)
