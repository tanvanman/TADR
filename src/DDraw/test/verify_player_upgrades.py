#!/usr/bin/env python3
"""Verify every feature in the produced DLL, including compiled-out profiles."""
import argparse
from pathlib import Path

MARKERS = (
    b'[DrawnMove] build=alt-v6',
    b'[MexUpgrade] build=optional-cursors-v10',
    b'[BuildRectangle] row-alt-v6',
    b'[QueuePrepend] build=ctrl-shift-v4',
    b'[AlliedCursors] build=light-v1',
    b'[UnitStatsPanel] build=bottom-bar-v2',
    b'[LabBuildCycles] build=native-cycle-v3',
)
parser = argparse.ArgumentParser()
parser.add_argument('dll', type=Path)
parser.add_argument('--mask', type=int, required=True)
args = parser.parse_args()
assert 0 <= args.mask < 128
data = args.dll.read_bytes()
assert data[:2] == b'MZ', 'Expected a PE DLL'
for index, marker in enumerate(MARKERS):
    assert (marker in data) == bool(args.mask & (1 << index)), 'Wrong upgrade selection: ' + marker.decode()
for unwanted in (b'[MoveOrderFeedback]', b'[MainMapRadar]', b'[MultiBuild]', b'DrawnMoveTestNoOrders'):
    assert unwanted not in data, 'Cancelled or debug-only feature included: ' + unwanted.decode()
print('Verified all seven upgrade switches, mask:', args.mask)
