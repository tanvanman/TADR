#!/usr/bin/env python3
"""Drive the production rectangle gesture and coordinate helpers with mocked TA.

This tests press/drag/release ownership and both view-conversion branches using
the actual methods extracted from tahook.cpp. Placement/pathfinding are mocked;
their queue sequencing is covered separately by build_rectangle_input_test.py.
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


def normalize_finally(method):
    """Use RAII for the helper's SEH cleanup when compiling the Linux harness."""
    while "__try" in method:
        start = method.index("__try")
        try_body = extract_method(method, "__try")
        final_start = method.index("__finally", start + len(try_body))
        final_body = extract_method(method[final_start:], "__finally")
        protected = try_body[try_body.index("{") + 1:-1]
        cleanup = final_body[final_body.index("{") + 1:-1]
        replacement = "{ Finally finallyGuard{[&]() {" + cleanup + "}}; {" + protected + "} }"
        method = method[:start] + replacement + method[final_start + len(final_body):]
    return method


HARNESS = r'''
#include "SmallMapViewport.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

#define USEMEGAMAP 1
#define MAX_SPACING 10
using HWND = void*;
using UINT = unsigned;
using WPARAM = std::uintptr_t;
using LPARAM = std::intptr_t;
struct RECT { int left, top, right, bottom; };
static unsigned LOWORD(std::uintptr_t value) { return value & 0xffff; }
static unsigned HIWORD(std::uintptr_t value) { return (value >> 16) & 0xffff; }
static LPARAM Point(int x, int y) {
    return static_cast<unsigned short>(x) | (std::uintptr_t(static_cast<unsigned short>(y)) << 16);
}
enum {
    WM_KILLFOCUS = 0x0008, WM_CANCELMODE = 0x001f, WM_ACTIVATEAPP = 0x001c,
    WM_KEYDOWN = 0x0100, WM_KEYUP = 0x0101, WM_CHAR = 0x0102,
    WM_MOUSEMOVE = 0x0200, WM_LBUTTONDOWN = 0x0201, WM_LBUTTONUP = 0x0202,
    WM_LBUTTONDBLCLK = 0x0203, WM_RBUTTONDOWN = 0x0204, WM_RBUTTONUP = 0x0205,
    WM_MOUSEWHEEL = 0x020a, WM_CAPTURECHANGED = 0x0215,
    VK_PRIOR = 0x21, VK_NEXT = 0x22, VK_ESCAPE = 0x1b,
    VK_LBUTTON = 1, VK_MENU = 0x12, VK_SHIFT = 0x10, VK_CONTROL = 0x11, VK_F2 = 0x71,
    MK_LBUTTON = 1, MK_SHIFT = 4, VK_OEM_5 = 0xdc
};
static std::map<int, bool> keys;
static short GetAsyncKeyState(int key) { return keys[key] ? static_cast<short>(0x8000) : 0; }
struct Finally { std::function<void()> action; ~Finally() { action(); } };
static HWND capturedWindow;
static HWND TAhWnd;
static bool failCapture;
static std::function<void()> onCaptureRelease;
static int captureSets, captureReleases;
static HWND GetCapture() { return capturedWindow; }
static HWND SetCapture(HWND window) {
    HWND previous = capturedWindow; ++captureSets;
    if (!failCapture) capturedWindow = window;
    return previous;
}
static bool ReleaseCapture() {
    capturedWindow = nullptr; ++captureReleases;
    if (onCaptureRelease) onCaptureRelease();
    return true;
}
namespace ChatLayout { static bool ScrollbackWheel(short) { return false; } }
class Dialog { public: int GetClickSnapOverrideKey() { return VK_MENU; } };
struct SharedLocal { void* Dialog = nullptr; } localShare;
static SharedLocal* LocalShare = &localShare;
class CUnitRotate {
public:
    static CUnitRotate* GetInstance() { return nullptr; }
    bool TryCycleRotation(int) { return false; }
};
namespace ordertype { enum { STOP = 1, BUILD = 14 }; }
enum { TAInGame = 1 };
struct SharedData { int TAProgress = TAInGame; } sharedData;
static SharedData* DataShare = &sharedData;
struct Position_Dword { std::uint16_t x_, X, z_, Z, y_, Y; };
struct FakeTA {
    Position_Dword MouseMapPos = {};
    int PrepareOrder_Type = ordertype::BUILD;
    RECT GameSreen_Rect = {128,32,1023,767};
    int EyeBallMapXPos = 256, EyeBallMapYPos = 512;
    int MapSizeX = 4096, MapSizeY = 4096;
    int FeatureMapSizeX = 256, FeatureMapSizeY = 264;
};
static int normalConversions, overviewConversions;
static int GetPosition_Dword(int x, int y, Position_Dword* point) {
    ++normalConversions;
    point->X = x; point->Y = y + 8; point->Z = 16;
    return 1;
}
struct Controller {
    bool overview = false;
    FakeTA* ta = nullptr;
    bool IsBliting() const { return overview; }
    bool UpdateBuildMouse(int x, int y) {
        ++overviewConversions;
        if (x < 128 || x >= 1024 || y < 32 || y >= 768) return false;
        ta->MouseMapPos.X = (x - 128) * 4;
        ta->MouseMapPos.Y = (y - 32) * 4 + 8;
        ta->MouseMapPos.Z = 16;
        return true;
    }
};
struct Minimap { Controller* Controler; };
struct Expander { Minimap* myMinimap; };
static Controller control;
static Minimap minimap = {&control};
static Expander expander = {&minimap};
static Expander* GUIExpander = &expander;
struct Commit { int startX, startY, endX, endY, spacing; bool active; };
class CTAHook {
public:
    FakeTA state;
    FakeTA* TAdynmem = &state;
    int VirtualKeyCode = VK_OEM_5;
    bool WriteLine = false, RingWrite = false, RectangleValid = false;
    bool RectangleMouseDown = false, RectangleCaptured = false;
    bool ClickSnapPreviewBuild = false, ClickSnapPreviewWreck = false;
    int ClickSnapPreviewPosXY[2] = {};
    int StartX = 0, StartY = 0, EndX = 0, EndY = 0;
    int FootPrintX = 2, FootPrintY = 3, Spacing = 0, MatrixLength = 0;
    short XMatrix[1000] = {-1}, YMatrix[1000] = {-1};
    int calculations = 0, nativeRectEnabled = 0, nativeRectDisabled = 0, warnings = 0;
    bool rejectGeometry = false;
    bool throwOnCommit = false;
    std::vector<Commit> commits;
    CTAHook() {
        keys.clear(); sharedData.TAProgress = TAInGame; control.ta = &state;
        capturedWindow = nullptr; onCaptureRelease = nullptr; captureSets = captureReleases = 0;
        TAhWnd = nullptr; failCapture = false;
    }
    bool UpdateRectangleMouse(LPARAM lParam);
    // GESTURE_DECLARATION
    void EndRectangleGesture(HWND WinProcWnd, bool keepRelease);
    bool SpacingMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
    void EnableTABuildRect() { ++nativeRectEnabled; }
    void DisableTABuildRect() { ++nativeRectDisabled; }
    short GetFootX() { return 2; }
    short GetFootY() { return 3; }
    void SendText(char*, int) { ++warnings; }
    void CalculateLine() {
        ++calculations; RectangleValid = !rejectGeometry;
        XMatrix[0] = RectangleValid ? StartX : -1;
        YMatrix[0] = RectangleValid ? StartY : -1;
    }
    void UpdateSpacing();
    void CalculateRing() {}
    void WriteDTLine() {
        commits.push_back({StartX,StartY,EndX,EndY,Spacing,WriteLine});
        if (throwOnCommit) throw std::runtime_error("mock native dispatch failure");
    }
    bool Dispatch(UINT msg, WPARAM wParam = 0, LPARAM point = 0) {
        // GESTURE_DISPATCH
        return SpacingMessage(msg, wParam, point);
    }
};

// PRODUCTION_METHODS

static void Begin(CTAHook& hook, bool overview, LPARAM point = Point(200,120)) {
    control.overview = overview;
    keys[hook.VirtualKeyCode] = true; keys[VK_LBUTTON] = true;
    assert(hook.Dispatch(WM_LBUTTONDOWN, MK_LBUTTON, point));
    assert(hook.WriteLine && hook.commits.empty());
    assert(hook.RectangleMouseDown && hook.RectangleCaptured && GetCapture() != nullptr);
    assert(TAhWnd == reinterpret_cast<HWND>(1));
}
static void ExpectCleared(const CTAHook& hook) {
    assert(!hook.WriteLine && !hook.RectangleValid);
    assert(hook.XMatrix[0] == -1 && hook.YMatrix[0] == -1);
}

static void TestDragReleaseBothViews() {
    for (bool overview : {false,true}) {
        normalConversions = overviewConversions = 0;
        CTAHook hook; Begin(hook, overview);
        assert(hook.calculations == 1 && hook.nativeRectDisabled == 1);
        const int startX = overview ? (200-128)*4 : 200-128+256;
        const int startY = overview ? (120-32)*4+8 : 120-32+512+8;
        assert(hook.StartX == startX && hook.StartY == startY);
        assert(hook.Dispatch(WM_MOUSEMOVE, MK_LBUTTON, Point(250,180)));
        assert(hook.calculations == 2 && hook.commits.empty());
        // Release has a newer position than the last move. Native/map state is
        // deliberately stale so an UP that reuses the preview endpoint fails.
        hook.state.MouseMapPos.X = 1; hook.state.MouseMapPos.Y = 2;
        keys[VK_LBUTTON] = false;
        assert(hook.Dispatch(WM_LBUTTONUP, 0, Point(280,220)));
        assert(hook.commits.size() == 1 && hook.calculations == 3);
        const Commit& commit = hook.commits[0];
        assert(commit.active && commit.startX == startX && commit.startY == startY);
        assert(commit.endX == (overview ? (280-128)*4 : 280-128+256));
        assert(commit.endY == (overview ? (220-32)*4+8 : 220-32+512+8));
        ExpectCleared(hook);
        assert(!hook.RectangleMouseDown && !hook.RectangleCaptured && GetCapture() == nullptr);
        assert(hook.nativeRectEnabled >= 1);
        hook.Dispatch(WM_LBUTTONUP, 0, Point(280,220));
        hook.Dispatch(WM_MOUSEMOVE, 0, Point(290,230));
        assert(hook.commits.size() == 1); // No duplicate or post-release preview.
        assert(overview ? overviewConversions == 3 && normalConversions == 0
                        : normalConversions == 3 && overviewConversions == 0);
        // The configured key may remain held for a second independent drag.
        keys[VK_LBUTTON] = true;
        assert(hook.Dispatch(WM_LBUTTONDOWN, MK_LBUTTON, Point(300,240)));
        assert(hook.commits.size() == 1 && hook.WriteLine);
        keys[VK_LBUTTON] = false;
        assert(hook.Dispatch(WM_LBUTTONUP, 0, Point(340,280)));
        assert(hook.commits.size() == 2);
        ExpectCleared(hook);
    }
}

static void TestConfiguredKeyAndNativePassThrough() {
    CTAHook hook; control.overview = false;
    keys['X'] = keys[VK_LBUTTON] = true;
    assert(!hook.Dispatch(WM_LBUTTONDOWN, MK_LBUTTON, Point(200,120)));
    assert(!hook.WriteLine && hook.calculations == 0 && hook.commits.empty());
    keys[hook.VirtualKeyCode] = true; hook.state.PrepareOrder_Type = ordertype::STOP;
    assert(!hook.Dispatch(WM_LBUTTONDOWN, MK_LBUTTON, Point(200,120)));
    hook.state.PrepareOrder_Type = ordertype::BUILD;
    hook.RingWrite = true;
    assert(!hook.Dispatch(WM_LBUTTONDOWN, MK_LBUTTON, Point(200,120)));
    assert(hook.commits.empty() && !hook.WriteLine); // Existing ring owns its click.
    hook.RingWrite = false;
    assert(!hook.Dispatch(WM_LBUTTONDOWN, MK_LBUTTON, Point(50,120)));
    assert(!hook.WriteLine && hook.commits.empty()); // Menu/invalid start.
}

static void TestCancellationBothViews() {
    for (bool overview : {false,true}) {
        for (int reason = 0; reason < 9; ++reason) {
            CTAHook hook; Begin(hook, overview);
            switch (reason) {
            case 0:
                keys[hook.VirtualKeyCode] = false;
                hook.Dispatch(WM_KEYUP, hook.VirtualKeyCode); break;
            case 1:
                hook.Dispatch(WM_KILLFOCUS); break;
            case 2:
                // Model a missed key-up: async state is authoritative too.
                keys[hook.VirtualKeyCode] = false;
                hook.Dispatch(WM_MOUSEMOVE, MK_LBUTTON, Point(250,180)); break;
            case 3:
                hook.state.PrepareOrder_Type = ordertype::STOP;
                hook.Dispatch(WM_MOUSEMOVE, MK_LBUTTON, Point(250,180)); break;
            case 4:
                hook.Dispatch(WM_CANCELMODE); break;
            case 5:
                hook.Dispatch(WM_ACTIVATEAPP, 0); break;
            case 6:
                // KEYUP itself must cancel even if the asynchronous snapshot
                // has not caught up yet (or the user re-pressed the modifier).
                hook.Dispatch(WM_KEYUP, hook.VirtualKeyCode); break;
            case 7:
                hook.Dispatch(WM_KEYDOWN, VK_ESCAPE); break;
            default:
                keys[VK_CONTROL] = true;
                hook.Dispatch(WM_KEYDOWN, VK_F2); break;
            }
            ExpectCleared(hook);
            assert(hook.RectangleMouseDown && !hook.RectangleCaptured && GetCapture() == nullptr);
            keys[VK_LBUTTON] = false;
            assert(hook.Dispatch(WM_LBUTTONUP, 0, Point(280,220)));
            assert(hook.commits.empty());
            assert(!hook.RectangleMouseDown);
        }
        CTAHook invalidRelease; Begin(invalidRelease, overview);
        keys[VK_LBUTTON] = false;
        assert(invalidRelease.Dispatch(WM_LBUTTONUP, 0, Point(50,120)));
        ExpectCleared(invalidRelease); assert(invalidRelease.commits.empty());

        CTAHook tooLarge; Begin(tooLarge, overview);
        tooLarge.rejectGeometry = true; keys[VK_LBUTTON] = false;
        assert(tooLarge.Dispatch(WM_LBUTTONUP, 0, Point(280,220)));
        ExpectCleared(tooLarge); assert(tooLarge.commits.empty() && tooLarge.warnings == 1);
    }
}

static void TestSettingsAndCaptureFailure() {
    for (bool overview : {false,true}) {
        CTAHook settings; Begin(settings, overview);
        assert(!settings.Dispatch(WM_KEYDOWN, VK_F2));
        assert(settings.WriteLine && settings.RectangleCaptured); // F2 alone is unrelated.
        keys[VK_CONTROL] = true;
        assert(!settings.Dispatch(WM_KEYDOWN, VK_F2)); // Forward the settings shortcut after cancel.
        ExpectCleared(settings);
        assert(settings.RectangleMouseDown && GetCapture() == nullptr);
        keys[VK_LBUTTON] = false;
        assert(settings.Dispatch(WM_LBUTTONUP, 0, Point(280,220)));
        assert(settings.commits.empty() && !settings.RectangleMouseDown);

        CTAHook failure;
        control.overview = overview;
        keys[failure.VirtualKeyCode] = keys[VK_LBUTTON] = true;
        failCapture = true; capturedWindow = reinterpret_cast<HWND>(2);
        assert(failure.Dispatch(WM_LBUTTONDOWN, MK_LBUTTON, Point(200,120)));
        ExpectCleared(failure);
        assert(failure.RectangleMouseDown && !failure.RectangleCaptured);
        assert(captureSets == 1 && captureReleases == 0 && GetCapture() == reinterpret_cast<HWND>(2));
        assert(TAhWnd == reinterpret_cast<HWND>(1));
        keys[VK_LBUTTON] = false;
        assert(failure.Dispatch(WM_LBUTTONUP, 0, Point(280,220)));
        assert(failure.commits.empty() && !failure.RectangleMouseDown);
        assert(GetCapture() == reinterpret_cast<HWND>(2));
        // A failed capture must not poison the next drag after capture works.
        failCapture = false; keys[VK_LBUTTON] = true;
        assert(failure.Dispatch(WM_LBUTTONDOWN, MK_LBUTTON, Point(200,120)));
        assert(failure.WriteLine && failure.RectangleCaptured);
        keys[VK_LBUTTON] = false;
        assert(failure.Dispatch(WM_LBUTTONUP, 0, Point(280,220)));
        assert(failure.commits.size() == 1);
        ExpectCleared(failure);
    }
}

static void TestWheelUpdatesOnlyPreview() {
    for (bool overview : {false,true}) {
        CTAHook hook; Begin(hook, overview);
        const int initialCalculations = hook.calculations;
        assert(hook.Dispatch(WM_MOUSEWHEEL, std::uintptr_t(120) << 16, Point(220,140)));
        assert(hook.Spacing == 1 && hook.calculations == initialCalculations + 1);
        assert(hook.Dispatch(WM_MOUSEWHEEL, std::uintptr_t(static_cast<unsigned short>(-120)) << 16, Point(220,140)));
        assert(hook.Spacing == 0 && hook.calculations == initialCalculations + 2);
        assert(hook.commits.empty() && hook.WriteLine);
        keys[VK_LBUTTON] = false;
        assert(hook.Dispatch(WM_LBUTTONUP, 0, Point(280,220)));
        assert(hook.commits.size() == 1 && hook.commits[0].spacing == 0);
    }
}

static void TestReentrantCaptureAndDoubleClick() {
    CTAHook hook; Begin(hook, true);
    int reentrant = 0;
    onCaptureRelease = [&]() {
        ++reentrant;
        hook.Dispatch(WM_CAPTURECHANGED);
    };
    keys[VK_LBUTTON] = false;
    assert(hook.Dispatch(WM_LBUTTONUP, 0, Point(280,220)));
    assert(reentrant == 1 && captureReleases == 1 && hook.commits.size() == 1);
    ExpectCleared(hook);
    keys[VK_LBUTTON] = true;
    assert(hook.Dispatch(WM_LBUTTONDBLCLK, MK_LBUTTON, Point(200,120)));
    assert(hook.WriteLine && hook.commits.size() == 1); // New drag, never a second-click commit.
    keys[VK_LBUTTON] = false;
    assert(hook.Dispatch(WM_LBUTTONUP, 0, Point(230,150)));
    assert(hook.commits.size() == 2 && reentrant == 2);
    ExpectCleared(hook);

    CTAHook stolen; Begin(stolen, false);
    capturedWindow = reinterpret_cast<HWND>(2);
    stolen.Dispatch(WM_CAPTURECHANGED);
    ExpectCleared(stolen);
    assert(stolen.RectangleMouseDown && GetCapture() == reinterpret_cast<HWND>(2));
    assert(captureReleases == 0); // Never release another window's capture.
    keys[VK_LBUTTON] = false;
    assert(stolen.Dispatch(WM_LBUTTONUP, 0, Point(280,220)));
    assert(stolen.commits.empty());
}

static void TestDispatchFailureAndGameExitCleanup() {
    CTAHook failure; Begin(failure, false);
    failure.throwOnCommit = true; keys[VK_LBUTTON] = false;
    bool threw = false;
    try { failure.Dispatch(WM_LBUTTONUP, 0, Point(280,220)); }
    catch (const std::runtime_error&) { threw = true; }
    assert(threw && failure.commits.size() == 1);
    ExpectCleared(failure);
    assert(!failure.RectangleMouseDown && !failure.RectangleCaptured && GetCapture() == nullptr);
    assert(captureReleases == 1);

    CTAHook leaving; Begin(leaving, true);
    sharedData.TAProgress = 0;
    leaving.TAdynmem = nullptr; // No game-memory reads after leaving the game.
    keys[VK_LBUTTON] = false;
    assert(leaving.Dispatch(WM_LBUTTONUP, 0, Point(280,220)));
    ExpectCleared(leaving);
    assert(leaving.commits.empty() && !leaving.RectangleMouseDown && GetCapture() == nullptr);
}

int main() {
    (void)DataShare;
    TestDragReleaseBothViews();
    TestConfiguredKeyAndNativePassThrough();
    TestCancellationBothViews();
    TestSettingsAndCaptureFailure();
    TestWheelUpdatesOnlyPreview();
    TestReentrantCaptureAndDoubleClick();
    TestDispatchFailureAndGameExitCleanup();
    std::cout << "Rectangle gesture integration tests passed\n";
}
'''


