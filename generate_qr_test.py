#!/usr/bin/env python3
"""Generate QR code test assets for Williams Tube DM pairing proof-of-concept.

Produces:
  1. A 256x256 QR code PNG (for quick IMG program validation)
  2. A 512x512 QR V1 template macro (finder/timing patterns only, for macro atlas)
  3. A C++ header with the data module positions for a test payload

Usage:
    python3 generate_qr_test.py [--payload 483291]
"""

import argparse
import os
import qrcode
import qrcode.constants
from qrcode.main import QRCode
from PIL import Image

# Import glyph data from the ROM generator (same as generate_macro_atlas.py)
from generate_pda_rom import build_cp437_font, build_custom_icons, UI_MAP, CELL_W, CELL_H

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# QR V1 is 21x21 modules. We center it in a 32x32 bitmap grid.
QR_SIZE = 21
GRID_SIZE = 32
QR_OFFSET = (GRID_SIZE - QR_SIZE) // 2  # 5 cells of padding on each side

# Bitmap cell size in the 512x512 macro image
CELL_PX = 512 // GRID_SIZE  # 16 pixels per cell

# VQ codebook indices
VQ_BLACK = 0    # all-black 8x8 block
VQ_WHITE = 255  # all-white 8x8 block


def generate_qr_matrix(payload: str):
    """Generate QR V1 module matrix for a numeric payload.

    Returns a 21x21 list of lists (True = dark module).
    """
    qr = QRCode(
        version=1,
        error_correction=qrcode.constants.ERROR_CORRECT_H,
        box_size=1,
        border=0,
    )
    qr.add_data(payload)
    qr.make(fit=False)
    # qr.modules is a list of lists of bools (True = dark)
    return qr.modules


def get_fixed_pattern_mask(size=21):
    """Return a 21x21 mask of which modules are fixed (finder, timing, format, dark).

    True = fixed pattern module (part of the template).
    """
    mask = [[False] * size for _ in range(size)]

    # Finder patterns (7x7) + separators (1 module around each finder)
    # Top-left finder: (0,0) to (6,6), separator row 7 and col 7
    for r in range(8):
        for c in range(8):
            if r < size and c < size:
                mask[r][c] = True

    # Top-right finder: (0, size-7) to (6, size-1), separator col size-8
    for r in range(8):
        for c in range(size - 8, size):
            if r < size and c >= 0:
                mask[r][c] = True

    # Bottom-left finder: (size-7, 0) to (size-1, 6), separator row size-8
    for r in range(size - 8, size):
        for c in range(8):
            if r >= 0 and c < size:
                mask[r][c] = True

    # Timing patterns: row 6 (between finders) and col 6
    for c in range(8, size - 8):
        mask[6][c] = True
    for r in range(8, size - 8):
        mask[r][6] = True

    # Dark module (always dark, at position (size-8, 8) = (13, 8) for V1)
    mask[size - 8][8] = True

    # Format information: around top-left finder + split across top-right and bottom-left
    # Row 8, cols 0-8 (with skip at col 6 for timing)
    for c in range(9):
        mask[8][c] = True
    # Col 8, rows 0-8 (with skip at row 6 for timing)
    for r in range(9):
        mask[r][8] = True
    # Bottom-left format: col 8, rows size-7 to size-1
    for r in range(size - 7, size):
        mask[r][8] = True
    # Top-right format: row 8, cols size-8 to size-1
    for c in range(size - 8, size):
        mask[8][c] = True

    return mask


def render_qr_to_img(modules, img_size=256):
    """Render a QR module matrix as a standard black-on-white PNG."""
    size = len(modules)
    # Add quiet zone of 4 modules
    border = 4
    total = size + 2 * border
    cell = img_size // total

    img = Image.new("L", (img_size, img_size), 255)
    for r in range(size):
        for c in range(size):
            if modules[r][c]:  # dark module
                x0 = (c + border) * cell
                y0 = (r + border) * cell
                for py in range(y0, y0 + cell):
                    for px in range(x0, x0 + cell):
                        if px < img_size and py < img_size:
                            img.putpixel((px, py), 0)
    return img


def build_rom_glyphs():
    """Load ROM glyphs for rendering text into the template.

    Same logic as generate_macro_atlas.py's build_rom_glyphs().
    """
    cp437 = build_cp437_font()
    icons = build_custom_icons()
    glyphs = {}
    for pda_idx, cp437_idx in UI_MAP.items():
        if cp437[cp437_idx] is not None:
            glyphs[pda_idx] = cp437[cp437_idx]
    for code in range(32, 128):
        if cp437[code] is not None:
            glyphs[code] = cp437[code]
    for pda_idx, bitmap in icons.items():
        glyphs[pda_idx] = bitmap
    return glyphs


