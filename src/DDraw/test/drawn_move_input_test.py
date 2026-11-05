#!/usr/bin/env python3
"""Run the production gesture logic against a small fake engine on Linux.

Only platform headers and MSVC's SEH syntax are adapted. These tests do not
validate SEH unwinding, retail function addresses, or network packet encoding.
"""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--mod', choices=('OTA', 'PROTA', 'ESCALATION', 'BTA',
                                     'MAYHEM', 'TAZERO', 'TWILIGHT'),
                    default='ESCALATION')
args = parser.parse_args()

root = Path(__file__).resolve().parent
source = (root.parent / 'DrawnMove.cpp').read_text()
source = '\n'.join(line for line in source.splitlines() if not line.startswith('#include') or line == '#include "config.h"' )
source = source.replace('__try', '').replace('__finally', '')
with tempfile.TemporaryDirectory() as temp:
    path = Path(temp)
    from itertools import product
    for megamap, preview, no_orders in product((0, 1), repeat=3):
        variant = source if megamap else source.replace('#include "config.h"',
            '#include "config.h"\n#undef USEMEGAMAP\n#define USEMEGAMAP 0')
        (path / 'test.cpp').write_text(
            '#include "drawn_move_test_engine.h"\n' + variant +
            '\n#include "drawn_move_input_cases.h"\n')
        subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', '-g', f'-DTDRAW_CONFIG_{args.mod}',
                        f'-DDRAWN_MOVE_TEST_PREVIEW={preview}', f'-DDRAWN_MOVE_TEST_NO_ORDERS={no_orders}',
                        '-I', str(root), '-I', str(root.parent), str(path / 'test.cpp'),
                        '-o', str(path / 'test')], check=True)
        subprocess.run([str(path / 'test')], check=True,
                       env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
    # Regression: the original test harness supplied USEMEGAMAP itself and
    # masked a missing production config.h include. The module must now fail
    # preprocessing without that header, rather than silently compiling out
    # the megamap routes.
    (path / 'missing_config.cpp').write_text(
        '#include "drawn_move_test_engine.h"\n' + source.replace('#include "config.h"', ''))
    check = subprocess.run(['g++', '-E', '-I', str(root), '-I', str(root.parent),
                            str(path / 'missing_config.cpp')], capture_output=True, text=True)
    assert check.returncode != 0 and 'DrawnMove requires config.h' in check.stderr
    print(f'{args.mod}: missing-configuration compilation guard test passed')
