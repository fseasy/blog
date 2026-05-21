#!/usr/bin/env python3
"""
从 /bef/ 迁移到 {% include bef.html path='...' %}
Usage: ./scripts_/migrate_bef_links.py [--dry-run]
"""

import argparse
import re
import shutil
from pathlib import Path

POSTS_DIR = (Path(__file__).parent.parent /  "_posts").resolve()


def find_files_with_bef():
    """Find all markdown files containing /bef/ links."""
    files = []
    for md_file in Path(POSTS_DIR).rglob("*.md"):
        content = md_file.read_text()
        if "/bef/" in content:
            files.append(str(md_file))
    return files


def show_bef_links(file_path):
    """Show lines containing /bef/ links."""
    content = Path(file_path).read_text()
    for i, line in enumerate(content.splitlines(), 1):
        if "/bef/" in line:
            print(f"  {i}: {line.rstrip()}")


def replace_bef_links(content):
    """Replace src="/bef/path" with {% include bef.html path='path' %}."""
    pattern = r'src="/bef/([^"]*)"'

    def replacement(m):
        path = m.group(1)
        tmpl = 'src="{% include bef.html path=\'%s\' %}"'
        return tmpl % path

    return re.sub(pattern, replacement, content)


def process_file(file_path, dry_run=True):
    """Process a single file."""
    print("=" * 40)
    print(f"File: {file_path}")
    print("=" * 40)

    print("Lines containing /bef/:")
    show_bef_links(file_path)
    print()

    if dry_run:
        new_content = replace_bef_links(Path(file_path).read_text())
        original = Path(file_path).read_text()
        if new_content != original:
            import difflib
            diff = difflib.unified_diff(
                original.splitlines(keepends=True),
                new_content.splitlines(keepends=True),
                fromfile=file_path,
                tofile=file_path,
            )
            print("Diff:")
            print("".join(diff))
        else:
            print("(no changes)")
        return False

    # Backup
    bak_path = file_path + ".bak"
    shutil.copy2(file_path, bak_path)

    # Replace
    new_content = replace_bef_links(Path(file_path).read_text())
    Path(file_path).write_text(new_content)

    print(f"Replaced (backup: {bak_path})")
    return True


def main():
    parser = argparse.ArgumentParser(description="Migrate /bef/ links to include tag")
    parser.add_argument("--dry-run", action="store_true", help="Show changes without modifying files")
    args = parser.parse_args()

    print("Finding posts with /bef/ links...")
    files = find_files_with_bef()

    if not files:
        print("No /bef/ links found in posts.")
        return

    print("Found /bef/ links in:\n")
    for f in files:
        print(f"  {f}")
    print()

    for file_path in files:
        process_file(file_path, dry_run=args.dry_run)
        if not args.dry_run:
            answer = input("Continue to next file? (y/n): ")
            if answer.lower() != "y":
                break
        print()

    print("Done!")


if __name__ == "__main__":
    main()