def bake_text(img, col, row, text, rom_glyphs):
    """Render text string into a 512x512 image at the given text grid position.

    Uses the same coordinate system as the CRT: 40 cols x 8 rows.
    """
    TEXT_COLS = 40
    TEXT_ROWS = 8
    cell_w = 512 / TEXT_COLS   # 12.8
    cell_h = 512 / TEXT_ROWS   # 64.0

    for i, ch in enumerate(text):
        glyph_idx = ord(ch)
        glyph_data = rom_glyphs.get(glyph_idx)
        if glyph_data is None:
            continue

        c = col + i
        if c >= TEXT_COLS:
            break

        x0 = int(c * cell_w)
        x1 = int((c + 1) * cell_w)
        y0 = int(row * cell_h)
        y1 = int((row + 1) * cell_h)

        glyph_h = len(glyph_data)
        glyph_w = 8
        for py in range(y0, y1):
            gy = min(int((py - y0) / (y1 - y0) * glyph_h), glyph_h - 1)
            byte_val = glyph_data[gy]
            for px in range(x0, x1):
                gx = min(int((px - x0) / (x1 - x0) * glyph_w), glyph_w - 1)
                if byte_val & (0x80 >> gx):
                    img.putpixel((px, py), 255)


def render_template_macro(modules, rom_glyphs=None):
    """Render a 512x512 macro image with only the fixed QR patterns.

    The image is a luminance mask matching the macro atlas format:
    255 = foreground (green on CRT), 0 = background (dark on CRT).

    For QR: light modules → 255 (foreground/green), dark modules → 0 (background).
    This produces an "inverted" QR on the CRT (green background, dark modules).
    """
    fixed_mask = get_fixed_pattern_mask(QR_SIZE)
    img = Image.new("L", (512, 512), 0)

    for r in range(QR_SIZE):
        for c in range(QR_SIZE):
            if not fixed_mask[r][c]:
                continue

            # Map QR module to pixel region in 512x512 image
            grid_r = r + QR_OFFSET
            grid_c = c + QR_OFFSET
            x0 = grid_c * CELL_PX
            y0 = grid_r * CELL_PX

            # Light module = foreground (255), dark module = background (0)
            val = 0 if modules[r][c] else 255
            for py in range(y0, y0 + CELL_PX):
                for px in range(x0, x0 + CELL_PX):
                    if px < 512 and py < 512:
                        img.putpixel((px, py), val)

    # Also fill the quiet zone around the QR with foreground (light background)
    # This ensures the QR has a visible light border for finder pattern detection
    for grid_r in range(GRID_SIZE):
        for grid_c in range(GRID_SIZE):
            qr_r = grid_r - QR_OFFSET
            qr_c = grid_c - QR_OFFSET

            # Is this cell in the quiet zone? (outside QR but inside grid)
            in_quiet = (qr_r < 0 or qr_r >= QR_SIZE or
                        qr_c < 0 or qr_c >= QR_SIZE)
            if not in_quiet:
                continue

            x0 = grid_c * CELL_PX
            y0 = grid_r * CELL_PX
            for py in range(y0, y0 + CELL_PX):
                for px in range(x0, x0 + CELL_PX):
                    if px < 512 and py < 512:
                        img.putpixel((px, py), 255)

    # Bake "Manual code:" label into the template (text row 7, bottom of screen)
    # The dynamic 6-digit code will be written at runtime at col 15
    if rom_glyphs:
        bake_text(img, 1, 7, "Manual code:", rom_glyphs)

    return img


def generate_data_modules(modules):
    """Extract the variable (data + EC) module positions and values.

    Returns a list of (grid_col, grid_row, is_dark) for non-fixed modules.
    """
    fixed_mask = get_fixed_pattern_mask(QR_SIZE)
    data_modules = []

    for r in range(QR_SIZE):
        for c in range(QR_SIZE):
            if fixed_mask[r][c]:
                continue
            grid_r = r + QR_OFFSET
            grid_c = c + QR_OFFSET
            data_modules.append((grid_c, grid_r, modules[r][c]))

    return data_modules


