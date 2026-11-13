#!/usr/bin/env python3
"""Rebuild docs/images from the firmware's own drawing code.

Compiles tools/render_previews.cpp, renders every screen, writes one PNG per
screen and composes docs/images/interface-overview.png. Needs g++ and Pillow.

    python3 tools/build_docs_images.py
"""
from pathlib import Path
import re
import subprocess
import tempfile

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs" / "images"
BG, RIM, INK, MUTED, ACCENT = (14, 15, 19), (34, 36, 44), (236, 236, 236), (150, 152, 160), (255, 176, 0)

# (file, caption) in overview order.
SCREENS = [
    ("clock", "CLOCK"), ("menu", "MENU"), ("quick", "QUICK SETTINGS"), ("actions", "RADIAL MENU"),
    ("light", "LIGHT"), ("colors", "COLOURS"), ("thermostat", "THERMOSTAT"), ("heating", "HEATING"),
    ("music", "PLAYER"), ("music-led", "PLAYER · LED ART"), ("now-playing", "SONG BANNER"), ("forecast", "FORECAST"),
    ("plane", "PLANE · DEPARTING"), ("plane-landing", "PLANE · ARRIVING"), ("alerts", "ALERTS"), ("timer", "TIMER"),
    ("status", "STATUS"), ("more", "MORE"), ("lights", "LIGHTS"),
]
NIGHT = [("clock", "CLOCK"), ("music", "PLAYER"), ("thermostat", "THERMOSTAT"), ("quick", "QUICK SETTINGS")]
# Theme colour index (THEME_COLORS order) -> name; every second one of the 20.
THEMES = [(0, "RED"), (2, "ORANGE"), (4, "YELLOW"), (7, "GREEN"), (10, "CYAN"),
          (13, "BLUE"), (15, "VIOLET"), (17, "MAGENTA"), (18, "PINK"), (19, "WHITE")]


def font(size, bold=True):
    for name in ("DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf", "DejaVuSans.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default(size)


def render(binary, out, *args):
    out.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(binary), str(out), *map(str, args)], check=True)


def ball(path, size):
    """One 240x240 frame as a round display of `size` pixels."""
    im = Image.open(path).convert("RGB")
    im = im.resize((size, size), Image.NEAREST if size % 240 == 0 else Image.LANCZOS)
    tile = Image.new("RGB", (size + 12, size + 12), BG)
    ImageDraw.Draw(tile).ellipse((0, 0, size + 11, size + 11), fill=RIM)
    mask = Image.new("L", (size, size), 0)
    ImageDraw.Draw(mask).ellipse((0, 0, size - 1, size - 1), fill=255)
    tile.paste(im, (6, 6), mask)
    return tile


def row(sheet, draw, tiles, y, size, gap, margin, caption_font):
    for i, (tile, caption) in enumerate(tiles):
        x = margin + i * (size + 12 + gap)
        sheet.paste(tile, (x, y))
        draw.text((x + (size + 12) / 2, y + size + 22), caption, font=caption_font, fill=ACCENT, anchor="mt")


def main():
    version = re.search(r'^\s+version: "(.*)"', (ROOT / "pixizhi.yaml").read_text(), re.M).group(1)
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        binary = tmp / "render_previews"
        subprocess.run(["g++", "-std=c++17", "-O2", f"-I{ROOT}", str(ROOT / "tools" / "render_previews.cpp"),
                        "-o", str(binary)], check=True)
        render(binary, tmp / "day")
        render(binary, tmp / "night", 3, "night")
        for idx, _ in THEMES:
            render(binary, tmp / f"theme{idx}", idx)

        OUT.mkdir(parents=True, exist_ok=True)
        for name, _ in SCREENS:
            Image.open(tmp / "day" / f"{name}.ppm").save(OUT / f"{name}.png", optimize=True)
        for name, _ in NIGHT:
            Image.open(tmp / "night" / f"{name}.ppm").save(OUT / f"night-{name}.png", optimize=True)

        size, gap, margin, cols = 480, 24, 36, 4
        cell = size + 12
        width = margin * 2 + cols * cell + (cols - 1) * gap
        rows = -(-len(SCREENS) // cols)
        small = (width - margin * 2 - 9 * 16) // 10 - 12
        height = 130 + rows * (cell + 70) + 80 + small + 12 + 70 + 80 + cell + 70 + margin
        sheet = Image.new("RGB", (width, height), BG)
        draw = ImageDraw.Draw(sheet)
        draw.text((margin, 34), f"PixiZhi {version} · Soft LED interface", font=font(40), fill=INK)
        draw.text((margin, 88), "Rendered by the firmware's own drawing code (pixizhi_draw.h) at 240x240. "
                  "Fictional sample data and cover art.", font=font(20, False), fill=MUTED)
        y = 130
        cap = font(20)
        for r in range(rows):
            part = SCREENS[r * cols:(r + 1) * cols]
            row(sheet, draw, [(ball(tmp / "day" / f"{n}.ppm", size), c) for n, c in part], y, size, gap, margin, cap)
            y += cell + 70
        draw.text((margin, y + 20), "Theme colours (10 of 20 shown)", font=font(28), fill=INK)
        y += 80
        tiles = [(ball(tmp / f"theme{idx}" / ("clock.ppm" if k % 2 == 0 else "quick.ppm"), small), name)
                 for k, (idx, name) in enumerate(THEMES)]
        row(sheet, draw, tiles, y, small, 16, margin, font(16))
        y += small + 12 + 70
        draw.text((margin, y + 20), "Night mode (everything red, any theme colour)", font=font(28), fill=INK)
        y += 80
        row(sheet, draw, [(ball(tmp / "night" / f"{n}.ppm", size), c) for n, c in NIGHT], y, size, gap, margin, cap)
        sheet.save(OUT / "interface-overview.png", optimize=True)
    print(f"Wrote {len(SCREENS) + len(NIGHT) + 1} images to {OUT.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
