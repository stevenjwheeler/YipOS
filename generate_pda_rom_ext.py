#!/usr/bin/env python3
"""Generate the extended PDA ROM atlas (Bank 1) for the Williams Tube wrist CRT.

Produces a 256x512 RGBA atlas (16x16 grid, each cell 16x32 pixels).

Bank 1 layout:
    0-31:    Accented Latin lowercase (àáâãäåæçèéêëìíîïðñòóôõöøùúûüýþßÿ)
    32-61:   Accented Latin uppercase (ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞ)
    62-65:   Extra Latin (œŒ¿¡)
    66-148:  Hiragana (ぁ-ん, 83 codepoints)
    149-231: Katakana (ァ-ン, 83 codepoints)
    232-238: Japanese punctuation (。、「」・ー～)
    239-255: Reserved

Glyph source: System Unicode font rendered via Pillow, thresholded to 1-bit.

Usage:
    python3 generate_pda_rom_ext.py [--output WilliamsTube_PDARom_Ext.png]
"""

import argparse
import os
import shutil
import sys

from PIL import Image, ImageDraw, ImageFont

CELL_W, CELL_H = 16, 32
GRID_COLS, GRID_ROWS = 16, 16
ATLAS_W = GRID_COLS * CELL_W   # 256
ATLAS_H = GRID_ROWS * CELL_H   # 512

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# Unity project texture directory (auto-copy target)
COPY_DEST_DIR = os.path.join(
    os.path.expanduser("~"),
    "Documents/unity/3d/unity/Rexipso Dark Test 1/"
    "Assets/Foxipso/Assets/Yip-Boi/Williams Tube/Textures",
)


# ---------------------------------------------------------------------------
# Character set definition
# ---------------------------------------------------------------------------

# Indices 0-31: Accented Latin lowercase
# U+00E0-U+00F6, U+00F8-U+00FE (skip ÷ U+00F7), then ß(U+00DF), ÿ(U+00FF)
LATIN_LOWER = "àáâãäåæçèéêëìíîïðñòóôõöøùúûüýþßÿ"

# Indices 32-61: Accented Latin uppercase
# U+00C0-U+00D6, U+00D8-U+00DE (skip × U+00D7)
LATIN_UPPER = "ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞ"

# Indices 62-65: Extra Latin
LATIN_EXTRA = "œŒ¿¡"

# Indices 66-148: Hiragana (U+3041-U+3093, 83 codepoints)
HIRAGANA = "".join(chr(cp) for cp in range(0x3041, 0x3094))

# Indices 149-231: Katakana (U+30A1-U+30F3, 83 codepoints)
KATAKANA = "".join(chr(cp) for cp in range(0x30A1, 0x30F4))

# Indices 232-238: Japanese punctuation
JP_PUNCT = "。、「」・ー～"

# Build full character list (index = position in this list)
CHARSET = list(LATIN_LOWER + LATIN_UPPER + LATIN_EXTRA
               + HIRAGANA + KATAKANA + JP_PUNCT)


# ---------------------------------------------------------------------------
# Font discovery
# ---------------------------------------------------------------------------

# Candidate fonts in order of preference.
# unifont is pixel-perfect at 8x16; others are TrueType fallbacks.
FONT_CANDIDATES = [
    # BDF/PCF bitmap fonts (ideal for pixel-perfect output)
    "/usr/share/fonts/unifont/unifont.otf",
    "/usr/share/fonts/unifont/unifont.ttf",
    "/usr/share/fonts/truetype/unifont/unifont.ttf",
    "/usr/share/fonts/gnu-unifont/unifont.otf",
    # Noto (good Unicode coverage including CJK)
    "/usr/share/fonts/google-noto/NotoSansMono-Regular.ttf",
    "/usr/share/fonts/noto/NotoSansMono-Regular.ttf",
    "/usr/share/fonts/truetype/noto/NotoSansMono-Regular.ttf",
    "/usr/share/fonts/google-noto-cjk/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
    # DejaVu (common on Linux, no CJK)
    "/usr/share/fonts/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    # Liberation
    "/usr/share/fonts/liberation-mono/LiberationMono-Regular.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
]

