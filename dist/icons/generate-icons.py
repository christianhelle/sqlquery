#!/usr/bin/env python3
"""Generate every raster icon in the repository from the quad disc logo.

``src/resources/logo.svg`` is the design master. There is no SVG rasterizer in
the toolchain, so the geometry constants below mirror that file by hand -- if
you change one, change the other. Everything is drawn at 8x and downscaled with
LANCZOS, which is what gives the wedge edges their antialiasing.

    python dist/icons/generate-icons.py

Requires Pillow. Rewrites icon.ico, icon.icns, the Linux hicolor PNG, and the
two logo.png copies used by the docs site and the Chocolatey package.
"""

import struct
import sys
from pathlib import Path

from PIL import Image, ImageDraw

# --- geometry, on the 256 unit canvas of logo.svg -------------------------

CANVAS = 256.0
CENTRE = 128.0
OUTER_RADIUS = 120.0
HUB_RADIUS = 44.0
CHEVRON = [(116.0, 107.0), (139.0, 128.0), (116.0, 149.0)]
CHEVRON_WIDTH = 12.0

# Clockwise from the top-right quadrant. Each colour nods at the provider it
# stands for without being that vendor's exact brand hex.
WEDGES = [
    ("#1B9AAA", 273.0),  # SQLite
    ("#D9453D", 3.0),    # SQL Server
    ("#4B6BD6", 93.0),   # PostgreSQL
    ("#E8973A", 183.0),  # MySQL / MariaDB
]
HUB_COLOUR = "#16202B"
CHEVRON_COLOUR = "#FFFFFF"

GAP_DEGREES = 3.0
SUPERSAMPLE = 8

# Below this the chevron collapses into a smudge, so the hub shrinks to a plain
# dot and the gaps widen enough to survive the downscale.
SIMPLIFY_AT_OR_BELOW = 24
SMALL_GAP_DEGREES = 5.0
SMALL_HUB_RADIUS = 34.0

# --- drawing --------------------------------------------------------------


def render(size):
    """Return an RGBA logo `size` pixels square."""
    simple = size <= SIMPLIFY_AT_OR_BELOW
    gap = SMALL_GAP_DEGREES if simple else GAP_DEGREES

    big = size * SUPERSAMPLE
    scale = big / CANVAS
    image = Image.new("RGBA", (big, big), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)

    centre = CENTRE * scale
    outer = OUTER_RADIUS * scale
    box = [centre - outer, centre - outer, centre + outer, centre + outer]
    for colour, start in WEDGES:
        draw.pieslice(box, start + gap, start + 90.0 - gap, fill=colour)

    hub = (SMALL_HUB_RADIUS if simple else HUB_RADIUS) * scale
    draw.ellipse(
        [centre - hub, centre - hub, centre + hub, centre + hub],
        fill=HUB_COLOUR,
    )

    if not simple:
        points = [(x * scale, y * scale) for x, y in CHEVRON]
        width = CHEVRON_WIDTH * scale
        draw.line(points, fill=CHEVRON_COLOUR, width=round(width), joint="curve")
        # Pillow has no round cap, so cap the stroke by hand.
        for x, y in points:
            draw.ellipse(
                [x - width / 2, y - width / 2, x + width / 2, y + width / 2],
                fill=CHEVRON_COLOUR,
            )

    return image.resize((size, size), Image.Resampling.LANCZOS)


# --- containers -----------------------------------------------------------

ICO_SIZES = [16, 24, 32, 48, 64, 128, 256]

# type -> pixel size. ic11..ic14 are the retina variants of ic04, ic05, ic07
# and ic08, and hold a plain PNG at twice the nominal size.
ICNS_CHUNKS = [
    (b"ic04", 16),
    (b"ic05", 32),
    (b"ic07", 128),
    (b"ic08", 256),
    (b"ic09", 512),
    (b"ic10", 1024),
    (b"ic11", 32),
    (b"ic12", 64),
    (b"ic13", 256),
    (b"ic14", 512),
]


def write_ico(path, frames):
    largest = frames[max(frames)]
    largest.save(
        path,
        format="ICO",
        sizes=[(s, s) for s in sorted(frames)],
        append_images=[frames[s] for s in sorted(frames) if s != max(frames)],
    )


def write_icns(path, frames):
    import io

    chunks = []
    for kind, size in ICNS_CHUNKS:
        buffer = io.BytesIO()
        frames[size].save(buffer, format="PNG")
        payload = buffer.getvalue()
        chunks.append(kind + struct.pack(">I", len(payload) + 8) + payload)

    body = b"".join(chunks)
    path.write_bytes(b"icns" + struct.pack(">I", len(body) + 8) + body)


def main():
    root = Path(__file__).resolve().parents[2]

    wanted = sorted(set(ICO_SIZES) | {size for _, size in ICNS_CHUNKS} | {512})
    frames = {size: render(size) for size in wanted}

    write_ico(root / "src/resources/icon.ico", {s: frames[s] for s in ICO_SIZES})
    write_icns(root / "src/resources/icon.icns", frames)

    frames[256].save(
        root / "src/linux/usr/share/icons/hicolor/256x256/apps/sqlquery.png"
    )
    frames[512].save(root / "images/logo.png")
    frames[512].save(root / "docs/logo.png")

    print("wrote icon.ico, icon.icns, sqlquery.png, images/logo.png, docs/logo.png")


if __name__ == "__main__":
    sys.exit(main())
