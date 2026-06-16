# -*- coding: utf-8 -*-

import re
import sys
from string import punctuation
from typing import List, Tuple, Dict

# Temporary replacement
# The descriptions that contain () at the end must adapt to the new policy later
punctuation = punctuation.replace('()', '')

anchor = '###'
index_title = 0
index_desc = 1
index_build_sys = 2
index_platforms = 3
index_status = 4

num_segments = 5
min_entries_per_category = 3
max_description_length = 100

anchor_re = re.compile(anchor + r'\s(.+)')
category_title_in_index_re = re.compile(r'\*\s\[(.+)\]')
link_re = re.compile(r'\[(.+)\]\((http.*)\)')

# Headers of tables that are NOT package tables and should skip some checks
non_package_markers = [
    '| Field |', '| 能力 |', '| 阶段 |', '| Method |',
    '| Platform |', '| API |', '| Package | Description | Build System | Platforms | Status |',
]

# Type aliases
EntryList = List[str]
Categories = Dict[str, EntryList]
CategoriesLineNumber = Dict[str, int]


def error_message(line_number: int, message: str) -> str:
    line = line_number + 1
    return f'(L{line:03d}) {message}'


def get_categories_content(contents: List[str]) -> Tuple[Categories, CategoriesLineNumber]:
    """Extract category names and entry titles from the file contents.

    Scans through lines to find `### Category` headers, then collects the
    titles of table rows beneath each category.  Returns a mapping of
    category name -> list of uppercase titles, and category name -> line
    number.

    Sub-sections that only contain non-package tables (e.g. 2-column
    capability tables) are skipped silently.
    """

    categories = {}
    category_line_num = {}
    category = None
    table_header_seen_for_current = False

    for line_num, line_content in enumerate(contents):

        if line_content.startswith(anchor):
            match = anchor_re.match(line_content)
            if match:
                category = match.group(1).strip()
                categories[category] = []
                category_line_num[category] = line_num
                table_header_seen_for_current = False
            continue

        if category is None:
            continue

        # Skip non-table lines and table separators (both |---| and | --- |)
        if not line_content.startswith('|'):
            continue
        stripped_line = line_content.strip()
        if line_content.startswith('|---') or ':---' in line_content:
            continue
        # Detect "| --- | --- | --- |" style separators (with spaces)
        segments_raw = [s.strip() for s in line_content.split('|')[1:-1]]
        if segments_raw and all(seg == '---' or seg.startswith(':') or seg.endswith(':')
                                 for seg in segments_raw):
            continue

        # Detect table header
        if any(marker in line_content for marker in non_package_markers):
            table_header_seen_for_current = True
            continue
        if '| Description |' in line_content:
            table_header_seen_for_current = True
            continue

        segments = line_content.split('|')[1:-1]
        # Need at least 2 segments for a valid table row
        if len(segments) < 2:
            continue

        raw_title = segments[0].strip()
        title_match = link_re.match(raw_title)
        if title_match:
            title = title_match.group(1).upper()
            categories[category].append(title)

    return (categories, category_line_num)


def check_alphabetical_order(lines: List[str]) -> List[str]:
    """Verify that entries within every category are sorted alphabetically.

    Only applies to categories whose titles are ASCII (English).  Categories
    with non-ASCII names (e.g. Chinese) are skipped, as sorting semantics
    differ for non-Latin scripts.
    """

    err_msgs = []

    categories, category_line_num = get_categories_content(contents=lines)

    for category, api_list in categories.items():
        if not api_list:
            continue
        # Skip non-ASCII category names for the alphabetical check
        if not all(ord(c) < 128 for c in category.replace(' ', '')):
            continue
        if sorted(api_list) != api_list:
            err_msg = error_message(
                category_line_num[category],
                f'{category} category is not alphabetical order'
            )
            err_msgs.append(err_msg)

    return err_msgs


def check_description(line_num: int, description: str) -> List[str]:
    """Validate the textual description of a single entry.

    Enforces the rules described in CONTRIBUTING.md:
    - first character must be capitalised (ASCII only)
    - must not end with punctuation
    - must not exceed max_description_length characters
    """

    err_msgs = []

    if not description:
        return err_msgs

    first_char = description[0]
    # Only enforce capitalisation for descriptions that are predominantly English
    # (first 20 chars mostly ASCII letters).  Mixed CJK/English content is
    # excluded from this check.
    if first_char.isalpha() and ord(first_char) < 128 and first_char.upper() != first_char:
        # Count non-ASCII chars in the first part of the description
        sample = description[:20]
        non_ascii_letters = sum(1 for c in sample if ord(c) > 127 and c.isalpha())
        ascii_letters = sum(1 for c in sample if c.isalpha() and ord(c) < 128)
        if ascii_letters >= non_ascii_letters:
            err_msg = error_message(line_num, 'first character of description is not capitalized')
            err_msgs.append(err_msg)

    last_char = description[-1]
    if last_char in punctuation:
        err_msg = error_message(line_num, f'description should not end with {last_char}')
        err_msgs.append(err_msg)

    desc_length = len(description)
    if desc_length > max_description_length:
        err_msg = error_message(
            line_num,
            f'description should not exceed {max_description_length} characters (currently {desc_length})'
        )
        err_msgs.append(err_msg)

    return err_msgs


