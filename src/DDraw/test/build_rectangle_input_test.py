#!/usr/bin/env python3
"""Check that production WriteDTLine forwards planned building slots unchanged.

The method is extracted from tahook.cpp on every run and compiled with the
native placement call mocked. Geometry has separate tests; this suite checks
that committing rows, columns, rectangles, or rings adds no queue reordering,
extra moves, or placement warnings. Requires g++ (or $CXX).
"""

import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


DDRAW = Path(__file__).resolve().parent.parent


def extract_method(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    tokens = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', re.S)
    depth = 0
    for token in tokens.finditer(source, opening):
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0:
                return source[start:token.end()]
    raise RuntimeError(f"Unclosed production method: {signature}")


HARNESS = r'''
#include <algorithm>
#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

using Point = std::pair<int, int>;
class CTAHook {
public:
    bool WriteLine = true, OptimizeRows = false;
    int FootPrintX = 2, FootPrintY = 3;
    short XMatrix[1000] = {}, YMatrix[1000] = {};
    int optimized = 0, warnings = 0;
    std::vector<Point> dispatched;
    void WriteDTLine();
    void OptimizeDTRows() { ++optimized; }
    void ClickBuilding(int x, int y) { dispatched.emplace_back(x, y); }
    void SendText(char*, int) { ++warnings; }
    void Load(const std::vector<Point>& points) {
        assert(points.size() <= 999);
        // Poison the unused tail so the test detects dispatch beyond either
        // sentinel instead of accidentally finding another zero terminator.
        std::fill(XMatrix, XMatrix + 1000, 1234);
        std::fill(YMatrix, YMatrix + 1000, 5678);
        for (std::size_t i = 0; i < points.size(); ++i) {
            XMatrix[i] = static_cast<short>(points[i].first);
            YMatrix[i] = static_cast<short>(points[i].second);
        }
        XMatrix[points.size()] = YMatrix[points.size()] = -1;
    }
};

// PRODUCTION_METHOD

static void ExpectExactDispatch(const CTAHook& hook, const std::vector<Point>& points) {
    assert(hook.dispatched.size() == points.size() && hook.warnings == 0);
    for (std::size_t i = 0; i < points.size(); ++i) {
        // Native storage is a signed short; preserve its bits/arguments for
        // ClickBuilding, which already handles the native world coordinates.
        assert(hook.dispatched[i].first == static_cast<short>(points[i].first));
        assert(hook.dispatched[i].second == static_cast<short>(points[i].second));
        assert(hook.XMatrix[i] == static_cast<short>(points[i].first));
        assert(hook.YMatrix[i] == static_cast<short>(points[i].second));
    }
}

static void TestPlannedRowsColumnsAndSingleBuildings() {
    const std::vector<std::vector<Point>> plans = {
        {{160,320}, {192,320}, {224,320}, {256,320}},
        {{160,320}, {160,368}, {160,416}, {160,464}},
        {{160,320}},
        // A horizontal sweep of a filled grid. Preserve each row's supplied
        // direction and its transition to the following row exactly.
        {{160,320}, {208,320}, {256,320},
         {256,384}, {208,384}, {160,384},
         {160,448}, {208,448}, {256,448}},
        {{34000,320}, {34048,320}, {34096,320},
         {34096,384}, {34048,384}, {34000,384}}
    };
    for (const auto& plan : plans) {
        for (bool reverse : {false, true}) {
            auto points = plan;
            if (reverse) std::reverse(points.begin(), points.end());
            CTAHook hook;
            // Existing line optimization must never alter a drawn rectangle,
            // including a 2x2-footprint building that could match its old gate.
            hook.OptimizeRows = true; hook.FootPrintX = hook.FootPrintY = 2;
            hook.Load(points); hook.WriteDTLine();
            ExpectExactDispatch(hook, points);
            assert(hook.optimized == 0);
        }
    }
}

static void TestEmptyAndIndependentSentinels() {
    CTAHook empty; empty.Load({}); empty.WriteDTLine();
    ExpectExactDispatch(empty, {});
    for (bool xSentinel : {false, true}) {
        CTAHook hook;
        const std::vector<Point> points = {{160,320}, {192,320}};
        hook.Load(points);
        // The old placement loop stops when EITHER coordinate is terminated.
        if (xSentinel) hook.YMatrix[2] = 5678;
        else hook.XMatrix[2] = 1234;
        hook.WriteDTLine(); ExpectExactDispatch(hook, points);
    }
}

static void TestMaximumCapacity() {
    std::vector<Point> points;
    for (int y = 0; y < 37; ++y) {
        for (int x = 0; x < 27; ++x)
            points.emplace_back(1000 + (y % 2 ? 26-x : x)*16, 1000 + y*16);
    }
    assert(points.size() == 999);
    CTAHook hook; hook.Load(points); hook.WriteDTLine();
    ExpectExactDispatch(hook, points);
    assert(hook.XMatrix[999] == -1 && hook.YMatrix[999] == -1);
}

static void TestExistingRingDelegation() {
    const std::vector<Point> ring = {{320,320}, {352,320}, {352,352}, {320,352}};
    for (bool optimize : {false, true}) {
        for (bool twoSquareFootprint : {false, true}) {
            CTAHook hook;
            hook.WriteLine = false; hook.OptimizeRows = optimize;
            hook.FootPrintX = hook.FootPrintY = twoSquareFootprint ? 2 : 3;
            hook.Load(ring); hook.WriteDTLine();
            ExpectExactDispatch(hook, ring);
            assert(hook.optimized == (optimize && twoSquareFootprint ? 1 : 0));
        }
    }
}

int main() {
    TestPlannedRowsColumnsAndSingleBuildings();
    TestEmptyAndIndependentSentinels();
    TestMaximumCapacity();
    TestExistingRingDelegation();
    std::cout << "Rectangle native dispatch integration tests passed\n";
}
'''


def main():
    source = (DDRAW / "tahook.cpp").read_text()
    method = extract_method(source, "void CTAHook::WriteDTLine()")
    with tempfile.TemporaryDirectory(prefix="rectangle-input-test-") as temp:
        cpp = Path(temp) / "rectangle_input_test.cpp"
        binary = Path(temp) / "rectangle_input_test"
        cpp.write_text(HARNESS.replace("// PRODUCTION_METHOD", method))
        subprocess.run(shlex.split(os.environ.get("CXX", "g++")) + [
            "-std=c++11", "-Wall", "-Wextra", "-Werror", "-g",
            "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
            "-I", str(DDRAW), str(cpp), "-o", str(binary),
        ], check=True)
        env = os.environ.copy()
        # LeakSanitizer cannot run under ptrace in some desktop environments.
        env["ASAN_OPTIONS"] = env.get("ASAN_OPTIONS", "") + ":detect_leaks=0"
        subprocess.run([str(binary)], check=True, env=env)


if __name__ == "__main__":
    main()
