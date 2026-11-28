#!/usr/bin/env python3
"""Verify the unit statistics compile-time choice in a release DLL."""
import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('dll', type=Path)
parser.add_argument('--enabled', required=True, type=int, choices=(0, 1))
args = parser.parse_args()
data = args.dll.read_bytes()
marker = b'[UnitStatsPanel] build=bottom-bar-v2'
assert (marker in data) == bool(args.enabled), 'Incorrect unit statistics feature selection'
print('Unit statistics feature selection verified (enabled=%d)' % args.enabled)
