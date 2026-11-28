#!/usr/bin/env python3
"""Verify that the production artifact contains the enabled megamap route."""
from pathlib import Path
import sys

data = Path(sys.argv[1]).read_bytes()
marker = b'[DrawnMove] compiled-megamap=enabled'
if marker not in data:
    raise SystemExit('FAIL: megamap integration is not enabled in this DLL')
print('Verified: megamap integration is enabled in the production DLL')
