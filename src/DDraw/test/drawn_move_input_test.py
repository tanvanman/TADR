#!/usr/bin/env python3
"""Run the production gesture logic against a small fake engine on Linux.

Only platform headers and MSVC's SEH syntax are adapted. These tests do not
validate SEH unwinding, retail function addresses, or network packet encoding.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--mod', choices=('OTA', 'PROTA', 'ESCALATION', 'BTA',
                                     'MAYHEM', 'TAZERO', 'TWILIGHT'),
                    default='ESCALATION')
parser.add_argument('--feature', choices=('on', 'off', 'both'), default='both',
                    help='Exercise the enabled implementation, disabled no-op controls, or both')
args = parser.parse_args()

root = Path(__file__).resolve().parent
# The megamap must forward rapid presses before native double-click selection.
overview = (root.parent / 'MegamapControl.cpp').read_text()
gate_start = overview.index('#if DRAWN_MOVE_FORMATIONS_ENABLE', overview.index('bool MegaMapControl::Message('))
gate = overview[gate_start:overview.index('#endif', gate_start)]
press_condition = gate[:gate.index('{')]
assert all(message in press_condition for message in (
    'WM_LBUTTONDOWN', 'WM_RBUTTONDOWN', 'WM_LBUTTONDBLCLK', 'WM_RBUTTONDBLCLK'))
assert 'DrawnMove::Message(' in gate
source = (root.parent / 'DrawnMove.cpp').read_text()
source = '\n'.join(line for line in source.splitlines() if not line.startswith('#include') or line == '#include "config.h"' )
source = source.replace('__try', '').replace('__finally', '')


def test_modifier_gates():
    """Exercise the actual marker-drag conditions and rotation message handler."""
    def block(text, signature):
        start = text.index(signature)
        opening = text.index('{', start)
        depth = 0
        for end in range(opening, len(text)):
            depth += (text[end] == '{') - (text[end] == '}')
            if depth == 0:
                return text[start:end + 1]
        raise AssertionError('unclosed production method')

    hook = (root.parent / 'tahook.cpp').read_text()
    def condition(signature, prefix):
        start = hook.index(signature)
        return hook[start:hook.index('{', start)].strip().replace(prefix, 'return ', 1) + ';'
    prime = condition('else if ((ordertype::STOP == TAdynmem->PrepareOrder_Type)', 'else if ')
    drag = condition('if (DraggingUnitOrders &&', 'if ')
    rotation = block((root.parent / 'unitrotate.cpp').read_text(), 'bool CUnitRotate::Message(')
    harness = r"""
