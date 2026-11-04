#!/usr/bin/env python3
"""Run the production gesture logic against a small fake engine on Linux.

Only platform headers and MSVC's SEH syntax are adapted. These tests do not
validate SEH unwinding, retail function addresses, or network packet encoding.
"""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parent
source = (root.parent / 'DrawnMove.cpp').read_text()
source = '\n'.join(line for line in source.splitlines() if not line.startswith('#include'))
source = source.replace('__try', '').replace('__finally', '')
with tempfile.TemporaryDirectory() as temp:
    path = Path(temp)
    (path / 'test.cpp').write_text(
        '#include "drawn_move_test_engine.h"\n' + source +
        '\n#include "drawn_move_input_cases.h"\n')
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-g', '-I', str(root),
                    '-I', str(root.parent), str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
