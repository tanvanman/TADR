#!/usr/bin/env python3
"""Check the produced DLL includes exactly the requested optional features."""
import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('dll', type=Path)
parser.add_argument('--formations', type=int, choices=(0,1), required=True)
parser.add_argument('--mex', type=int, choices=(0,1), required=True)
parser.add_argument('--grid', type=int, choices=(0,1), required=True)
args = parser.parse_args()
data = args.dll.read_bytes()
checks = ((b'[DrawnMove] build=alt-v6', args.formations),
          (b'[MexUpgrade] build=optional-cursors-v10', args.mex),
          (b'[BuildRectangle] click-v5', args.grid))
for marker, enabled in checks:
    if (marker in data) != bool(enabled):
        raise SystemExit('Wrong feature selection for ' + marker.decode())
if b'[MainMapRadar]' in data:
    raise SystemExit('Cancelled radar experiment must not be included')
print('Verified independent feature selection:', args.formations, args.mex, args.grid)