#include <cassert>
using HWND=void*; using UINT=unsigned; using WPARAM=unsigned; using LPARAM=unsigned;
constexpr int VK_CONTROL=17, VK_SHIFT=16, VK_OEM_2=191, WM_KEYDOWN=0x100, TAInGame=6;
namespace ordertype { enum { STOP=1, BUILD=14 }; }
struct TAdynmemStruct { int BuildUnitID=1, PrepareOrder_Type=ordertype::BUILD; } world;
TAdynmemStruct* TAdynmem=&world;
struct Data { int TAProgress=TAInGame; } data; Data* DataShare=&data;
bool ctrl=false, shift=false, available=true;
short GetAsyncKeyState(int key) { return ((key==VK_CONTROL && ctrl) || (key==VK_SHIFT && shift)) ? short(0x8000) : 0; }
#if PREPEND_ORDERS_ENABLE
namespace PrependOrders { bool ModifierDown() { return available && ctrl && shift; } }
#endif
class Dialog { public: int GetRotateBuildKey() { return VK_OEM_2; } };
struct Shared { void* Dialog=nullptr; } shared; Shared* LocalShare=&shared;
TAdynmemStruct* GetTA() { return &world; }
class CUnitRotate { public: int calls=0; bool Message(HWND,UINT,WPARAM,LPARAM);
    bool TryCycleRotation(int direction) { assert(direction==1); ++calls; return true; }
};
enum class DraggingOrderStateEnum { IDLE, ACTIVE };
DraggingOrderStateEnum DraggingUnitOrdersState=DraggingOrderStateEnum::IDLE;
struct Unit { void* UnitOrders=nullptr; } unit;
struct Order { void* AttackTargat=nullptr; Unit* Unit_ptr=&unit; } order;
Order* DraggingUnitOrders=&order;
bool FindUnitOrdersUnderMouse() { return true; }
bool IsAnOrder(void*,Order*) { return true; }
bool primeGate() { PRIME }
bool dragGate() { DRAG }
ROTATION
int main() {
    for (bool hasCtrl : {false,true}) for (bool hasShift : {false,true})
        for (bool enabled : {false,true}) {
        ctrl=hasCtrl;shift=hasShift;available=enabled;
        const bool prepend=PREPEND_ORDERS_ENABLE && enabled && ctrl && shift;
        world.PrepareOrder_Type=ordertype::STOP;
        assert(primeGate()==(shift && !prepend));
        assert(dragGate()==(shift && !prepend));
        world.PrepareOrder_Type=ordertype::BUILD;
        CUnitRotate rotation;
        assert(rotation.Message(nullptr,WM_KEYDOWN,VK_OEM_2,0)==(!ctrl || prepend));
        assert(rotation.calls==int(!ctrl || prepend));
        assert(!rotation.Message(nullptr,WM_KEYDOWN,'A',0));
        assert(!rotation.Message(nullptr,0,VK_OEM_2,0));
        world.PrepareOrder_Type=ordertype::STOP;
        assert(!rotation.Message(nullptr,WM_KEYDOWN,VK_OEM_2,0));
    }
}
""".replace('PRIME', prime).replace('DRAG', drag).replace('ROTATION', rotation)
    # The brace-list loops require initializer_list on strict standard libraries.
    harness = '#include <initializer_list>\n#include <cstddef>\n' + harness
    with tempfile.TemporaryDirectory() as temp:
        path = Path(temp)
        (path / 'test.cpp').write_text(harness)
        for enabled in (0, 1):
            subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                            '-DUSEMEGAMAP=0', f'-DPREPEND_ORDERS_ENABLE={enabled}',
                            str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
            subprocess.run([str(path / 'test')], check=True)
    print('Production marker-drag and rotation modifier gates passed (prepend on/off)')

with tempfile.TemporaryDirectory() as temp:
    path = Path(temp)
    # Exercise source-selected feature variants using temporary copies of the
    # real profile headers. Production builds take their gates from config_*.h,
    # never from compiler overrides.
    for header in (root.parent).glob('config*.h'):
        (path / header.name).write_bytes(header.read_bytes())
    (path / 'OptionalUnitControls.h').write_bytes((root.parent / 'OptionalUnitControls.h').read_bytes())
    profile = root.parent / ('config_' + args.mod.lower() + '.h')
    profile_text = profile.read_text()
    from itertools import product
    variants = []
    if args.feature != 'off':
        variants.extend((1, debug, megamap, preview, no_orders, 1)
                        for debug, megamap, preview, no_orders in product((0, 1), repeat=4))
        # Formations still compile and retain Ctrl-to-cancel when prepend is omitted.
        variants.extend((1, 0, megamap, 1, 0, 0) for megamap in (0, 1))
    if args.feature != 'on':
        variants.extend((0, 0, megamap, 1, 0, 1) for megamap in (0, 1))
    for enabled, debug, megamap, preview, no_orders, prepend in variants:
        selected = profile_text
        for name, value in (('DRAWN_MOVE_FORMATIONS_ENABLE', enabled), ('PREPEND_ORDERS_ENABLE', prepend)):
            selected, count = re.subn(r'(#define\s+' + name + r'\s+)[01]\b',
                lambda match: match.group(1) + str(value), selected)
            assert count == 1, name
        (path / profile.name).write_text(selected)
        variant = source.replace('#include "config.h"',
            f'#include "config.h"\n#undef USEMEGAMAP\n#define USEMEGAMAP {megamap}')
        (path / 'test.cpp').write_text(
            '#include "drawn_move_test_engine.h"\n' + variant +
            '\n#include "drawn_move_input_cases.h"\n')
        subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', '-g', f'-DTDRAW_CONFIG_{args.mod}',
                        *(['-D_DEBUG'] if debug else []),
                        f'-DDRAWN_MOVE_TEST_PREVIEW={preview}', f'-DDRAWN_MOVE_TEST_NO_ORDERS={no_orders}',
                        '-I', str(root), '-I', str(root.parent), str(path / 'test.cpp'),
                        '-o', str(path / 'test')], check=True)
        # An omitted feature contributes no build marker or gesture implementation.
        assert (b'[DrawnMove] build=alt-v6' in (path / 'test').read_bytes()) == bool(enabled)
        # Both release knob values must dispatch orders; the diagnostic key is
        # absent from release code, rather than merely defaulting to false.
        assert (b'DrawnMoveTestNoOrders' in (path / 'test').read_bytes()) == bool(enabled and debug)
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

test_modifier_gates()
