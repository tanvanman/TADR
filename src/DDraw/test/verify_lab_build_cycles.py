#!/usr/bin/env python3
"""Verify that a built DLL contains exactly the requested cycle feature."""
import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('dll', type=Path)
parser.add_argument('--enabled', required=True, type=int, choices=(0, 1))
args = parser.parse_args()
data = args.dll.read_bytes()
assert data[:2] == b'MZ', 'Expected a PE DLL'
marker = b'[LabBuildCycles] build=native-cycle-v3'
assert (marker in data) == bool(args.enabled), 'Lab build-cycle feature mismatch'
print('Lab build cycles:', 'enabled' if args.enabled else 'compiled out')
