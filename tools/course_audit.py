#!/usr/bin/env python3
"""Lightweight course audit checks for structural/factual drift."""
from __future__ import annotations
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]


def fail(msg: str) -> None:
    print(f"FAIL: {msg}")


def warn(msg: str) -> None:
    print(f"WARN: {msg}")


def iter_markdown_files() -> list[Path]:
    return [p for p in (ROOT / "src").rglob("*.md")] + [ROOT / "README.md"]


def check_placeholders() -> int:
    errors = 0
    targets = [ROOT / "book.toml", ROOT / "README.md", ROOT / "src"]
    rx = re.compile(r"your-org|placeholder|TODO\s*:\s*replace", re.IGNORECASE)
    for t in targets:
        files = [t] if t.is_file() else [p for p in t.rglob("*.md")]
        for f in files:
            text = f.read_text(encoding="utf-8")
            if rx.search(text):
                fail(f"Placeholder text found in {f.relative_to(ROOT)}")
                errors += 1
    return errors


def check_markdown_relative_links() -> int:
    errors = 0
    rx = re.compile(r"\[[^\]]+\]\(([^)]+)\)")
    for md in iter_markdown_files():
        text = md.read_text(encoding="utf-8")
        for target in rx.findall(text):
            if target.startswith(("http://", "https://", "mailto:", "#")):
                continue
            rel = target.split("#", 1)[0]
            if not rel:
                continue
            resolved = (md.parent / rel).resolve()
            if not resolved.exists():
                fail(
                    f"Broken relative markdown link in {md.relative_to(ROOT)} -> {target}"
                )
                errors += 1
    return errors


def parse_canonical_titles() -> dict[int, str]:
    canon: dict[int, str] = {}
    for line in (ROOT / "src" / "SUMMARY.md").read_text(encoding="utf-8").splitlines():
        m = re.search(r"\[Chapter\s+(\d+)\s+—\s+([^\]]+)\]", line)
        if m:
            canon[int(m.group(1))] = m.group(2).strip()
    return canon


def check_chapter_index_titles(canon: dict[int, str]) -> int:
    errors = 0
    for idx in sorted((ROOT / "src").glob("part*/chapter-*/index.md")):
        first = idx.read_text(encoding="utf-8").splitlines()[0].strip()
        m = re.match(r"#\s*Chapter\s+(\d+)\s+—\s+(.+)$", first)
        if not m:
            fail(f"Non-standard chapter heading format in {idx.relative_to(ROOT)}")
            errors += 1
            continue
        number = int(m.group(1))
        title = m.group(2).strip()
        expected = canon.get(number)
        if expected and title != expected:
            fail(
                f"Title mismatch in {idx.relative_to(ROOT)} (got '{title}', expected '{expected}')"
            )
            errors += 1
    return errors


def check_chapter_template_sections() -> int:
    errors = 0
    required = [
        "## Who This Chapter Is For",
        "## Prerequisites",
        "## Learning Objectives",
        "## Key Terms",
        "## Practical Checkpoint",
        "## What to Read Next",
    ]
    for idx in sorted((ROOT / "src").glob("part*/chapter-*/index.md")):
        text = idx.read_text(encoding="utf-8")
        for sec in required:
            if sec not in text:
                fail(f"Missing '{sec}' in {idx.relative_to(ROOT)}")
                errors += 1
    return errors


def check_hardware_readme_references() -> int:
    errors = 0
    readme = ROOT / "hardware/stm32/nucleo-f401re/README.md"
    text = readme.read_text(encoding="utf-8")
    referenced = ["startup/", "linker/", "include/cmsis"]
    existing_paths = {
        p.relative_to(readme.parent).as_posix()
        for p in readme.parent.rglob("*")
        if p.is_dir()
    }
    for ref in referenced:
        if ref.rstrip("/") not in existing_paths and ref in text and "not checked into this directory" not in text:
            fail(f"README references '{ref}' but directory does not exist and isn't marked optional")
            errors += 1
    return errors


def main() -> int:
    total_errors = 0
    canon = parse_canonical_titles()
    if not canon:
        fail("Could not parse chapter titles from src/SUMMARY.md")
        return 1

    total_errors += check_placeholders()
    total_errors += check_chapter_index_titles(canon)
    total_errors += check_chapter_template_sections()
    total_errors += check_hardware_readme_references()
    total_errors += check_markdown_relative_links()

    if total_errors == 0:
        print("PASS: all audit checks passed")
        return 0

    warn(f"Audit completed with {total_errors} issue(s)")
    return 1


if __name__ == "__main__":
    sys.exit(main())
