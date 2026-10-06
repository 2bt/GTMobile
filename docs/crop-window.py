#!/usr/bin/env python3
"""Crop a GTMobile canvas shot to the modal window (frame-colored chrome)."""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image

# gui.hpp color::FRAME = mix(DARK_GREY, BLACK, 0.3)
FRAME = (66, 60, 55)
TOL = 8


def window_box(im: Image.Image) -> tuple[int, int, int, int]:
    a = np.asarray(im.convert("RGB"))
    d = np.abs(a.astype(np.int16) - FRAME).sum(axis=2)
    ys, xs = np.where(d <= TOL)
    if len(xs) == 0:
        raise SystemExit("no window frame found (is this a popup shot?)")
    return int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("src")
    p.add_argument("dest")
    args = p.parse_args()
    src, dest = Path(args.src), Path(args.dest)
    im = Image.open(src)
    box = window_box(im)
    dest.parent.mkdir(parents=True, exist_ok=True)
    im.convert("RGB").crop(box).save(dest)
    print(f"{src.name} -> {dest} {box[2]-box[0]}x{box[3]-box[1]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