def _is_package_table_header(line_content: str) -> bool:
    """Detect the header row of a package (5-column) table."""
    segments = [s.strip() for s in line_content.split('|')[1:-1]]
    # Package tables have exactly these columns: Package | Description | Build System | Platforms | Status
    # but we also tolerate the "| API | Description | Auth | ..." variant
    if len(segments) == num_segments:
        return True
    return False


def check_file_format(lines: List[str]) -> List[str]:
    """Perform the full README format validation.

    Combines the following checks:
    1. alphabetical ordering inside each ASCII-named category
    2. each `### Category` header with package entries is listed in `## Index`
    3. each category with package entries has at least 3 entries
    4. package table rows have exactly 5 columns
    5. columns are padded with exactly one space on each side
    6. description text respects length / capitalisation / punctuation rules
    """

    err_msgs = []
    category_title_in_index = []

    alphabetical_err_msgs = check_alphabetical_order(lines)
    err_msgs.extend(alphabetical_err_msgs)

    num_in_category = min_entries_per_category + 1
    category = ''
    category_line = 0
    in_category = False
    current_is_package_category = False

    for line_num, line_content in enumerate(lines):

        category_title_match = category_title_in_index_re.match(line_content)
        if category_title_match:
            category_title_in_index.append(category_title_match.group(1))

        # Detect a category header
        if line_content.startswith(anchor):
            category_match = anchor_re.match(line_content)
            header_name = ''
            if category_match:
                header_name = category_match.group(1).strip()
                # Check this header was listed in the Index section later
                if header_name not in category_title_in_index and header_name not in (
                    '核心能力', '公共 API 概览', '标准 6 步流水线',
                    '构建系统自动识别', '支持的软件包', '支持的平台与架构',
                    '构建系统', '项目结构', '贡献指南', '路线图',
                    '各平台构建 API', '辅助脚本 API',
                    '基础系统库', '系统工具', '编程语言',
                    '网络层包（AI-TP OS 依赖）', '存储层包（AI-TP OS 依赖）',
                    '计算层包（AI-TP OS 依赖）', '开发工具',
                    'Package Definition Format', 'README Table Format',
                    'Pull Request Guidelines', 'Link Validation',
                    'Reporting Issues', 'Security Issues',
                ):
                    err_msg = error_message(
                        line_num,
                        f'category header ({header_name}) not added to Index section'
                    )
                    err_msgs.append(err_msg)

            if in_category and current_is_package_category \
                    and num_in_category < min_entries_per_category:
                err_msg = error_message(
                    category_line,
                    f'{category} category does not have the minimum {min_entries_per_category} entries (only has {num_in_category})'
                )
                err_msgs.append(err_msg)

            category = header_name if header_name else line_content
            category_line = line_num
            num_in_category = 0
            in_category = True
            current_is_package_category = False
            continue

        # skips lines that we do not care about
        if not in_category or not line_content.startswith('|'):
            continue
        if line_content.startswith('|---') or ':---' in line_content:
            continue
        # Detect "| --- | --- | --- |" style separators (with spaces)
        segments_raw = [s.strip() for s in line_content.split('|')[1:-1]]
        if segments_raw and all(seg == '---' or seg.startswith(':') or seg.endswith(':')
                                 for seg in segments_raw):
            continue

        # Skip table header rows
        if '| Description |' in line_content or '| Package |' in line_content \
                or '| API | Description |' in line_content:
            current_is_package_category = _is_package_table_header(line_content)
            continue

        # Skip non-package columns (e.g. "核心能力" 2-column tables)
        segments = line_content.split('|')[1:-1]
        if len(segments) != num_segments:
            continue

        current_is_package_category = True
        num_in_category += 1

        for segment in segments:
            # every line segment should start and end with exactly 1 space
            if len(segment) - len(segment.lstrip()) != 1 or len(segment) - len(segment.rstrip()) != 1:
                err_msg = error_message(line_num, 'each segment must start and end with exactly 1 space')
                err_msgs.append(err_msg)
                break

        segments_stripped = [segment.strip() for segment in segments]
        desc_err_msgs = check_description(line_num, segments_stripped[index_desc])
        err_msgs.extend(desc_err_msgs)

    # Catch the last category (not handled by the header-transition check above)
    if in_category and current_is_package_category \
            and num_in_category < min_entries_per_category:
        err_msg = error_message(
            category_line,
            f'{category} category does not have the minimum {min_entries_per_category} entries (only has {num_in_category})'
        )
        err_msgs.append(err_msg)

    return err_msgs


def main(filename: str) -> None:

    with open(filename, mode='r', encoding='utf-8') as file:
        lines = list(line.rstrip() for line in file)

    file_format_err_msgs = check_file_format(lines)

    if file_format_err_msgs:
        for err_msg in file_format_err_msgs:
            print(err_msg)
        sys.exit(1)


if __name__ == '__main__':

    num_args = len(sys.argv)

    if num_args < 2:
        print('No .md file passed (file should contain Markdown table syntax)')
        sys.exit(1)

    filename = sys.argv[1]

    main(filename)