def write_cpp_header(data_modules, payload, output_path):
    """Write a C++ header with the data module positions for the test payload."""
    with open(output_path, "w") as f:
        f.write(f"// Auto-generated QR V1 data modules for payload \"{payload}\"\n")
        f.write("// Generated by generate_qr_test.py — do not edit\n")
        f.write("#pragma once\n\n")
        f.write("#include <array>\n\n")
        f.write("namespace YipOS {\n")
        f.write("namespace QRTest {\n\n")
        f.write(f"static constexpr int QR_TEMPLATE_MACRO = 37;\n")
        f.write(f"static constexpr int QR_OFFSET = {QR_OFFSET};\n")
        f.write(f"static constexpr int QR_SIZE = {QR_SIZE};\n")
        f.write(f"static constexpr int VQ_BLACK = {VQ_BLACK};\n")
        f.write(f"static constexpr int VQ_WHITE = {VQ_WHITE};\n\n")

        # Only emit the light (non-dark) data modules — those are the ones
        # we need to write in bitmap mode (dark modules = background = already stamped)
        light_modules = [(c, r) for c, r, is_dark in data_modules if not is_dark]
        dark_modules = [(c, r) for c, r, is_dark in data_modules if is_dark]

        f.write(f"// Light data modules to write (VQ_WHITE) after template stamp\n")
        f.write(f"// Template leaves data area as background (dark), so we write light modules\n")
        f.write(f"static constexpr int LIGHT_MODULE_COUNT = {len(light_modules)};\n")
        f.write(f"struct Module {{ int col; int row; }};\n")
        f.write(f"static constexpr std::array<Module, {len(light_modules)}> LIGHT_MODULES = {{{{\n")
        for i, (c, r) in enumerate(light_modules):
            comma = "," if i < len(light_modules) - 1 else ""
            f.write(f"    {{{c}, {r}}}{comma}\n")
        f.write(f"}}}};\n\n")

        f.write(f"// Dark data modules (for reference — these are already background)\n")
        f.write(f"static constexpr int DARK_MODULE_COUNT = {len(dark_modules)};\n")

        total = len(light_modules) + len(dark_modules)
        f.write(f"\n// Total data modules: {total} ({len(light_modules)} light + {len(dark_modules)} dark)\n")
        f.write(f"// Estimated render time: 1 macro stamp + {len(light_modules)} writes × 70ms ≈ {len(light_modules) * 0.07:.1f}s\n")

        f.write(f"\n}} // namespace QRTest\n")
        f.write(f"}} // namespace YipOS\n")


def main():
    parser = argparse.ArgumentParser(description="Generate QR test assets")
    parser.add_argument("--payload", default="483291",
                        help="Numeric payload to encode (default: 483291)")
    parser.add_argument("--output-dir", default=SCRIPT_DIR,
                        help="Output directory for generated files")
    args = parser.parse_args()

    out = args.output_dir
    payload = args.payload

    print(f"Generating QR V1 for payload: {payload}")

    print("Loading ROM glyphs...")
    rom_glyphs = build_rom_glyphs()
    print(f"  Loaded {len(rom_glyphs)} glyphs")

    modules = generate_qr_matrix(payload)
    print(f"  QR matrix: {len(modules)}x{len(modules[0])} modules")

    # 1. Test QR PNG for IMG program quick test
    qr_img = render_qr_to_img(modules, img_size=256)
    qr_png_path = os.path.join(out, "qr_test.png")
    qr_img.save(qr_png_path)
    print(f"  Saved test QR PNG: {qr_png_path}")

    # 2. QR template macro (512x512, fixed patterns only, with "Manual code:" label)
    template_img = render_template_macro(modules, rom_glyphs)
    template_path = os.path.join(out, "qr_template.png")
    template_img.save(template_path)
    print(f"  Saved QR template macro: {template_path}")

    # 3. Data module positions for C++ header
    data_modules = generate_data_modules(modules)
    light_count = sum(1 for _, _, d in data_modules if not d)
    dark_count = sum(1 for _, _, d in data_modules if d)
    print(f"  Data modules: {len(data_modules)} total ({light_count} light, {dark_count} dark)")
    print(f"  Estimated render: 1 stamp + {light_count} writes × 70ms ≈ {light_count * 0.07:.1f}s")

    header_path = os.path.join(out, "yip_os", "src", "screens", "QRTestData.hpp")
    write_cpp_header(data_modules, payload, header_path)
    print(f"  Saved C++ header: {header_path}")

    # Also make a copy for assets/images so IMG program can display it
    assets_img_dir = os.path.join(out, "yip_os", "assets", "images")
    if os.path.isdir(assets_img_dir):
        import shutil
        dst = os.path.join(assets_img_dir, "qr_test.png")
        shutil.copy2(qr_png_path, dst)
        print(f"  Copied to assets: {dst}")
    else:
        print(f"  (assets/images not found at {assets_img_dir}, skipping copy)")


if __name__ == "__main__":
    main()
