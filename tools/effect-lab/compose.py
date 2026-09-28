"""Builds old-vs-new comparison sheets from effect lab captures.

usage: python compose.py <lab_dir>
<lab_dir>/old/<effect>_<ms>.png and <lab_dir>/new/<effect>_<ms>.png are arranged per effect as
two rows (old on top, new below) with one column per capture time, written to
<lab_dir>/compare/<effect>.png.
"""

import re
import sys
from pathlib import Path

from PIL import Image, ImageDraw

THUMB_WIDTH = 512
LABEL_HEIGHT = 24
ROW_LABEL_WIDTH = 60
CAPTURE = re.compile(r"^(?P<effect>[a-z]+)_(?P<ms>\d+)\.png$")


def collect(directory: Path) -> dict[str, dict[int, Path]]:
    captures: dict[str, dict[int, Path]] = {}
    if not directory.is_dir():
        return captures
    for path in directory.iterdir():
        match = CAPTURE.match(path.name)
        if match:
            captures.setdefault(match["effect"], {})[int(match["ms"])] = path
    return captures


def thumbnail(path: Path | None, height: int) -> Image.Image:
    if path is None:
        placeholder = Image.new("RGB", (THUMB_WIDTH, height), (40, 0, 0))
        ImageDraw.Draw(placeholder).text((10, 10), "missing", fill=(255, 80, 80))
        return placeholder
    image = Image.open(path).convert("RGB")
    return image.resize((THUMB_WIDTH, height), Image.LANCZOS)


def compose(lab_dir: Path) -> None:
    old = collect(lab_dir / "old")
    new = collect(lab_dir / "new")
    out_dir = lab_dir / "compare"
    out_dir.mkdir(exist_ok=True)

    thumb_height = THUMB_WIDTH * 576 // 1024

    for effect in sorted(set(old) | set(new)):
        times = sorted(set(old.get(effect, {})) | set(new.get(effect, {})))
        sheet = Image.new(
            "RGB",
            (ROW_LABEL_WIDTH + THUMB_WIDTH * len(times), LABEL_HEIGHT + 2 * thumb_height),
            (16, 16, 16),
        )
        draw = ImageDraw.Draw(sheet)

        for column, ms in enumerate(times):
            x = ROW_LABEL_WIDTH + column * THUMB_WIDTH
            draw.text((x + 6, 6), f"{effect}  +{ms} ms", fill=(230, 230, 230))
            for row, captures in enumerate((old, new)):
                y = LABEL_HEIGHT + row * thumb_height
                sheet.paste(thumbnail(captures.get(effect, {}).get(ms), thumb_height), (x, y))

        for row, label in enumerate(("old", "new")):
            draw.text((10, LABEL_HEIGHT + row * thumb_height + thumb_height // 2), label, fill=(230, 230, 230))

        target = out_dir / f"{effect}.png"
        sheet.save(target)
        print(f"wrote {target}")


if __name__ == "__main__":
    compose(Path(sys.argv[1] if len(sys.argv) > 1 else "."))