# Separate CJK font candidates (for kana if primary font lacks them)
CJK_FONT_CANDIDATES = [
    "/usr/share/fonts/google-noto-cjk/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/google-noto-cjk/NotoSansJP-Regular.otf",
    "/usr/share/fonts/noto-cjk/NotoSansJP-Regular.otf",
    "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
]


def find_font(candidates):
    """Find the first available font from a list of candidates."""
    for path in candidates:
        if os.path.exists(path):
            return path
    return None


def load_font(path, size):
    """Load a font at the given pixel size."""
    try:
        return ImageFont.truetype(path, size)
    except Exception as e:
        print(f"  Warning: couldn't load {path}: {e}")
        return None


# ---------------------------------------------------------------------------
# Rendering
# ---------------------------------------------------------------------------

def render_char_to_bitmap(char, font, cell_w=CELL_W, cell_h=CELL_H):
    """Render a single Unicode character to a 1-bit bitmap (list of ints).

    Returns a list of cell_h integers, each representing one row of cell_w
    pixels (MSB = leftmost).  Returns None if the character can't be rendered.
    """
    # Render to a grayscale image larger than the cell, then crop/center
    img = Image.new("L", (cell_w * 3, cell_h * 3), 0)
    draw = ImageDraw.Draw(img)
    draw.text((cell_w, cell_h), char, fill=255, font=font)

    # Find bounding box of the rendered glyph
    bbox = img.getbbox()
    if bbox is None:
        return None  # blank glyph

    # Crop to content
    content = img.crop(bbox)
    cw, ch = content.size

    # Scale down if too wide (kana from CJK fonts are often 16px wide)
    if cw > cell_w:
        content = content.resize((cell_w, min(ch, cell_h)), Image.LANCZOS)
        cw, ch = content.size

    # Truncate height if needed
    if ch > cell_h:
        content = content.crop((0, 0, cw, cell_h))
        ch = cell_h

    # Center in cell
    final = Image.new("L", (cell_w, cell_h), 0)
    ox = (cell_w - cw) // 2
    oy = (cell_h - ch) // 2
    final.paste(content, (ox, oy))

    # Threshold to 1-bit and convert to row integers
    msb = 1 << (cell_w - 1)
    rows = []
    for y in range(cell_h):
        row_val = 0
        for x in range(cell_w):
            if final.getpixel((x, y)) > 96:
                row_val |= (msb >> x)
        rows.append(row_val)
    return rows


def render_glyph(atlas, glyph_rows, col, row, cell_w=CELL_W, cell_h=CELL_H):
    """Stamp a bitmap glyph into the atlas at grid position (col, row)."""
    msb = 1 << (cell_w - 1)
    x0 = col * cell_w
    y0 = row * cell_h
    for y, row_val in enumerate(glyph_rows):
        for x in range(cell_w):
            if row_val & (msb >> x):
                atlas.putpixel((x0 + x, y0 + y), (255, 255, 255, 255))


