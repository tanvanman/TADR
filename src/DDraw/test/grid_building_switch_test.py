#!/usr/bin/env python3
"""Verify that disabling grid building restores the stock row tool.

Compares production legacy row plans with the enabled grid tool's live Alt
override, and preprocesses input paths for every independent feature mix.
Also exercises default grids, planned-mode dispatch and capacity boundaries.
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
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include "BuildRectangleGeometry.h"

constexpr int VK_MENU = 0x12;
constexpr unsigned UnitSelected_State = 0x10, canmove = 1, canfly = 2;
bool altHeld = false;
short GetAsyncKeyState(int key) { assert(key == VK_MENU); return altHeld ? static_cast<short>(0x8000) : 0; }
struct PlayerStruct;
struct UnitDefStruct { void* CANBUILD_ptr = nullptr; unsigned UnitTypeMask_0 = 0; };
struct UnitStruct {
    int UnitID = 0, XPos = 0, YPos = 0;
    PlayerStruct* Owner_PlayerPtr0 = nullptr;
    UnitDefStruct* UnitType = nullptr;
    unsigned UnitSelected = 0;
    void* IsUnit = nullptr;
    float Nanoframe = 0;
};
struct PlayerStruct { UnitStruct* Units = nullptr; UnitStruct* UnitsAry_End = nullptr; };
namespace ordertype { constexpr int BUILD = 14; }
struct FakeTA { int LocalHumanPlayer_PlayerID = 0, BuildUnitID = 1, PrepareOrder_Type = ordertype::BUILD; PlayerStruct Players[10]; };
class CUnitRotate {
public:
    static CUnitRotate* GetInstance() { return nullptr; }
    void ApplyRotationTo(int) {}
};

class CTAHook {
public:
    int StartX = 160, StartY = 320, EndX = 160, EndY = 320;
    int FootPrintX = 2, FootPrintY = 2, Spacing = 0, MatrixLength = 0, Direction = 0;
    bool OptimizeRows = false, WriteLine = true, RectangleValid = false, RectangleLineMode = false;
    FakeTA state;
    FakeTA* TAdynmem = &state;
    short XMatrix[1000], YMatrix[1000];
    int optimized = 0;
    std::vector<std::pair<int,int>> dispatched;
    CTAHook() {
        std::fill(XMatrix,XMatrix+1000,-1);
        std::fill(YMatrix,YMatrix+1000,-1);
    }
    void CalculateLine();
    void WriteDTLine();
    void RefreshRectangleFootprint();
    int GetFootX() { return FootPrintX; }
    int GetFootY() { return FootPrintY; }
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

static void TestKnownRows() {
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
    std::cout << "Original row examples and optimizer passed\n";
}

static std::vector<std::pair<int,int>> Snapshot(const CTAHook& hook) {
    std::vector<std::pair<int,int>> points;
    for (int i=0; i<1000; ++i) {
        if (hook.XMatrix[i] == -1 || hook.YMatrix[i] == -1) return points;
        points.emplace_back(hook.XMatrix[i],hook.YMatrix[i]);
    }
    assert(false && "Missing native matrix sentinel");
    return points;
}

static void TestRowsAcrossAnglesAndSpacing() {
    int id = 0;
    for (const auto& footprint : std::vector<std::pair<int,int>>{{1,1},{2,2},{2,3},{5,7}})
    for (int spacing : {0,1,3})
    for (const auto& delta : std::vector<std::pair<int,int>>{{192,64},{64,192},{192,192},{0,192},{192,0},{0,0}})
    for (int sx : {-1,1}) for (int sy : {-1,1}) {
        CTAHook hook;
        hook.StartX = hook.StartY = 4096;
        hook.EndX = hook.StartX + sx*delta.first; hook.EndY = hook.StartY + sy*delta.second;
        hook.FootPrintX = footprint.first; hook.FootPrintY = footprint.second; hook.Spacing = spacing;
        hook.CalculateLine();
        const auto points = Snapshot(hook);
        assert(!points.empty() && points.size() <= 999);
#if GRID_BUILDING_ENABLE
        assert(hook.RectangleLineMode && hook.RectangleValid);
#endif
        hook.WriteDTLine(); assert(hook.dispatched == points);
        std::cout << "PLAN " << id++ << ' ' << hook.Direction << ' ' << points.size();
        for (const auto& p : points) std::cout << ' ' << p.first << ',' << p.second;
        std::cout << '\n';
    }
    assert(id == 288);
}

static void TestRowCapacityAndSafeRejection() {
    for (bool vertical : {false,true}) {
        CTAHook hook;
        hook.FootPrintX = hook.FootPrintY = 1;
        if (vertical) hook.EndY += 998*16;
        else hook.EndX += 998*16;
        hook.CalculateLine(); assert(Snapshot(hook).size() == 999);
        assert(hook.XMatrix[999] == -1 && hook.YMatrix[999] == -1);
        hook.WriteDTLine(); assert(hook.dispatched.size() == 999);
        hook.dispatched.clear();
        if (vertical) hook.EndY += 16;
        else hook.EndX += 16;
        hook.CalculateLine(); assert(Snapshot(hook).empty());
        assert(hook.Direction == 0 && hook.MatrixLength == 0);
#if GRID_BUILDING_ENABLE
        assert(!hook.RectangleValid);
#endif
        hook.WriteDTLine(); assert(hook.dispatched.empty());
    }
}

#if GRID_BUILDING_ENABLE
static void TestDefaultGridAndLiveModeRefresh() {
    CTAHook hook; hook.EndX = 224; hook.EndY = 384; hook.OptimizeRows = true;
    const std::vector<std::pair<int,int>> rectangle = {
        {224,384},{192,384},{160,384},{160,352},{192,352},{224,352},{224,320},{192,320},{160,320}};
    const std::vector<std::pair<int,int>> diagonal = {{160,320},{192,352},{224,384}};
    hook.CalculateLine();
    assert(!hook.RectangleLineMode && hook.RectangleValid && hook.Direction == 0);
    assert(Snapshot(hook) == rectangle);
    hook.WriteDTLine(); assert(hook.dispatched == rectangle && hook.optimized == 0);

    hook.dispatched.clear(); altHeld = true;
    hook.RefreshRectangleFootprint(); // A missed key message still updates stationary previews.
    assert(hook.RectangleLineMode && Snapshot(hook) == diagonal);
    altHeld = false; // Dispatch must use the previewed mode, not another key poll.
    hook.WriteDTLine(); assert(hook.dispatched == diagonal && hook.optimized == 1);

    hook.dispatched.clear(); hook.RefreshRectangleFootprint();
    assert(!hook.RectangleLineMode && Snapshot(hook) == rectangle);
    hook.WriteDTLine(); assert(hook.dispatched == rectangle && hook.optimized == 1);

    CTAHook capacity;
    capacity.StartX = capacity.StartY = 0;
    capacity.FootPrintX = capacity.FootPrintY = 1;
    capacity.EndX = 26*16; capacity.EndY = 36*16;
    capacity.CalculateLine(); assert(capacity.RectangleValid && Snapshot(capacity).size() == 999);
    capacity.EndX += 16; capacity.CalculateLine();
    assert(!capacity.RectangleValid && Snapshot(capacity).empty());
    std::cout << "Default grids, live Alt refresh and planned-mode dispatch passed\n";
}
#endif

int main(int argc, char** argv) {
    assert(argc == 2); altHeld = std::string(argv[1]) == "1";
#if GRID_BUILDING_ENABLE
    if (!altHeld) { TestDefaultGridAndLiveModeRefresh(); return 0; }
#endif
    TestKnownRows();
    TestRowsAcrossAnglesAndSpacing();
    TestRowCapacityAndSafeRejection();
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
        assert ('[BuildRectangle] row-alt-v6' in output) == bool(grid)
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
    methods += '\n#if GRID_BUILDING_ENABLE\n' + extract_method(source,
        'void CTAHook::RefreshRectangleFootprint()') + '\n#endif\n'
    with tempfile.TemporaryDirectory(prefix='grid-switch-test-') as temp:
        cpp = Path(temp) / 'stock_rows.cpp'
        cpp.write_text(HARNESS.replace('// PRODUCTION_METHODS',methods))
        env = os.environ.copy()
        env['ASAN_OPTIONS'] = env.get('ASAN_OPTIONS','') + ':detect_leaks=0'
        results = []
        for grid in (0,1):
            binary = Path(temp) / ('rows_' + str(grid))
            subprocess.run(CXX + ['-std=c++11','-Wall','-Wextra','-Werror','-g',
                '-DGRID_BUILDING_ENABLE='+str(grid),'-fsanitize=address,undefined',
                '-fno-omit-frame-pointer','-I',str(DDRAW),str(cpp),'-o',str(binary)],check=True)
            run = subprocess.run([str(binary),'1'],check=True,env=env,capture_output=True,text=True)
            results.append([line for line in run.stdout.splitlines() if line.startswith('PLAN ')])
            if grid: subprocess.run([str(binary),'0'],check=True,env=env)
        assert results[0] == results[1] and len(results[0]) == 288
    print('Legacy and Alt row plans match for 288 angle/sign/footprint/spacing cases')
    print('Independent grid/formation input switches passed all eight feature combinations')


if __name__ == '__main__':
    main()