def main():
    source = (DDRAW / "tahook.cpp").read_text()
    helper = re.search(r'bool CTAHook::RectangleGestureMessage\(([^)]*)\)', source)
    if not helper:
        raise RuntimeError("Expected the production RectangleGestureMessage helper")
    signature = helper.group()
    declaration = signature.replace("CTAHook::", "") + ";"
    dispatch = "if (RectangleGestureMessage(" + (
        "reinterpret_cast<HWND>(1), msg, wParam, point)) return true;" if "HWND" in helper.group(1)
        else "msg, wParam, point)) return true;"
    )
    wheel_start = source.index("case WM_MOUSEWHEEL:", source.index("bool CTAHook::Message("))
    wheel_end = source.index("\n\t\t\t\tbreak;", wheel_start) + len("\n\t\t\t\tbreak;")
    wheel = "bool CTAHook::SpacingMessage(UINT Msg, WPARAM wParam, LPARAM lParam) { switch (Msg) {" + source[wheel_start:wheel_end] + "} return false; }"
    methods = "\n\n".join((
        extract_method(source, "bool CTAHook::UpdateRectangleMouse(LPARAM lParam)"),
        normalize_finally(extract_method(source, "void CTAHook::EndRectangleGesture(")),
        normalize_finally(extract_method(source, signature)),
        extract_method(source, "void CTAHook::UpdateSpacing()"),
        wheel,
    ))
    harness = HARNESS.replace("// GESTURE_DECLARATION", declaration).replace(
        "// GESTURE_DISPATCH", dispatch).replace("// PRODUCTION_METHODS", methods)
    with tempfile.TemporaryDirectory(prefix="rectangle-gesture-test-") as temp:
        cpp = Path(temp) / "rectangle_gesture_test.cpp"
        binary = Path(temp) / "rectangle_gesture_test"
        cpp.write_text(harness)
        subprocess.run(shlex.split(os.environ.get("CXX", "g++")) + [
            "-std=c++11", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter", "-g",
            "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-I", str(DDRAW),
            str(cpp), "-o", str(binary),
        ], check=True)
        env = os.environ.copy()
        env["ASAN_OPTIONS"] = env.get("ASAN_OPTIONS", "") + ":detect_leaks=0"
        subprocess.run([str(binary)], check=True, env=env)


if __name__ == "__main__":
    main()