def main():
    parser = argparse.ArgumentParser(
        description="Generate extended PDA ROM atlas (Bank 1) for Williams Tube")
    parser.add_argument("--output", "-o", default="WilliamsTube_PDARom_Ext.png",
                        help="Output PNG filename")
    parser.add_argument("--font", "-f", default=None,
                        help="Path to a TrueType/OpenType font to use")
    parser.add_argument("--font-size", type=int, default=28,
                        help="Font size in pixels (default: 28)")
    parser.add_argument("--cjk-font", default=None,
                        help="Path to a CJK font for kana (if primary lacks them)")
    parser.add_argument("--cjk-font-size", type=int, default=28,
                        help="CJK font size in pixels (default: 28)")
    parser.add_argument("--no-copy", action="store_true",
                        help="Don't auto-copy to Unity project textures directory")
    args = parser.parse_args()

    output_path = os.path.join(SCRIPT_DIR, args.output)

    # --- Find fonts ---
    if args.font:
        font_path = args.font
    else:
        font_path = find_font(FONT_CANDIDATES)
    if not font_path:
        print("ERROR: No suitable font found. Install unifont or noto-fonts,")
        print("       or specify --font /path/to/font.ttf")
        sys.exit(1)

    print(f"Primary font: {font_path}")
    font = load_font(font_path, args.font_size)
    if not font:
        print("ERROR: Failed to load primary font")
        sys.exit(1)

    # CJK font for kana (may be same as primary)
    cjk_font = None
    if args.cjk_font:
        cjk_font_path = args.cjk_font
    else:
        cjk_font_path = find_font(CJK_FONT_CANDIDATES)
    if cjk_font_path:
        print(f"CJK font: {cjk_font_path}")
        cjk_font = load_font(cjk_font_path, args.cjk_font_size)

    # --- Render atlas ---
    atlas = Image.new("RGBA", (ATLAS_W, ATLAS_H), (0, 0, 0, 0))

    rendered = 0
    failed = []

    for idx, char in enumerate(CHARSET):
        if idx >= 256:
            break

        # Choose font: use CJK font for kana if available
        cp = ord(char)
        use_font = font
        if cjk_font and cp >= 0x3000:
            use_font = cjk_font

        bitmap = render_char_to_bitmap(char, use_font)
        if bitmap is None:
            # Try CJK fallback even for non-CJK ranges
            if cjk_font and use_font != cjk_font:
                bitmap = render_char_to_bitmap(char, cjk_font)
            if bitmap is None:
                # Try primary font if we used CJK
                if use_font != font:
                    bitmap = render_char_to_bitmap(char, font)

        if bitmap:
            col = idx % GRID_COLS
            row = idx // GRID_COLS
            render_glyph(atlas, bitmap, col, row)
            rendered += 1
        else:
            failed.append((idx, char, f"U+{cp:04X}"))

    atlas.save(output_path)

    print(f"\nSaved: {output_path}")
    print(f"  Size: {atlas.size[0]}x{atlas.size[1]} ({GRID_COLS}x{GRID_ROWS} grid, "
          f"{CELL_W}x{CELL_H} per cell)")
    print(f"  Rendered: {rendered}/{len(CHARSET)} characters")
    print(f"  Layout:")
    print(f"    0-31:    Accented Latin lowercase ({len(LATIN_LOWER)} chars)")
    print(f"    32-61:   Accented Latin uppercase ({len(LATIN_UPPER)} chars)")
    print(f"    62-65:   Extra Latin ({len(LATIN_EXTRA)} chars)")
    print(f"    66-148:  Hiragana ({len(HIRAGANA)} chars)")
    print(f"    149-231: Katakana ({len(KATAKANA)} chars)")
    print(f"    232-238: Japanese punctuation ({len(JP_PUNCT)} chars)")
    print(f"    239-255: Reserved")

    if failed:
        print(f"\n  Failed to render {len(failed)} characters:")
        for idx, char, name in failed[:20]:
            print(f"    [{idx}] {char} ({name})")
        if len(failed) > 20:
            print(f"    ... and {len(failed) - 20} more")

    # Auto-copy to Unity project
    if not args.no_copy and os.path.isdir(COPY_DEST_DIR):
        dest = os.path.join(COPY_DEST_DIR, args.output)
        shutil.copy2(output_path, dest)
        print(f"\nCopied to Unity: {dest}")
    elif not args.no_copy:
        print(f"\nUnity textures dir not found, skipping auto-copy: {COPY_DEST_DIR}")


if __name__ == "__main__":
    main()
