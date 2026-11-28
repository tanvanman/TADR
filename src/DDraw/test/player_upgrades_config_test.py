#!/usr/bin/env python3
"""Check explicit mod defaults and every independent player-upgrade selection.

The optional CI preparation mode edits the Escalation header in the disposable
runner checkout. Production feature selection remains in config_*.h.
"""
import argparse
import itertools
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
FLAGS = ('DRAWN_MOVE_FORMATIONS_ENABLE', 'MEX_UPGRADE_ENABLE', 'GRID_BUILDING_ENABLE',
         'PREPEND_ORDERS_ENABLE', 'ALLIED_CURSORS_ENABLE', 'UNIT_STATS_PANEL_ENABLE',
         'LAB_BUILD_CYCLES_ENABLE')
PROFILES = ('ESCALATION', 'PROTA', 'OTA', 'TAZERO', 'BTA', 'MAYHEM', 'TWILIGHT')


def select(data, values):
    for flag, value in zip(FLAGS, values):
        pattern = rb'(?m)^#define ' + flag.encode() + rb' [^\r\n]*'
        assert len(re.findall(pattern, data)) == 1, flag + ' must be explicitly selected once'
        data = re.sub(pattern, ('#define ' + flag + ' ' + str(value)).encode(), data)
    return data


def preprocess(root, profile):
    source = '#include "config.h"\nUPGRADE_VALUES ' + ' '.join(FLAGS) + '\n'
    command = shlex.split(os.environ.get('CXX', 'g++')) + [
        '-std=c++17', '-E', '-P', '-x', 'c++', '-I', str(root),
        '-DTDRAW_CONFIG_' + profile, '-']
    return subprocess.run(command, input=source, text=True, capture_output=True)


def values(result):
    assert result.returncode == 0, result.stderr
    line = next(line for line in result.stdout.splitlines() if line.startswith('UPGRADE_VALUES '))
    return tuple(map(int, line.split()[1:]))


def test():
    with tempfile.TemporaryDirectory() as folder:
        root = Path(folder)
        for path in (ROOT / 'config.h', ROOT / 'OptionalUnitControls.h', *ROOT.glob('config_*.h')):
            shutil.copyfile(path, root / path.name)
        for profile in PROFILES:
            data = (root / ('config_' + profile.lower() + '.h')).read_bytes()
            select(data, (0,) * len(FLAGS))  # Also proves there is exactly one explicit definition.
            assert values(preprocess(root, profile)) == ((1,) * 7 if profile == 'ESCALATION' else (0,) * 7)
        path = root / 'config_escalation.h'
        original = path.read_bytes()
        for combination in itertools.product((0, 1), repeat=len(FLAGS)):
            path.write_bytes(select(original, combination))
            assert values(preprocess(root, 'ESCALATION')) == combination
        for flag in FLAGS:
            for invalid in (-1, 2):
                combination = [0] * 7
                combination[FLAGS.index(flag)] = invalid
                path.write_bytes(select(original, combination))
                result = preprocess(root, 'ESCALATION')
                assert result.returncode != 0 and flag in result.stderr
            path.write_bytes(re.sub(rb'(?m)^#define ' + flag.encode() + rb' [^\r\n]*\r?\n', b'', original))
            result = preprocess(root, 'ESCALATION')
            assert result.returncode != 0 and flag in result.stderr
        no_hook = re.sub(rb'(?m)^#define TA_HOOK_ENABLE 1', b'#define TA_HOOK_ENABLE 0', original)
        path.write_bytes(select(no_hook, (0,) * 7))
        assert values(preprocess(root, 'ESCALATION')) == (0,) * 7
        for index in range(3):
            combination = [0] * 7
            combination[index] = 1
            path.write_bytes(select(no_hook, combination))
            assert preprocess(root, 'ESCALATION').returncode != 0
    print('Seven explicit mod profiles and all 128 independent upgrade combinations passed')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--prepare-escalation-mask', type=int)
    args = parser.parse_args()
    if args.prepare_escalation_mask is not None:
        mask = args.prepare_escalation_mask
        assert 0 <= mask < 128
        path = ROOT / 'config_escalation.h'
        path.write_bytes(select(path.read_bytes(), [(mask >> i) & 1 for i in range(7)]))
        print('Prepared disposable Escalation CI header, feature mask:', mask)
    else:
        test()


if __name__ == '__main__':
    main()
