#!/usr/bin/env python3
"""Generate the 4-frame buildrotate / buildrotateclick reference GAFs.

The DLL looks for two optional files under TA's `anims/` directory:

    anims/buildrotate.gaf       — idle cardinal arrows (4 frames)
    anims/buildrotateclick.gaf  — companion "click" art used during the
                                   ~200 ms feedback flash after a
                                   quadrant-click on a build-menu button.
                                   Optional. Falls back to tinting the
                                   idle GAF white when absent.

When neither file loads the DLL falls back to the built-in chevrons.

This script writes BOTH files (`buildrotate.gaf` + `buildrotateclick.gaf`)
to the output directory by default. Frames are ordered to match the
rotation index used by CUnitRotate:

    frame 0 = S    (apex points down,  hotspot anchored at bottom mid-edge)
    frame 1 = E    (apex points right, hotspot anchored at right  mid-edge)
    frame 2 = N    (apex points up,    hotspot anchored at top    mid-edge)
    frame 3 = W    (apex points left,  hotspot anchored at left   mid-edge)

The hotspot in each frame should be the visual centre of the cardinal
edge so the frame slots cleanly into the chevron-occupied band on a
build button. Pixels matching `Background` are treated as transparent.

GAF format (from Total Annihilation, all little-endian):

    Header (12 bytes):
        u32 IDVersion        = 0x00010100
        u32 NumberOfEntries
        u32 Reserved         = 0

    Entry pointer table (4 * NumberOfEntries bytes):
        u32 EntryOffset[]

    For each entry:
        u16 Frames
        u16 _padding
        u32 _reserved
        char Name[32]
        For each frame:
            u32 FramePtrOffset
            u32 Animated

    Frame struct (24 bytes):
        u16 Width
        u16 Height
        i16 XPos        (hotspot)
        i16 YPos        (hotspot)
        u8  Background
        u8  Compressed  (0 = raw)
        u16 SubFrames   = 0
        u32 _reserved   = 0
        u32 FrameDataOffset
        u32 FPS / duration

We emit uncompressed frames since size doesn't matter for a reference
file and it keeps the writer simple.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path
from typing import List, Tuple

# Frame canvas. 7x7 — half the previous 14x14 reference size; the
# triangle still reads clearly inside the cardinal margin band.
FRAME_W = 7
FRAME_H = 7

# Palette indices used by the test artwork.
# Per palettes/PALETTE.PAL inspection on a stock TA install:
#   0 = black      9 = light blue   234 = build-rect green
#   240..247 = team-colour slots (volatile: overwritten with player logo
#              colour at game start, so unsafe for static art)
#   249 = pure red    250 = pure green   251 = pure yellow (chat-text hue)
#   252 = pure blue   253 = magenta      254 = pure cyan
#   255 = pure white
TRANSPARENT     = 9    # GAF Background; the DLL skips these pixels on blit.
IDLE_TRI_COLOR  = 251  # pure yellow (255,255,0), same hue as in-game chat text
CLICK_TRI_COLOR = 255  # pure white — pressed-state flash

# Hotspot — landed at the *outer* tip of the chevron stack so the frame
# sits flush against the button edge with the apex pointing outward.
HOTSPOT_X = FRAME_W // 2
HOTSPOT_Y = FRAME_H // 2


def make_blank() -> List[List[int]]:
    return [[TRANSPARENT for _ in range(FRAME_W)] for _ in range(FRAME_H)]


def plot(canvas: List[List[int]], x: int, y: int, color: int) -> None:
    if 0 <= x < FRAME_W and 0 <= y < FRAME_H:
        canvas[y][x] = color


def line(canvas: List[List[int]], x0: int, y0: int, x1: int, y1: int,
         color: int, thickness: int = 1) -> None:
    """Bresenham; each plotted pixel is a (2*half+1) square."""
    half = thickness // 2
    dx = abs(x1 - x0)
    dy = -abs(y1 - y0)
    sx = 1 if x0 < x1 else -1
    sy = 1 if y0 < y1 else -1
    err = dx + dy
    while True:
        for oy in range(-half, half + 1):
            for ox in range(-half, half + 1):
                plot(canvas, x0 + ox, y0 + oy, color)
        if x0 == x1 and y0 == y1:
            break
        e2 = 2 * err
        if e2 >= dy:
            err += dy; x0 += sx
        if e2 <= dx:
            err += dx; y0 += sy


def draw_solid_triangle(canvas: List[List[int]], cardinal: int,
                        fill_color: int) -> None:
    """Single solid isoceles triangle, apex at the cardinal edge midpoint.

    Scanline fill from the apex inward: at each step away from the
    apex along -out, the perpendicular half-width grows linearly until
    it reaches the base.
    """
    out = [(0, 1), (1, 0), (0, -1), (-1, 0)][cardinal & 3]
    perp = (out[1], -out[0])

    if cardinal == 0:
        apex = (FRAME_W // 2, FRAME_H - 1)
    elif cardinal == 1:
        apex = (FRAME_W - 1, FRAME_H // 2)
    elif cardinal == 2:
        apex = (FRAME_W // 2, 0)
    else:
        apex = (0, FRAME_H // 2)

    depth     = FRAME_H - 2  # leave a 2px margin on the far side
    base_half = (FRAME_W // 2) - 1

    for r in range(depth + 1):
        half_w = (base_half * r + depth // 2) // depth
        cx = apex[0] - r * out[0]
        cy = apex[1] - r * out[1]
        for w in range(-half_w, half_w + 1):
            plot(canvas, cx + w * perp[0], cy + w * perp[1], fill_color)


def serialize_canvas(canvas: List[List[int]]) -> bytes:
    return bytes(b for row in canvas for b in row)


def build_gaf(frames: List[Tuple[List[List[int]], int, int]],
              entry_name: str = "BUILDROTATE") -> bytes:
    """Pack a list of (canvas, hotspot_x, hotspot_y) frames into a GAF.

    Layout written (all uncompressed):
        Header (12) | EntryOffsetTable (4) | EntryHeader (40) |
        FrameTable (8 * N) | FrameStructs (24 * N) | FrameData ...
    """
    n = len(frames)

    # Compute layout offsets.
    header_size       = 12
    entry_table_size  = 4
    entry_header_size = 40
    frame_table_size  = 8 * n
    frame_struct_size = 24
    frame_data_size   = sum(FRAME_W * FRAME_H for _ in frames)

    entry_off       = header_size + entry_table_size
    frame_table_off = entry_off + entry_header_size
    frame_struct_off_first = frame_table_off + frame_table_size
    frame_data_off_first   = frame_struct_off_first + frame_struct_size * n

    out = bytearray()
    # GAF Header.
    out += struct.pack("<III", 0x00010100, 1, 0)
    # Entry offset table (one entry).
    out += struct.pack("<I", entry_off)
    # Entry header.
    name_bytes = entry_name.encode("ascii")[:31].ljust(32, b"\x00")
    out += struct.pack("<HHI32s", n, 0, 0, name_bytes)
    # Frame table (FramePtrOffset + Animated per frame).
    for i in range(n):
        out += struct.pack("<II", frame_struct_off_first + i * frame_struct_size, 10)
    # Frame structs.
    cur_data_off = frame_data_off_first
    for canvas, hx, hy in frames:
        out += struct.pack("<HHhhBBHIII",
                           FRAME_W, FRAME_H,
                           hx, hy,
                           TRANSPARENT,
                           0,         # uncompressed
                           0,         # SubFrames
                           0,         # reserved
                           cur_data_off,
                           0)         # FPS / duration
        cur_data_off += FRAME_W * FRAME_H
    # Frame data.
    for canvas, _hx, _hy in frames:
        out += serialize_canvas(canvas)
    return bytes(out)


def make_frames(fill: int) -> List[Tuple[List[List[int]], int, int]]:
    out: List[Tuple[List[List[int]], int, int]] = []
    for cardinal in range(4):
        canvas = make_blank()
        draw_solid_triangle(canvas, cardinal, fill)
        out.append((canvas, HOTSPOT_X, HOTSPOT_Y))
    return out


def main(argv: List[str]) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-d", "--out-dir", type=Path, default=Path("."),
                    help="output directory (default: current dir)")
    args = ap.parse_args(argv)
    args.out_dir.mkdir(parents=True, exist_ok=True)

    idle_path  = args.out_dir / "buildrotate.gaf"
    click_path = args.out_dir / "buildrotateclick.gaf"

    idle_blob = build_gaf(make_frames(IDLE_TRI_COLOR),
                          entry_name="BUILDROTATE")
    idle_path.write_bytes(idle_blob)
    print(f"Wrote {len(idle_blob)} bytes to {idle_path}")

    click_blob = build_gaf(make_frames(CLICK_TRI_COLOR),
                           entry_name="BUILDROTATECLICK")
    click_path.write_bytes(click_blob)
    print(f"Wrote {len(click_blob)} bytes to {click_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
