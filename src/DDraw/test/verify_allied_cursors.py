#!/usr/bin/env python3
"""Verify the standalone cursor test DLL feature selection."""
import argparse
from pathlib import Path
p = argparse.ArgumentParser()
p.add_argument('dll', type=Path)
p.add_argument('--enabled', type=int, choices=(0,1), required=True)
a = p.parse_args()
data = a.dll.read_bytes()
assert (b'[AlliedCursors] build=light-v1' in data) == bool(a.enabled), 'Wrong cursor feature selection'
assert b'[MainMapRadar]' not in data, 'Cancelled radar feature must stay excluded'
print('Verified teammate cursor feature:', a.enabled)
