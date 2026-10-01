#!/usr/bin/env python3
"""Verify that every `STUDIO:` hook in upstream-owned files is listed in STUDIO_HOOKS.md.

Studio code lives in its own directories; any change we make to files owned by
upstream Godot must be marked with a `// STUDIO: <feature>` (or `# STUDIO:`)
comment and listed in STUDIO_HOOKS.md, so upstream merges know what to re-check.

Exit code 0 when consistent, 1 otherwise.
"""

import os
import re
import subprocess
import sys

STUDIO_OWNED_PREFIXES = (
    "editor/studio/",
    "tests/editor/studio/",
    "misc/studio/",
    "docs/",
    ".github/workflows/studio_",
    ".superpowers/",
)
STUDIO_OWNED_FILES = ("STUDIO_HOOKS.md", "STUDIO.md")
MARKER = re.compile(r"(//|#)\s*STUDIO:")
LISTED = re.compile(r"`([^`]+)`")


def main(root):
    files = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard"],
        cwd=root,
        check=True,
        capture_output=True,
        text=True,
    ).stdout.splitlines()

    hooked = set()
    for path in files:
        if path.startswith(STUDIO_OWNED_PREFIXES) or path in STUDIO_OWNED_FILES:
            continue
        full = os.path.join(root, path)
        if not os.path.isfile(full):
            continue
        try:
            with open(full, encoding="utf-8") as f:
                if MARKER.search(f.read()):
                    hooked.add(path)
        except (UnicodeDecodeError, OSError):
            continue

    hooks_md = os.path.join(root, "STUDIO_HOOKS.md")
    listed = set()
    if os.path.isfile(hooks_md):
        with open(hooks_md, encoding="utf-8") as f:
            for line in f:
                match = LISTED.search(line) if line.startswith("|") else None
                if match:
                    listed.add(match.group(1))

    ok = True
    for path in sorted(hooked - listed):
        print(f"ERROR: {path} contains a STUDIO: hook but is not listed in STUDIO_HOOKS.md")
        ok = False
    for path in sorted(listed - hooked):
        print(f"ERROR: {path} is listed in STUDIO_HOOKS.md but contains no STUDIO: hook")
        ok = False
    if ok:
        print(f"OK: {len(hooked)} hooked upstream file(s), all listed in STUDIO_HOOKS.md")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1] if len(sys.argv) > 1 else os.getcwd()))
