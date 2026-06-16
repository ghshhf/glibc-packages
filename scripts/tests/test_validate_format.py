# -*- coding: utf-8 -*-

import os
import sys
import tempfile

# Ensure the scripts/validate package is importable
SCRIPTS_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'validate')
sys.path.insert(0, os.path.dirname(SCRIPTS_DIR))

from validate.format import (  # noqa: E402
    check_file_format,
    check_description,
    check_alphabetical_order,
    get_categories_content,
)


def _write_temp_file(contents: str) -> str:
    tmp = tempfile.NamedTemporaryFile(mode='w', suffix='.md', delete=False, encoding='utf-8')
    tmp.write(contents)
    tmp.close()
    return tmp.name


def _read_lines(contents: str):
    return [line.rstrip() for line in contents.split('\n')]


# ---------------------------------------------------------------------------
# check_description
# ---------------------------------------------------------------------------

def test_check_description_rejects_trailing_punctuation():
    errs = check_description(1, 'This description ends with a period.')
    assert any('should not end with' in e for e in errs), errs


def test_check_description_accepts_trailing_paren():
    """`()` is explicitly excluded from `punctuation` in format.py."""
    errs = check_description(1, 'Some description (CLI API)')
    assert not any('should not end with' in e for e in errs), errs


def test_check_description_rejects_lowercase_first_letter():
    errs = check_description(1, 'lowercase description here')
    assert any('not capitalized' in e for e in errs), errs


def test_check_description_accepts_capitalized():
    errs = check_description(1, 'Properly formatted description')
    assert errs == []


def test_check_description_rejects_overly_long():
    errs = check_description(1, 'A' * 101)
    assert any('should not exceed' in e for e in errs), errs


def test_check_description_accepts_chinese_without_capitalisation_flag():
    """Chinese descriptions should not trigger the ASCII capitalisation rule."""
    errs = check_description(1, '跨平台构建脚本')
    assert not any('not capitalized' in e for e in errs), errs


# ---------------------------------------------------------------------------
# check_alphabetical_order
# ---------------------------------------------------------------------------

def test_check_alphabetical_order_accepts_sorted_entries():
    content = (
        '### Animals\n'
        '\n'
        '| Package | Description | Build System | Platforms | Status |\n'
        '| --- | --- | --- | --- | --- |\n'
        '| [ant](https://x) | Small insect | custom | All | ✅ |\n'
        '| [bear](https://x) | Large mammal | custom | All | ✅ |\n'
        '| [cat](https://x) | Feline pet | custom | All | ✅ |\n'
    )
    errs = check_alphabetical_order(_read_lines(content))
    assert not any('alphabetical' in e for e in errs), errs


def test_check_alphabetical_order_rejects_unsorted_entries():
    content = (
        '### Animals\n'
        '\n'
        '| Package | Description | Build System | Platforms | Status |\n'
        '| --- | --- | --- | --- | --- |\n'
        '| [cat](https://x) | Feline pet | custom | All | ✅ |\n'
        '| [ant](https://x) | Small insect | custom | All | ✅ |\n'
        '| [bear](https://x) | Large mammal | custom | All | ✅ |\n'
    )
    errs = check_alphabetical_order(_read_lines(content))
    assert any('alphabetical' in e for e in errs), errs


def test_check_alphabetical_order_skips_chinese_category_names():
    content = (
        '### 基础系统库\n'
        '\n'
        '| Package | Description | Build System | Platforms | Status |\n'
        '| --- | --- | --- | --- | --- |\n'
        '| [zlib](https://x) | Compression | configure | All | ✅ |\n'
        '| [ncurses](https://x) | Terminal UI | configure | All | ✅ |\n'
        '| [openssl](https://x) | TLS/SSL | custom | All | ✅ |\n'
    )
    errs = check_alphabetical_order(_read_lines(content))
    assert not any('alphabetical' in e for e in errs), errs


# ---------------------------------------------------------------------------
# check_file_format
# ---------------------------------------------------------------------------

def test_check_file_format_rejects_extra_space_padding():
    content = (
        '## Index\n'
        '\n'
        '* [Animals](#animals)\n'
        '\n'
        '### Animals\n'
        '\n'
        '| Package | Description | Build System | Platforms | Status |\n'
        '| --- | --- | --- | --- | --- |\n'
        '|  [ant](https://x)  | An ant | custom | All | ✅ |\n'
        '| [bear](https://x) | Bear animal | custom | All | ✅ |\n'
        '| [cat](https://x) | Cat animal | custom | All | ✅ |\n'
    )
    errs = check_file_format(_read_lines(content))
    assert any('exactly 1 space' in e for e in errs), errs


def test_check_file_format_rejects_category_with_too_few_entries():
    content = (
        '## Index\n'
        '\n'
        '* [Animals](#animals)\n'
        '\n'
        '### Animals\n'
        '\n'
        '| Package | Description | Build System | Platforms | Status |\n'
        '| --- | --- | --- | --- | --- |\n'
        '| [ant](https://x) | An ant | custom | All | ✅ |\n'
    )
    errs = check_file_format(_read_lines(content))
    assert any('minimum' in e for e in errs), errs


def test_check_file_format_accepts_well_formed_file():
    content = (
        '## Index\n'
        '\n'
        '* [Animals](#animals)\n'
        '\n'
        '### Animals\n'
        '\n'
        '| Package | Description | Build System | Platforms | Status |\n'
        '| --- | --- | --- | --- | --- |\n'
        '| [ant](https://x) | An ant | custom | All | ✅ |\n'
        '| [bear](https://x) | Bear animal | custom | All | ✅ |\n'
        '| [cat](https://x) | Cat animal | custom | All | ✅ |\n'
    )
    errs = check_file_format(_read_lines(content))
    assert errs == [], errs


def test_get_categories_content_extracts_titles():
    content = (
        '### Sample\n'
        '\n'
        '| Package | Description | Build System | Platforms | Status |\n'
        '| --- | --- | --- | --- | --- |\n'
        '| [alpha](https://x) | First entry | custom | All | ✅ |\n'
        '| [beta](https://x) | Second entry | custom | All | ✅ |\n'
        '| [gamma](https://x) | Third entry | custom | All | ✅ |\n'
    )
    categories, _ = get_categories_content(_read_lines(content))
    assert 'Sample' in categories
    assert [t.title() for t in categories['Sample']] == ['Alpha', 'Beta', 'Gamma']


def test_get_categories_content_handles_content_before_first_category():
    content = (
        '# README\n'
        '\n'
        'Some introductory text\n'
        '\n'
        '### First\n'
        '\n'
        '| Package | Description | Build System | Platforms | Status |\n'
        '| --- | --- | --- | --- | --- |\n'
        '| [alpha](https://x) | One entry | custom | All | ✅ |\n'
        '| [beta](https://x) | Two entry | custom | All | ✅ |\n'
        '| [gamma](https://x) | Three entry | custom | All | ✅ |\n'
    )
    # Must not raise an UnboundLocalError
    categories, _ = get_categories_content(_read_lines(content))
    assert 'First' in categories


# ---------------------------------------------------------------------------
# README.md self-test
# ---------------------------------------------------------------------------

def test_project_readme_passes_format_check():
    readme_path = os.path.join(
        os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),
        'README.md'
    )
    with open(readme_path, mode='r', encoding='utf-8') as f:
        lines = [line.rstrip() for line in f]
    errs = check_file_format(lines)
    assert errs == [], f'REREADME.md has formatting issues: {errs}'
