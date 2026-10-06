#!/usr/bin/env python3
"""Slice GUI icons from assets/gui.png using enum class Icon in src/gui.hpp."""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GUI_HPP = ROOT / "src" / "gui.hpp"
ATLAS = ROOT / "assets" / "gui.png"
OUT_DIR = Path(__file__).resolve().parent / "assets" / "icons"

ENUM_RE = re.compile(r"enum class Icon\s*\{([^}]+)\}", re.S)
ENTRY_RE = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)(?:\s*=\s*(\d+))?")


def parse_icons(text: str) -> list[tuple[str, int]]:
    m = ENUM_RE.search(text)
    if not m:
        raise SystemExit("enum class Icon not found in gui.hpp")
    value = None
    icons: list[tuple[str, int]] = []
    for raw in m.group(1).splitlines():
        line = raw.split("//", 1)[0].strip().rstrip(",")
        if not line:
            continue
        em = ENTRY_RE.match(line)
        if not em:
            raise SystemExit(f"unparsed Icon line: {raw!r}")
        name = em.group(1)
        if em.group(2) is not None:
            value = int(em.group(2))
        else:
            if value is None:
                raise SystemExit(f"Icon {name} has no value")
            value += 1
        icons.append((name, value))
    return icons


def self_check(icons: list[tuple[str, int]]) -> None:
    by_name = dict(icons)
    assert by_name["Decrease"] == 144, by_name.get("Decrease")
    assert by_name["AddRowAbove"] == 160
    assert by_name["Copy"] == 208
    assert by_name["Noise"] == 224
    assert by_name["Highpass"] == 234
    print(f"ok: {len(icons)} icons")


def crop_icon(name: str, index: int) -> Path:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    dest = OUT_DIR / (name.lower() + ".png")
    x = (index % 16) * 16
    y = (index // 16) * 16
    geom = f"16x16+{x}+{y}"
    cmds = [
        ["magick", str(ATLAS), "-crop", geom, "+repage", str(dest)],
        ["convert", str(ATLAS), "-crop", geom, "+repage", str(dest)],
    ]
    last_err = None
    for cmd in cmds:
        try:
            subprocess.run(cmd, check=True, capture_output=True)
            return dest
        except (FileNotFoundError, subprocess.CalledProcessError) as e:
            last_err = e
    raise SystemExit(f"ImageMagick crop failed for {name}: {last_err}")


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--check", action="store_true", help="parse and assert known indices")
    args = p.parse_args()
    icons = parse_icons(GUI_HPP.read_text())
    self_check(icons)
    if args.check:
        return 0
    if not ATLAS.is_file():
        raise SystemExit(f"missing atlas {ATLAS}")
    for name, index in icons:
        crop_icon(name, index)
        print(name.lower())
    return 0


if __name__ == "__main__":
    sys.exit(main())
