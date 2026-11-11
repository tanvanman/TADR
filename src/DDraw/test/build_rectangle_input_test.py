#!/usr/bin/env python3
"""Exercise the production rectangle queue/probe methods with native TA calls mocked.

The C++ bodies below are extracted from tahook.cpp on every run. This checks the
shared battlefield/megamap dispatch sequencing, not Windows messages, pathfinding,
or the separate native retreat-order implementation. Requires g++ (or $CXX).
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
    # Ignore braces in comments and literals while finding the actual method end.
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
#include "BuildRectangleGeometry.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <set>
#include <utility>
#include <vector>

struct Position_Dword {
    std::uint16_t x_, X, z_, Z, y_, Y;
};
static_assert(sizeof(Position_Dword) == 12, "Match the native position layout");
struct FakeTA {
    Position_Dword MouseMapPos = {17, 117, 29, 229, 43, 343};
    int BuildSpotState = 0;
    unsigned CircleSelect_Pos1TAx = 0, CircleSelect_Pos1TAy = 0;
};
enum Kind { Build, InvalidBuild, Stage, Finish, Warning };
struct Event {
    Kind kind;
    int x, y, dx, dy, footX, footY;
};
static std::vector<Event> events;
static int stageCalls, failStage, probes, optimized;
static bool failFinish, mutatePosition, fixedTopLeft;
static int snapX, snapY;
static std::set<std::pair<int, int>> invalidSlots;
static bool IsValid(int x, int y) { return invalidSlots.count({x, y}) == 0; }
static void Reset() {
    events.clear(); invalidSlots.clear();
    stageCalls = failStage = probes = optimized = 0;
    failFinish = mutatePosition = fixedTopLeft = false;
    snapX = 5; snapY = -3;
}

namespace BuildRectangleOrders {
    bool QueueRetreat(int x, int y, int dx, int dy, int footX, int footY, bool final) {
        events.push_back({final ? Finish : Stage, x, y, dx, dy, footX, footY});
        if (final) return !failFinish;
        return ++stageCalls != failStage;
    }
}

class CTAHook {
public:
    FakeTA state;
    FakeTA* TAdynmem = &state;
    bool WriteLine = true, OptimizeRows = false;
    int FootPrintX = 2, FootPrintY = 3;
    int RectangleExitSide = BuildRectangleGeometry::Left, MatrixLength = 0;
    short XMatrix[1000] = {}, YMatrix[1000] = {};
    void WriteDTLine();
    bool ProbeRectangleSlot(int x, int y, int& centerX, int& centerY);
    void OptimizeDTRows() { ++optimized; }
    void SendText(char*, int) { events.push_back({Warning, 0, 0, 0, 0, 0, 0}); }
    void ClickBuilding(int x, int y) {
        x = static_cast<unsigned short>(x); y = static_cast<unsigned short>(y);
        events.push_back({IsValid(x, y) ? Build : InvalidBuild, x, y, 0, 0, 0, 0});
    }
    void TestBuildSpot() {
        ++probes;
        const int x = state.MouseMapPos.X, y = state.MouseMapPos.Y;
        // Return native-style footprint corners. Production code must convert
        // these to a centre, rather than use unsnapped mouse coordinates.
        state.CircleSelect_Pos1TAx = fixedTopLeft ? 120 : x + snapX - FootPrintX * 8;
        state.CircleSelect_Pos1TAy = fixedTopLeft ? 200 : y + snapY - FootPrintY * 8;
        state.BuildSpotState = IsValid(x, y) ? 70 : 6;
        // Native hooks may touch fractional words/elevation as well as X/Y.
        if (mutatePosition) state.MouseMapPos = {1, 2, 3, 4, 5, 6};
    }
    void Load(const std::vector<BuildRectangleGeometry::Point>& points) {
        assert(!points.empty() && points.size() <= 999);
        for (std::size_t i = 0; i < points.size(); ++i) {
            XMatrix[i] = static_cast<short>(points[i].x);
            YMatrix[i] = static_cast<short>(points[i].y);
        }
        XMatrix[points.size()] = YMatrix[points.size()] = -1;
        MatrixLength = static_cast<int>(points.size()) - 1;
    }
};

// PRODUCTION_METHODS

static int Count(Kind kind) {
    return static_cast<int>(std::count_if(events.begin(), events.end(),
        [kind](const Event& event) { return event.kind == kind; }));
}
static void ExpectRetreat(const Event& event, Kind kind, int x, int y,
    int dx, int dy, const CTAHook& hook) {
    assert(event.kind == kind && event.x == x && event.y == y);
    assert(event.dx == dx && event.dy == dy);
    assert(event.footX == hook.FootPrintX && event.footY == hook.FootPrintY);
}

static void TestClosingOrderAllSides() {
    // All four exits and two coordinate regions exercise the same queue method
    // used by both map views. The high region also tests signed-short storage.
    const int builders[][2] = {{140,384}, {276,384}, {208,300}, {208,468}};
    for (int origin : {0, 34000}) {
        for (int side = BuildRectangleGeometry::Left; side <= BuildRectangleGeometry::Bottom; ++side) {
            Reset(); CTAHook hook;
            std::vector<BuildRectangleGeometry::Point> points;
            BuildRectangleGeometry::Side exitSide;
            assert(BuildRectangleGeometry::Generate(160 + origin, 320, 256 + origin, 448,
                2, 3, 1, builders[side][0] + origin, builders[side][1], 999, points, &exitSide));
            assert(exitSide == side && points.size() == 9);
            hook.RectangleExitSide = exitSide; hook.Load(points);
            hook.WriteDTLine();
            const bool exitOnX = side == BuildRectangleGeometry::Left || side == BuildRectangleGeometry::Right;
            const int dx = exitOnX ? (side == BuildRectangleGeometry::Left ? -1 : 1) : 0;
            const int dy = !exitOnX ? (side == BuildRectangleGeometry::Top ? -1 : 1) : 0;
            const int strip = exitOnX ? points.back().x : points.back().y;
            std::size_t next = 0;
            for (const auto& point : points) {
                if ((exitOnX ? point.x : point.y) == strip)
                    ExpectRetreat(events.at(next++), Stage, point.x + snapX, point.y + snapY, dx, dy, hook);
                const Event& build = events.at(next++);
                assert(build.kind == Build && build.x == point.x && build.y == point.y);
            }
            ExpectRetreat(events.at(next++), Finish, points.back().x + snapX,
                points.back().y + snapY, dx, dy, hook);
            assert(next == events.size() && probes == 9 && stageCalls == 3 && optimized == 0);
        }
    }
}

static void TestProbeCenterAndFullRestore() {
    for (const auto& footprint : std::vector<std::pair<int,int>>{{1,3}, {2,4}, {3,1}}) {
        for (bool valid : {false, true}) {
            Reset(); CTAHook hook;
            hook.FootPrintX = footprint.first; hook.FootPrintY = footprint.second;
            fixedTopLeft = mutatePosition = true;
            if (!valid) invalidSlots.insert({40001, 503});
            const Position_Dword before = hook.state.MouseMapPos;
            int x = -1, y = -1;
            assert(hook.ProbeRectangleSlot(static_cast<short>(40001), 503, x, y) == valid);
            assert(x == 120 + footprint.first * 8 && y == 200 + footprint.second * 8);
            assert(std::memcmp(&before, &hook.state.MouseMapPos, sizeof(before)) == 0);
            assert(probes == 1 && events.empty());
        }
    }
}

static void TestFailureLeavesEntryOpen() {
    Reset(); CTAHook hook;
    hook.Load({{128,192}, {128,224}, {96,224}, {96,192}, {96,160}});
    failStage = 2;
    hook.WriteDTLine();
    assert(Count(Build) == 3 && probes == 4 && Count(Stage) == 2);
    assert(Count(Finish) == 0 && Count(Warning) == 1);
    assert(events.at(events.size() - 2).kind == Stage && events.back().kind == Warning);
    // The failed entry (96,192) and every later construction are unqueued.
    for (const Event& event : events)
        assert(event.kind != Build || event.x != 96 || event.y == 224);

    Reset(); CTAHook finalFailure;
    finalFailure.Load({{128,192}, {96,192}}); failFinish = true;
    finalFailure.WriteDTLine();
    assert(Count(Build) == 2 && Count(Stage) == 1 && Count(Finish) == 1);
    assert(events.back().kind == Warning);
}

static void TestInvalidFinalSlot() {
    for (bool exitOnX : {false, true}) {
        // Once with a valid closing cell, once with the entire closing strip
        // blocked: retain the rectangle boundary on its normal axis, but use
        // the last actual construction's centre along the tangent axis.
        for (bool validClosingCell : {false, true}) {
            Reset(); CTAHook hook;
            hook.RectangleExitSide = exitOnX ? BuildRectangleGeometry::Left : BuildRectangleGeometry::Top;
            const std::vector<BuildRectangleGeometry::Point> points = exitOnX
                ? std::vector<BuildRectangleGeometry::Point>{{128,256}, {96,256}, {96,224}, {96,192}}
                : std::vector<BuildRectangleGeometry::Point>{{256,128}, {256,96}, {224,96}, {192,96}};
            hook.Load(points);
            for (std::size_t i = validClosingCell ? 3 : 1; i < points.size(); ++i)
                invalidSlots.insert({points[i].x, points[i].y});
            hook.WriteDTLine();
            assert(probes == 4 && Count(Build) == (validClosingCell ? 3 : 1));
            assert(Count(Stage) == (validClosingCell ? 2 : 0) && Count(Finish) == 1);
            const auto& lastValid = points[validClosingCell ? 2 : 0];
            ExpectRetreat(events.back(), Finish,
                (exitOnX ? points.back().x : lastValid.x) + snapX,
                (exitOnX ? lastValid.y : points.back().y) + snapY,
                exitOnX ? -1 : 0, exitOnX ? 0 : -1, hook);
        }
    }
    Reset(); CTAHook allInvalid;
    allInvalid.Load({{128,192}, {96,192}});
    invalidSlots = {{128,192}, {96,192}};
    allInvalid.WriteDTLine();
    assert(probes == 2 && Count(InvalidBuild) == 2 && Count(Build) == 0);
    assert(Count(Stage) == 0 && Count(Finish) == 0);
}

static void TestRingHasNoRectangleSideEffects() {
    Reset(); CTAHook hook;
    hook.WriteLine = false; hook.OptimizeRows = true; hook.FootPrintX = hook.FootPrintY = 2;
    hook.Load({{320,320}, {352,320}, {352,352}, {320,352}});
    // Ring generation need not populate rectangle-only metadata.
    hook.MatrixLength = 1000000;
    hook.WriteDTLine();
    assert(probes == 0 && optimized == 1 && events.size() == 4);
    assert(events[0].kind == Build && events[0].x == 320 && events[0].y == 320);
    assert(events[1].kind == Build && events[1].x == 352 && events[1].y == 320);
    assert(events[2].kind == Build && events[2].x == 352 && events[2].y == 352);
    assert(events[3].kind == Build && events[3].x == 320 && events[3].y == 352);
}

static void TestMaximumCapacityAndSentinel() {
    Reset(); CTAHook hook;
    std::vector<BuildRectangleGeometry::Point> points;
    for (int i = 998; i >= 0; --i) points.push_back({1000 + i * 16, 320});
    hook.Load(points);
    assert(hook.XMatrix[999] == -1 && hook.YMatrix[999] == -1);
    hook.WriteDTLine();
    assert(probes == 999 && Count(Build) == 999 && Count(Stage) == 1 && Count(Finish) == 1);
    assert(Count(Warning) == 0 && hook.XMatrix[999] == -1 && hook.YMatrix[999] == -1);
}

int main() {
    TestClosingOrderAllSides();
    TestProbeCenterAndFullRestore();
    TestFailureLeavesEntryOpen();
    TestInvalidFinalSlot();
    TestRingHasNoRectangleSideEffects();
    TestMaximumCapacityAndSentinel();
    std::cout << "Rectangle queue integration tests passed\n";
}
'''


def main():
    source = (DDRAW / "tahook.cpp").read_text()
    methods = "\n\n".join(extract_method(source, signature) for signature in (
        "void CTAHook::WriteDTLine()",
        "bool CTAHook::ProbeRectangleSlot(int x, int y, int& centerX, int& centerY)",
    ))
    with tempfile.TemporaryDirectory(prefix="rectangle-input-test-") as temp:
        cpp = Path(temp) / "rectangle_input_test.cpp"
        binary = Path(temp) / "rectangle_input_test"
        cpp.write_text(HARNESS.replace("// PRODUCTION_METHODS", methods))
        command = shlex.split(os.environ.get("CXX", "g++")) + [
            "-std=c++11", "-Wall", "-Wextra", "-Werror", "-g",
            "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
            "-I", str(DDRAW), str(cpp), "-o", str(binary),
        ]
        subprocess.run(command, check=True)
        env = os.environ.copy()
        # LeakSanitizer cannot run under ptrace in some desktop environments.
        env["ASAN_OPTIONS"] = env.get("ASAN_OPTIONS", "") + ":detect_leaks=0"
        subprocess.run([str(binary)], check=True, env=env)


if __name__ == "__main__":
    main()
