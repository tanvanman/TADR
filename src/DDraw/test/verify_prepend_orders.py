#!/usr/bin/env python3
"""Verify the release DLL's prepend feature choice and experimental exclusions."""
import argparse
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('dll', type=Path)
p.add_argument('--enabled', type=int, choices=(0, 1), required=True)
a = p.parse_args()
data = a.dll.read_bytes()
assert (b'[QueuePrepend] build=ctrl-shift-v4' in data) == bool(a.enabled)
for unwanted in (b'[MultiBuild]', b'[MainMapRadar]', b'DrawnMoveTestNoOrders'):
    assert unwanted not in data, unwanted
print('Verified Ctrl+Shift prepend feature:', a.enabled)
