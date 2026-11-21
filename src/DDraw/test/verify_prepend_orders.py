#!/usr/bin/env python3
"""Verify that a release DLL includes exactly the selected prepend implementation."""
import argparse
from pathlib import Path
import re

p = argparse.ArgumentParser()
p.add_argument('dll', type=Path)
p.add_argument('--enabled', type=int, choices=(0, 1), required=True)
a = p.parse_args()
data = a.dll.read_bytes()
markers = re.findall(rb'\[QueuePrepend\] build=[^\x00\r\n]*', data)
expected = [b'[QueuePrepend] build=ctrl-shift-v4'] if a.enabled else []
assert markers == expected, (markers, expected)
print('Verified Ctrl+Shift prepend feature:', a.enabled)
