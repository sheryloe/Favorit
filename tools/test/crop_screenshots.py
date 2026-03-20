#!/usr/bin/env python3

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Tuple
from PIL import Image


@dataclass
class Target:
    source: Path
    width: int
    height: int


def crop_to_exact(image_path: Path, width: int, height: int) -> Tuple[int, int]:
    im = Image.open(image_path)
    current_w, current_h = im.size

    left = max(0, (current_w - width) // 2)
    top = max(0, (current_h - height) // 2)
    right = left + min(width, current_w)
    bottom = top + min(height, current_h)

    if (current_w, current_h) != (width, height):
        im = im.crop((left, top, right, bottom))
        im.save(image_path)
        return current_w, current_h

    return current_w, current_h


def main() -> int:
    targets = [
        Target(Path("docs/assets/screenshots/favorite-widget-main.png"), 472, 410),
        Target(Path("docs/assets/screenshots/favorite-widget-settings.png"), 940, 620),
    ]

    results = []
    for target in targets:
        if not target.source.exists():
            print(f"missing:{target.source}")
            return 1

        before = Image.open(target.source).size
        after_size = crop_to_exact(target.source, target.width, target.height)
        results.append((str(target.source), before, after_size))

    for source, before, after in results:
        print(f"{source}: {before} -> {after}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
