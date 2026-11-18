#!/usr/bin/env python3
"""Verify that disabling grid building restores the stock row tool.

Runs the production row calculation/dispatch with GRID_BUILDING_ENABLE=0,
and preprocesses the actual input paths for every independent feature mix.
The enabled grid geometry and gestures have their own regression suites.
"""

import itertools
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
from build_rectangle_input_test import extract_method


DDRAW = Path(__file__).resolve().parent.parent
CXX = shlex.split(os.environ.get("CXX", "g++"))


def preprocess(source, formations, mex, grid):
    source = re.sub(r'^\s*#include[^\n]*$', '', source, flags=re.M)
    result = subprocess.run(CXX + [
        '-E', '-P', '-x', 'c++', '-DUSEMEGAMAP=1',
        '-DDRAWN_MOVE_FORMATIONS_ENABLE=' + str(formations),
        '-DMEX_UPGRADE_ENABLE=' + str(mex),
        '-DGRID_BUILDING_ENABLE=' + str(grid), '-'],
        input=source, text=True, capture_output=True, check=True)
    return result.stdout


HARNESS = r'''
#include <algorithm>
#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

class CTAHook {
public:
    int StartX = 160, StartY = 320, EndX = 160, EndY = 320;
    int FootPrintX = 2, FootPrintY = 2, Spacing = 0, MatrixLength = 0, Direction = 0;
    bool OptimizeRows = false, WriteLine = true;
    short XMatrix[1000], YMatrix[1000];
    int optimized = 0;
    std::vector<std::pair<int,int>> dispatched;
    CTAHook() {
        std::fill(XMatrix,XMatrix+1000,-1);
        std::fill(YMatrix,YMatrix+1000,-1);
    }
    void CalculateLine();
    void WriteDTLine();
    void OptimizeDTRows() { ++optimized; }
    void ClickBuilding(int x, int y) { dispatched.emplace_back(x,y); }
};

// PRODUCTION_METHODS

static void ExpectPlan(CTAHook& hook, const std::vector<std::pair<int,int>>& expected, int direction) {
    hook.CalculateLine();
    assert(hook.Direction == direction && hook.MatrixLength == static_cast<int>(expected.size())-1);
    assert(hook.XMatrix[expected.size()] == -1 && hook.YMatrix[expected.size()] == -1);
    hook.WriteDTLine();
    assert(hook.dispatched == expected);
}

int main() {
    CTAHook horizontal;
    horizontal.EndX = 288;
    ExpectPlan(horizontal,{{160,320},{192,320},{224,320},{256,320},{288,320}},1);

    CTAHook spaced;
    spaced.EndX = 288; spaced.Spacing = 1;
    ExpectPlan(spaced,{{160,320},{208,320},{256,320}},1);

    CTAHook reverse;
    reverse.StartY = 448;
    ExpectPlan(reverse,{{160,448},{160,416},{160,384},{160,352},{160,320}},2);

    CTAHook diagonal;
    diagonal.EndX = 288; diagonal.EndY = 448;
    ExpectPlan(diagonal,{{160,320},{192,352},{224,384},{256,416},{288,448}},2);

    CTAHook single;
    ExpectPlan(single,{{160,320}},2);

    CTAHook optimized;
    optimized.EndX = 224; optimized.OptimizeRows = true;
    ExpectPlan(optimized,{{160,320},{192,320},{224,320}},1);
    assert(optimized.optimized == 1); // Stock rows retain their existing optimizer.
    std::cout << "Disabled grid: stock row geometry and dispatch passed\n";
}
'''


def main():
    source = (DDRAW / 'tahook.cpp').read_text()
    overview = (DDRAW / 'MegamapControl.cpp').read_text()
    header = (DDRAW / 'tahook.h').read_text()
    for formations, mex, grid in itertools.product((0,1),repeat=3):
        output = preprocess(source,formations,mex,grid)
        mega = preprocess(overview,formations,mex,grid)
        api = preprocess(header,formations,mex,grid)
        assert ('bool CTAHook::RectangleGestureMessage(' in output) == bool(grid)
        assert ('if (RectangleGestureMessage(' in output) == bool(grid)
        assert ('[BuildRectangle] click-v5' in output) == bool(grid)
        assert ('RefreshRectangleFootprint();' in output) == bool(grid)
        assert ('bool MegaMapControl::UpdateBuildMouse(' in mega) == bool(grid)
        assert ('DrawnMove::Message(' in output) == bool(formations)
        assert ('DrawnMove::Message(' in mega) == bool(formations)
        assert ('bool MegaMapControl::FormationGroundPoint(' in mega) == bool(formations)
        if grid:
            assert 'else if(WriteLine==true' not in output
            assert 'BuildRectangleGeometry::Generate(' in output
        else:
            assert 'else if(WriteLine==true' in output
            assert 'else if(WriteLine)' in output
            assert 'BuildRectangleGeometry::Generate(' not in output
            assert 'bool OwnsRectangleInput() const { return false; }' in api

    methods = '\n\n'.join(extract_method(source,signature) for signature in (
        'void CTAHook::CalculateLine()', 'void CTAHook::WriteDTLine()'))
    with tempfile.TemporaryDirectory(prefix='grid-switch-test-') as temp:
        cpp = Path(temp) / 'stock_rows.cpp'
        binary = Path(temp) / 'stock_rows'
        cpp.write_text(HARNESS.replace('// PRODUCTION_METHODS',methods))
        subprocess.run(CXX + ['-std=c++11','-Wall','-Wextra','-Werror','-g',
            '-DGRID_BUILDING_ENABLE=0','-fsanitize=address,undefined',
            '-fno-omit-frame-pointer',str(cpp),'-o',str(binary)],check=True)
        env = os.environ.copy()
        env['ASAN_OPTIONS'] = env.get('ASAN_OPTIONS','') + ':detect_leaks=0'
        subprocess.run([str(binary)],check=True,env=env)
    print('Independent grid/formation input switches passed all eight feature combinations')


if __name__ == '__main__':
    main()
