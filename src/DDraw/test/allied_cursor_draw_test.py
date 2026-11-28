#!/usr/bin/env python3
"""Compile production cursor capture/rendering with passive platform mocks."""

from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile

DDRAW = Path(__file__).resolve().parent.parent
source = re.sub(r'^#include "[^"\n]+"\s*$', '', (DDRAW / 'AlliedCursorDraw.cpp').read_text(), flags=re.M)

STUBS = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include "AlliedCursors.h"
#include "AlliedCursorGeometry.h"
class Label; // Existing UI class declared by the real dialog headers.
using HWND = void*;
struct POINT { int x, y; };
struct RECT { int left, top, right, bottom; };
struct _Position_Dword { unsigned short x_, X, z_, Z, y_, Y; };
using Position_Dword = _Position_Dword;
struct Panel { int xpos = 0, ypos = 0, width = 0, height = 0; };
struct GUI { GUI* per_active = nullptr; Panel* ControlsAry = nullptr; };
struct TAdynmemStruct {
    struct Desktop { GUI* TheActive_GUIMEM = nullptr; } desktopGUI;
    RECT GameSreen_Rect = {8,8,55,39};
    int FeatureMapSizeX = 64, FeatureMapSizeY = 64, MapSizeX = 1024, MapSizeY = 896;
    int EyeBallMapXPos = 0, EyeBallMapYPos = 0, SeaLevel = 7;
    void* COMIXFontHandle = nullptr; void* smalFontHandle = nullptr;
};
struct Program { HWND TAClass_Hwnd = reinterpret_cast<void*>(1); } program;
Program* programPointer = &program; Program** TAProgramStruct_PtrPtr = &programPointer;
constexpr int TAInGame = 3;
struct Shared { int TAProgress = TAInGame; int PlayerDotColors[10] = {}; } shared;
Shared* DataShare = &shared;
RECT dialogRect = {}; bool dialogShown = false;
struct Dialog { bool IsShow(RECT* rect) { *rect = dialogRect; return dialogShown; } } dialog;
RECT incomeRect = {};
struct CIncome {
    bool IsPointOver(int x,int y) { return x>=incomeRect.left && x<incomeRect.right && y>=incomeRect.top && y<incomeRect.bottom; }
} income;
struct Local { void* Dialog = &dialog; void* Income = &income; } local;
Local* LocalShare = &local;
bool foreground = true, iconic = false, visible = true, pointerValid = true;
POINT pointer = {24,24}; int terrainReads = 0, terrainZ = 0;
bool IsWindowVisible(HWND) { return visible; }
bool IsIconic(HWND) { return iconic; }
HWND GetForegroundWindow() { return foreground ? program.TAClass_Hwnd : nullptr; }
bool GetCursorPos(POINT* p) { *p = pointer; return pointerValid; }
bool ScreenToClient(HWND,POINT*) { return true; }
int GetPosition_Dword(int x,int y,Position_Dword* p) {
    ++terrainReads; *p = {}; p->X = x; p->Y = y; p->Z = terrainZ; return 1;
}
struct PALETTEENTRY { unsigned char peRed, peGreen, peBlue, peFlags; };
PALETTEENTRY TAPalette[256] = {};
char ThinFont[2048] = {};
bool megaVisible = false; int megaReads = 0;
struct MegaMapControl {
    bool IsBliting() { return megaVisible; }
    bool CursorGroundPoint(Position_Dword* p,int x,int y) {
        ++megaReads; *p = {}; p->X = x * 2; p->Y = y * 2; return x>=8 && x<56 && y>=8 && y<40;
    }
    bool CursorScreenPoint(POINT* p,RECT* clip,const Position_Dword& pos) {
        p->x = pos.X / 2; p->y = pos.Y / 2; *clip = {8,8,55,39}; return true;
    }
} control;
struct Minimap { MegaMapControl* Controler = &control; } minimap;
struct Expander { Minimap* myMinimap = &minimap; } expander;
Expander* GUIExpander = &expander;
'''

TESTS = r'''
int main() {
    TAdynmemStruct ta;
    Position_Dword position = {};
    std::array<AlliedCursorView,10> views = {};
    constexpr int width=64,height=48,pitch=80;
    std::vector<char> pixels(height*pitch,64);
#if !ALLIED_CURSORS_ENABLE
    assert(!AlliedCursorDraw::Sample(&ta,position));
    AlliedCursorDraw::Draw(pixels.data(),width,height,pitch,ta,views);
    for (char c:pixels) assert(c==64);
    assert(terrainReads==0 && megaReads==0);
#else
    for (int i=0;i<256;++i) TAPalette[i]={static_cast<unsigned char>(i),static_cast<unsigned char>(i),static_cast<unsigned char>(i),0};
    shared.PlayerDotColors[0]=224;
    assert(AlliedCursorDraw::Sample(&ta,position)); assert(position.X==16 && position.Y==16);
    ta.EyeBallMapXPos=100; ta.EyeBallMapYPos=80;
    assert(AlliedCursorDraw::Sample(&ta,position)); assert(position.X==116 && position.Y==96);
    ta.EyeBallMapXPos=0;ta.EyeBallMapYPos=0;
    terrainZ=-1; assert(AlliedCursorDraw::Sample(&ta,position)); assert(position.Z==ta.SeaLevel); terrainZ=0;
    foreground=false; assert(!AlliedCursorDraw::Sample(&ta,position)); foreground=true;
    iconic=true; assert(!AlliedCursorDraw::Sample(&ta,position)); iconic=false;
    visible=false; assert(!AlliedCursorDraw::Sample(&ta,position)); visible=true;
    pointerValid=false; assert(!AlliedCursorDraw::Sample(&ta,position)); pointerValid=true;
    shared.TAProgress=1; assert(!AlliedCursorDraw::Sample(&ta,position)); shared.TAProgress=TAInGame;
    pointer={2,24}; assert(!AlliedCursorDraw::Sample(&ta,position)); pointer={24,24};
    dialogRect={20,20,40,40}; dialogShown=true;
    assert(!AlliedCursorDraw::Sample(&ta,position)); dialogShown=false;
    incomeRect={20,20,40,40}; assert(!AlliedCursorDraw::Sample(&ta,position)); incomeRect={};
    Panel panel;panel.xpos=20;panel.ypos=20;panel.width=20;panel.height=20;
    GUI gui;gui.per_active=&gui;gui.ControlsAry=&panel;ta.desktopGUI.TheActive_GUIMEM=&gui;
    assert(!AlliedCursorDraw::Sample(&ta,position));ta.desktopGUI.TheActive_GUIMEM=nullptr;
    ta.EyeBallMapXPos=-40;assert(!AlliedCursorDraw::Sample(&ta,position));ta.EyeBallMapXPos=0;
    views[1].visible=true;views[1].x=16;views[1].y=16;views[1].color=0;
    const int readsBefore=terrainReads;
    AlliedCursorDraw::Draw(pixels.data(),width,height,pitch,ta,views);
    assert(terrainReads==readsBefore);assert(static_cast<unsigned char>(pixels[24*pitch+24])==224);
    for(int y=0;y<height;++y)for(int x=0;x<pitch;++x)
        if(x<8||x>=56||y<8||y>=40)assert(pixels[y*pitch+x]==64);
    // Cached blending must follow a changed/reloaded palette.
    const auto firstBlend=Blends(224); TAPalette[224]={0,0,0,0};
    const auto secondBlend=Blends(224);assert(firstBlend[8][64]!=secondBlend[8][64]);
    TAPalette[224]={224,224,224,0};
#if USEMEGAMAP
    megaVisible=true;assert(AlliedCursorDraw::Sample(&ta,position));assert(position.X==48&&position.Y==48&&megaReads==1);
    std::fill(pixels.begin(),pixels.end(),64);views[1].x=48;views[1].y=48;
    AlliedCursorDraw::Draw(pixels.data(),width,height,pitch,ta,views);
    assert(static_cast<unsigned char>(pixels[24*pitch+24])==224);
    // The strategic marker is smaller than its battlefield counterpart.
    assert(pixels[24*pitch+30]==64);megaVisible=false;
#endif
#endif
    std::cout << "Production passive cursor sampling/rendering passed\n";
}
'''

with tempfile.TemporaryDirectory(prefix='allied-cursor-draw-') as directory:
    directory=Path(directory)
    cpp=directory/'draw_test.cpp'
    cpp.write_text(STUBS + '\n' + source + '\n' + TESTS)
    for enabled,mega in [(0,0),(0,1),(1,0),(1,1)]:
        output=directory/f'draw-{enabled}-{mega}'
        command=shlex.split(os.environ.get('CXX','g++'))+[
            '-std=c++14','-Wall','-Wextra','-Werror','-pedantic',
            '-fsanitize=address,undefined','-fno-omit-frame-pointer','-g',
            f'-DALLIED_CURSORS_ENABLE={enabled}',f'-DUSEMEGAMAP={mega}',
            '-I',str(DDRAW),str(cpp),'-o',str(output)]
        subprocess.run(command,check=True)
        environment=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
        subprocess.run([str(output)],check=True,env=environment)
print('Verified cursor flag and megamap combinations: 00,01,10,11')
