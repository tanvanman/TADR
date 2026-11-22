#!/usr/bin/env python3
"""Verify that a release DLL includes exactly the selected move feedback implementation."""
import argparse
from pathlib import Path
import re

parser = argparse.ArgumentParser()
parser.add_argument('dll', type=Path)
parser.add_argument('--enabled', type=int, choices=(0, 1), required=True)
args = parser.parse_args()
markers = re.findall(rb'\[MoveOrderFeedback\] build=[^\x00\r\n]*', args.dll.read_bytes())
expected = [b'[MoveOrderFeedback] build=destinations-v3'] if args.enabled else []
if markers != expected:
    raise SystemExit('Wrong move order feedback build markers: ' + repr(markers))
print('Verified move order destination feedback:', args.enabled)
