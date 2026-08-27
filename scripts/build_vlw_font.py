#!/usr/bin/env python3
"""Render a TrueType font into the VLW smooth-font format used by LovyanGFX.

The 1.28" build ships data/ui_font.vlw (Noto Sans Bold, 15 px). The 2.8C board
has a 480x480 panel, i.e. twice the pixel density of the original layout, so it
embeds a 30 px cut of the same face instead of scaling the small one up.

Usage:
    python3 scripts/build_vlw_font.py --font NotoSans[wdth,wght].ttf \\
        --variation Bold --size 30 --out data/ui_font_30.vlw

VLW layout (all integers are big endian, 32 bit):
    header : glyph_count, encoder_version, size_px, 0, ascent, descent
    glyphs : unicode, height, width, x_advance, dY, dX, 0   (sorted by unicode)
    pixels : 8-bit alpha bitmaps, glyph order, width*height bytes each
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:  # pragma: no cover - helper script
    sys.exit("Pillow is required: pip install pillow")

# Same coverage as the bundled 15 px font: printable ASCII (no space) + degree.
DEFAULT_CHARS = [chr(c) for c in range(0x21, 0x7F)] + ["\u00b0"]


def render_glyph(font: ImageFont.FreeTypeFont, ch: str, pad: int):
    """Return (width, height, x_advance, dY, dX, alpha_bytes) for one glyph."""
    ascent, descent = font.getmetrics()
    canvas = Image.new("L", (pad * 2 + font.size * 3, pad * 2 + ascent + descent), 0)
    draw = ImageDraw.Draw(canvas)
    draw.text((pad, pad), ch, font=font, fill=255)

    box = canvas.getbbox()
    x_advance = int(round(font.getlength(ch)))
    if box is None:  # blank glyph (e.g. space)
        return 0, 0, x_advance, 0, 0, b""

    left, top, right, bottom = box
    width, height = right - left, bottom - top
    baseline = pad + ascent
    return (
        width,
        height,
        x_advance,
        baseline - top,  # dY: baseline to glyph top
        left - pad,      # dX: left side bearing
        canvas.crop(box).tobytes(),
    )


def build(font_path: Path, size: int, variation: str | None, chars: list[str]) -> bytes:
    font = ImageFont.truetype(str(font_path), size)
    if variation:
        font.set_variation_by_name(variation)
    ascent, descent = font.getmetrics()

    glyphs = []
    for ch in sorted(set(chars)):
        width, height, x_advance, dy, dx, bitmap = render_glyph(font, ch, pad=size)
        if width > 255 or x_advance > 255 or not -128 <= dx <= 127:
            raise ValueError(f"glyph {ch!r} does not fit the VLW 8-bit metrics")
        glyphs.append((ord(ch), height, width, x_advance, dy, dx, bitmap))

    out = bytearray()
    out += struct.pack(">6i", len(glyphs), 11, size, 0, ascent, descent)
    for unicode_, height, width, x_advance, dy, dx, _ in glyphs:
        out += struct.pack(">7i", unicode_, height, width, x_advance, dy, dx, 0)
    for *_, bitmap in glyphs:
        out += bitmap
    return bytes(out)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font", required=True, type=Path, help="source .ttf")
    parser.add_argument("--size", type=int, default=30, help="pixel size")
    parser.add_argument("--variation", default=None,
                        help="named instance of a variable font, e.g. Bold")
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()

    data = build(args.font, args.size, args.variation, DEFAULT_CHARS)
    args.out.write_bytes(data)
    print(f"{args.out}: {len(DEFAULT_CHARS)} glyphs, {len(data)} bytes")


if __name__ == "__main__":
    main()
