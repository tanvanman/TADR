#!/usr/bin/env python3
"""Validate real profile defaults, independent overrides and build-property wiring."""
import itertools
import os
from pathlib import Path
import shlex
import subprocess
import xml.etree.ElementTree as ET

DDRAW = Path(__file__).resolve().parent.parent
FLAGS = ('DRAWN_MOVE_FORMATIONS_ENABLE', 'MEX_UPGRADE_ENABLE', 'GRID_BUILDING_ENABLE')
DEFAULTS = {'ESCALATION': (1,1,1), 'PROTA': (1,0,1), 'OTA': (0,0,0),
            'TAZERO': (0,0,0), 'BTA': (0,0,0), 'MAYHEM': (0,0,0), 'TWILIGHT': (0,0,0)}


def preprocess(profile, overrides=(), header='config.h', extra=()):
    source = '#include "' + header + '"\nOPTION_VALUES ' + ' '.join(FLAGS) + '\n'
    command = shlex.split(os.environ.get('CXX', 'g++')) + [
        '-std=c++11', '-E', '-P', '-x', 'c++', '-I', str(DDRAW),
        '-DTDRAW_CONFIG_' + profile, *extra,
        *('-D' + name + '=' + str(value) for name, value in overrides), '-']
    return subprocess.run(command, input=source, text=True, capture_output=True)


def values(result):
    assert result.returncode == 0, result.stderr
    line = next(line for line in result.stdout.splitlines() if line.startswith('OPTION_VALUES '))
    return tuple(map(int, line.split()[1:]))


def main():
    for profile, defaults in DEFAULTS.items():
        assert values(preprocess(profile)) == defaults
        for combination in itertools.product((0,1), repeat=3):
            assert values(preprocess(profile, zip(FLAGS, combination))) == combination
        for index, flag in enumerate(FLAGS):
            expected = list(defaults); expected[index] = 1 - expected[index]
            assert values(preprocess(profile, [(flag, expected[index])])) == tuple(expected)
            for bad in (-1,2):
                failure = preprocess(profile, [(flag, bad)])
                assert failure.returncode != 0 and flag + ' must be 0 or 1' in failure.stderr
    all_off = list(zip(FLAGS, (0,0,0)))
    assert values(preprocess('ESCALATION', all_off, 'OptionalUnitControls.h', ['-DTA_HOOK_ENABLE=0'])) == (0,0,0)
    failure = preprocess('ESCALATION', zip(FLAGS, (1,0,0)), 'OptionalUnitControls.h', ['-DTA_HOOK_ENABLE=0'])
    assert failure.returncode != 0 and 'require TA_HOOK_ENABLE=1' in failure.stderr

    namespace = {'m': 'http://schemas.microsoft.com/developer/msbuild/2003'}
    project = ET.parse(DDRAW / 'ddraw.vcxproj').getroot()
    definitions = project.findall('.//m:ClCompile/m:PreprocessorDefinitions', namespace)
    assert any('$(OptionalUnitControlDefinitions)' in (node.text or '') for node in definitions)
    property_group = project.findall('.//m:OptionalUnitControlDefinitions', namespace)
    validation = project.find("m:Target[@Name='ValidateOptionalUnitControls']", namespace)
    assert validation is not None and validation.get('BeforeTargets') == 'PrepareForBuild'
    errors = validation.findall('m:Error', namespace)
    for flag in FLAGS:
        assert any(flag + '=$(' + flag + ')' in (node.text or '') for node in property_group)
        assert any(flag in node.get('Condition', '') and "!= '0'" in node.get('Condition', '')
                   and "!= '1'" in node.get('Condition', '') for node in errors)
    print('Optional controls: all 7 profile defaults and all 8 independent combinations passed')


if __name__ == '__main__':
    main()
