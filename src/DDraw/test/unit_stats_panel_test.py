#!/usr/bin/env python3
"""Exercise the production native status-bar controller with platform bridges mocked.

The whole production implementation is compiled. Only includes, fixed native
addresses and MSVC SEH syntax are adapted for a Linux host. The SEH finally
body is installed as a scope guard so early returns still restore font/clip.
These checks do not establish retail addresses, MSVC ABI or actual SEH behavior.
"""

import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parent.parent


def block(text, opening):
    """Return a C++ brace body, ignoring braces in comments and strings."""
    tokens = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', re.S)
    depth = 0
    for token in tokens.finditer(text, opening):
        if token.group() == '{':
            depth += 1
        elif token.group() == '}':
            depth -= 1
            if not depth:
                return text[opening + 1:token.start()], token.end()
    raise AssertionError('Unclosed production brace body')


def adapted_source():
    text = (ROOT / 'UnitStatsPanel.cpp').read_text()
    text = '\n'.join(line for line in text.splitlines() if not line.startswith('#include'))
    native = 'NativeDraw const DrawNative = reinterpret_cast<NativeDraw>(kInfoPanel);'
    assert text.count(native) == 1
    text = text.replace(native, 'NativeDraw const DrawNative = &FakeNativeDraw;')
    # Preserve the actual production cleanup statements, including return paths.
    for signature, following in (
            ('void DrawNativeSelected(', 'struct FontState'),
            ('bool PaintRestored(', 'bool PaintSafe(')):
        start = text.index(signature)
        end_signature = text.index(following, start)
        method = text[start:end_signature]
        try_at = method.index('__try')
        try_open = method.index('{', try_at)
        try_body, try_end = block(method, try_open)
        finally_at = method.index('__finally', try_end)
        finally_open = method.index('{', finally_at)
        finally_body, finally_end = block(method, finally_open)
        adapted = method[:try_at] + 'auto restoration = OnExit([&]() {' + finally_body + '});\n'
        adapted += '{' + try_body + '}' + method[finally_end:]
        text = text[:start] + adapted + text[end_signature:]
    text = text.replace('__try', 'try')
    text = text.replace('__except (EXCEPTION_EXECUTE_HANDLER)', 'catch (...)')
    assert '__try' not in text and '__finally' not in text and '__except' not in text
    return text


HARNESS = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <strings.h>
#include <utility>
#include <vector>
#include "UnitStatsCore.h"
#include "UnitStatsPanel.h"

#define __stdcall
using BOOL=int; using SIZE_T=std::size_t;
constexpr BOOL TRUE=1, FALSE=0;
constexpr int TAInGame=6, Player_LocalHuman=1;
constexpr unsigned WATCH=0x40, UnitSelected_State=0x10;
constexpr int INLINE_UNPROTECTEVINMENT=0x1003;
// LONG differs from int on Windows: use host long to catch mixed std::min/max
// templates even though this harness does not reproduce the native struct ABI.
struct RECT { long left=0,top=0,right=0,bottom=0; };
struct POINT { int x=0,y=0; };
struct _OFFSCREEN {
    int Width=0,Height=0,lPitch=0; void* lpSurface=nullptr; RECT ScreenRect;
};
using OFFSCREEN=_OFFSCREEN;
struct UnitDefStruct { char UnitName[32]={}; };
struct PlayerInfo { unsigned PropertyMask=0; char RaceSide=0; };
struct PlayerStruct;
struct UnitStruct {
    PlayerStruct* Owner_PlayerPtr0=nullptr; UnitDefStruct* UnitType=nullptr;
    unsigned UnitSelected=0; short UnitID=0; unsigned health=100;
};
struct PlayerStruct {
    ::PlayerInfo* PlayerInfo=nullptr; int My_PlayerType=Player_LocalHuman;
    UnitStruct* Units=nullptr; UnitStruct* UnitsAry_End=nullptr;
};
struct GUI0IDControl {
    int xpos=0,ypos=0,width=0,height=0,totalgadgets=0,id=0,active=0;
    char name[16]={};
};
struct GUIMEMSTRUCT { GUI0IDControl* ControlsAry=nullptr; };
struct Desktop { int field_68=-1; GUIMEMSTRUCT* TheActive_GUIMEM=nullptr; };
struct RaceSide { void* Font_File=nullptr; };
struct GAFSequence;
struct TAdynmemStruct {
    UnitDefStruct* UnitDef=nullptr; unsigned UNITINFOCount=0;
    int LocalHumanPlayer_PlayerID=0; PlayerStruct Players[10];
    UnitStruct* BeginUnitsArray_p=nullptr; UnitStruct* EndOfUnitsArray_p=nullptr;
    unsigned GameStateMask=0; Desktop desktopGUI; POINT CurtMousePostion;
    unsigned short MouseOverUnit=0,MouseOverFeature=0xffff;
    RaceSide RaceSideDataAry[5]; unsigned ScreenWidth=0,ScreenHeight=0;
    unsigned short InfoPanelCachedUnit=0;
    GAFSequence* InfoPanelBackgrounds[5]={};
};
struct TAProgramStruct {
    unsigned char* fontHandle=nullptr;
    int fontFrontColour=0,fontBackColour=0,fontAlpha=0;
};
struct Shared { int TAProgress=TAInGame; int ehaOff=0; } shared;
Shared* DataShare=&shared;
bool dialogShown=false;
struct Dialog { bool IsShow(RECT*) { return dialogShown; } } dialog;
struct LocalShared { void* Dialog=nullptr; } localShared;
LocalShared* LocalShare=&localShared;
struct Config { bool enabled=true; BOOL GetIniBool(const char* name,BOOL) {
    assert(std::strcmp(name,"UnitStatsPanel")==0); return enabled; } } config;
Config* MyConfig=&config;
struct GAFFrame { unsigned short Width=64,Height=32; short xPosition=0,yPosition=0; } frame;
struct GAFSequence { unsigned Frames=1; struct Ref { GAFFrame* PtrFrame=nullptr; } PtrFrameAry[1]; } sequence;
TAdynmemStruct world;
TAdynmemStruct* currentWorld=&world;
TAdynmemStruct** TAmainStruct_PtrPtr=&currentWorld;
TAProgramStruct program;
TAProgramStruct* currentProgram=&program;
TAProgramStruct** TAProgramStruct_PtrPtr=&currentProgram;
UnitDefStruct definitions[4]; UnitStruct units[6]; PlayerInfo playerInfo;
GUI0IDControl controls[5]; GUIMEMSTRUCT gui;
unsigned char sideFont[32]={}, oldFont[32]={};
OFFSCREEN surface;
std::vector<unsigned char> pixels;
constexpr std::size_t Guard=48;
constexpr unsigned char Clean=0xa5, NativeColor=31, BackgroundColor=42, GlyphColor=53;
struct Text { std::string value; int x,y; RECT clip; unsigned char* font; };
std::vector<Text> texts;
unsigned nativeCalls=0,dataCalls=0,backgroundCalls=0,hookCreated=0,hookDestroyed=0;
unsigned lastType=0,lastCount=0; const UnitStruct* lastSingle=nullptr;
TAdynmemStruct* lastWorld=nullptr;
bool dataFail=false,dataThrow=false,paintThrow=false,nativeThrow=false;
bool readFail=false,patchFail=false,constructorThrow=false;
unsigned readCalls=0,failReadNumber=0;
std::vector<unsigned short> nativeCacheValues;
std::vector<unsigned char> nativePixels;
int nativeExtent=310,lastNativeExtent=310;
unsigned short nativeObservedUnit=0;
int nativeObservedButton=-1;
bool nativeHonorsCache=false,texturedBackground=false;
std::array<UnitStatsCore::Stats,4> templates;
const std::array<unsigned char,5> Original={{0xE8,0x46,0x12,0,0}};
std::array<unsigned char,5> nativeCode=Original;

int _stricmp(const char* a,const char* b) { return strcasecmp(a,b); }
int _strnicmp(const char* a,const char* b,std::size_t n) { return strncasecmp(a,b,n); }
template<std::size_t N,class... Args>
int sprintf_s(char (&buffer)[N],const char* format,Args... args) {
    return std::snprintf(buffer,N,format,args...);
}
template<class Fn> struct ScopeExit {
    Fn fn; bool active;
    explicit ScopeExit(Fn f):fn(std::move(f)),active(true){}
    ScopeExit(ScopeExit&& other):fn(std::move(other.fn)),active(other.active){other.active=false;}
    ~ScopeExit(){if(active)fn();}
};
template<class Fn> ScopeExit<Fn> OnExit(Fn fn){return ScopeExit<Fn>(std::move(fn));}
unsigned short Cache(TAdynmemStruct* ta) {
    return ta->InfoPanelCachedUnit;
}
void SetCache(TAdynmemStruct* ta,unsigned short value) {
    ta->InfoPanelCachedUnit=value;
}
bool SameRect(RECT a,RECT b){return a.left==b.left&&a.top==b.top&&a.right==b.right&&a.bottom==b.bottom;}
int GetTextExtent(void*,const char* text) {
    int width=0; for(const unsigned char* p=reinterpret_cast<const unsigned char*>(text);*p;++p)
        width+=*p=='W'?9:(*p=='i'||*p=='l'?3:6);
    return width;
}
void Fill(OFFSCREEN* off,RECT rectangle,unsigned char color) {
    assert(off&&off->lpSurface);
    int left=static_cast<int>(std::max<long>(0,std::max(rectangle.left,off->ScreenRect.left)));
    int right=static_cast<int>(std::min<long>(off->Width-1,std::min(rectangle.right,off->ScreenRect.right)));
    int top=static_cast<int>(std::max<long>(0,std::max(rectangle.top,off->ScreenRect.top)));
    int bottom=static_cast<int>(std::min<long>(off->Height-1,std::min(rectangle.bottom,off->ScreenRect.bottom)));
    for(int y=top;y<=bottom;++y) for(int x=left;x<=right;++x)
        static_cast<unsigned char*>(off->lpSurface)[static_cast<std::size_t>(y)*off->lPitch+x]=color;
}
void PaintBackground(OFFSCREEN* off,RECT rectangle,int originX,int originY) {
    if(!texturedBackground){Fill(off,rectangle,BackgroundColor);return;}
    for(int y=static_cast<int>(rectangle.top);y<=rectangle.bottom;++y)
        for(int x=static_cast<int>(rectangle.left);x<=rectangle.right;++x){
            const auto colour=static_cast<unsigned char>(100+(((x-originX)%frame.Width)*3+(y-originY)*5)%20);
            Fill(off,{x,y,x,y},colour);
        }
}
int CopyGafToContext(OFFSCREEN* off,GAFFrame* image,int x,int y) {
    ++backgroundCalls;
    const int left=x-image->xPosition,top=y-image->yPosition;
    PaintBackground(off,{left,top,left+image->Width-1,top+image->Height-1},left,top);return 1;
}
int DrawTextInScreen(OFFSCREEN* off,char* text,int x,int y,int) {
    texts.push_back({text,x,y,off->ScreenRect,program.fontHandle});
    const int width=GetTextExtent(program.fontHandle,text);
    assert(x>=off->ScreenRect.left&&x+width<=off->ScreenRect.right);
    assert(y>=off->ScreenRect.top&&y+program.fontHandle[0]-1<=off->ScreenRect.bottom);
    Fill(off,{x,y,x+width-1,y+program.fontHandle[0]-1},GlyphColor);
    if(paintThrow) throw std::runtime_error("fake font fault");
    return width;
}
void FakeNativeDraw(OFFSCREEN* off) {
    ++nativeCalls;
    if(currentWorld){
        nativeObservedUnit=currentWorld->MouseOverUnit;
        nativeObservedButton=currentWorld->desktopGUI.field_68;
        nativeCacheValues.push_back(Cache(currentWorld));
    }
    if(nativeThrow) throw std::runtime_error("native exception must propagate");
    if(nativeHonorsCache&&currentWorld&&Cache(currentWorld)==123)return;
    if(currentWorld)SetCache(currentWorld,123);
    PaintBackground(off,{130,off->Height-32,off->Width-1,off->Height-1},129,off->Height-32);
    // Native player colour, name/health, mission, metal/energy and target bars.
    // They use distinct pixels so retaining only the title cannot pass this test.
    Fill(off,{130,off->Height-29,145,off->Height-5},NativeColor);
    Fill(off,{155,off->Height-27,210,off->Height-20},32);
    Fill(off,{155,off->Height-16,210,off->Height-12},33);
    Fill(off,{215,off->Height-28,230,off->Height-18},34);
    Fill(off,{215,off->Height-12,240,off->Height-3},35);
    Fill(off,{245,off->Height-20,nativeExtent,off->Height-18},36);
    // Keep the right edge authoritative even with a compact custom native UI.
    Fill(off,{nativeExtent,off->Height-4,nativeExtent,off->Height-4},37);
    lastNativeExtent=nativeExtent;
    const auto* start=static_cast<const unsigned char*>(off->lpSurface);
    nativePixels.assign(start,start+static_cast<std::size_t>(off->Height)*off->lPitch);
}
void* GetCurrentProcess(){return nullptr;}
BOOL ReadProcessMemory(void*,const void* address,void* output,SIZE_T size,SIZE_T* read) {
    ++readCalls;*read=0;
    if(readFail||(failReadNumber&&readCalls==failReadNumber))return FALSE;
    assert(reinterpret_cast<std::uintptr_t>(address)==0x00469615u&&size==5);
    std::memcpy(output,nativeCode.data(),size);*read=size;return TRUE;
}
struct SingleHook {
    std::array<unsigned char,5> saved;
    SingleHook(unsigned address,unsigned length,int mode,unsigned char* replacement):saved(nativeCode){
        assert(address==0x00469615u&&length==5&&mode==INLINE_UNPROTECTEVINMENT);
        ++hookCreated;
        if(constructorThrow)throw std::bad_alloc();
        std::memcpy(nativeCode.data(),replacement,5);
        if(patchFail)nativeCode[2]^=1;
    }
    ~SingleHook(){++hookDestroyed;nativeCode=saved;}
};
namespace IDDrawSurface {void OutptTxt(const char*){}}
namespace UnitStatsData {
bool Read(TAdynmemStruct* ta,unsigned type,const UnitStruct* single,unsigned count,UnitStatsCore::Stats& out) {
    ++dataCalls;lastType=type;lastSingle=single;lastCount=count;lastWorld=ta;
    if(dataThrow)throw std::runtime_error("backend fault");
    if(dataFail)return false;
    assert(ta==currentWorld&&type<4);
    out=templates[type];out.count=count;out.liveHealth=single!=nullptr;
    if(single)out.currentHealth=single->health;
    return true;
}}

// PRODUCTION_SOURCE

void ClearFrame(){
    nativeCalls=dataCalls=backgroundCalls=0;texts.clear();nativeCacheValues.clear();nativePixels.clear();
    std::fill(pixels.begin(),pixels.end(),Clean);
}
void SetSurface(int width,int height=480,int pitch=0){
    if(!pitch)pitch=width+20;
    surface.Width=width;surface.Height=height;surface.lPitch=pitch;
    surface.ScreenRect={0,0,width-1,height-1};
    pixels.assign(Guard+static_cast<std::size_t>(height)*pitch+Guard,Clean);
    surface.lpSurface=pixels.data()+Guard;
    if(currentWorld){currentWorld->ScreenWidth=width;currentWorld->ScreenHeight=height;}
}
void Fixture(){
    UnitStatsPanel::Shutdown();currentWorld=&world;world=TAdynmemStruct();
    playerInfo=PlayerInfo();program=TAProgramStruct();currentProgram=&program;
    for(auto& unit:units)unit=UnitStruct();
    for(auto& control:controls)control=GUI0IDControl();
    definitions[0]=UnitDefStruct();definitions[1]=UnitDefStruct();
    definitions[2]=UnitDefStruct();definitions[3]=UnitDefStruct();
    std::strcpy(definitions[0].UnitName,"ARMCOM");std::strcpy(definitions[1].UnitName,"ARMFLASH");
    std::strcpy(definitions[2].UnitName,"ARMSOLAR");std::strcpy(definitions[3].UnitName,"ARMCON");
    world.UnitDef=definitions;world.UNITINFOCount=4;
    world.Players[0].PlayerInfo=&playerInfo;world.Players[0].Units=units;world.Players[0].UnitsAry_End=units+5;
    world.BeginUnitsArray_p=units;world.EndOfUnitsArray_p=units+5;
    for(auto& unit:units){unit.Owner_PlayerPtr0=&world.Players[0];unit.UnitID=1;unit.UnitType=&definitions[1];}
    controls[0].width=128;controls[0].height=400;controls[0].totalgadgets=4;
    controls[1].id=1;controls[1].active=1;controls[1].xpos=10;controls[1].ypos=20;
    controls[1].width=40;controls[1].height=40;std::strcpy(controls[1].name,"ARMSOLAR");
    gui.ControlsAry=controls;world.desktopGUI.TheActive_GUIMEM=&gui;
    sideFont[0]=8;oldFont[0]=10;world.RaceSideDataAry[0].Font_File=sideFont;
    program.fontHandle=oldFont;program.fontFrontColour=7;program.fontBackColour=8;program.fontAlpha=9;
    sequence.Frames=1;sequence.PtrFrameAry[0].PtrFrame=&frame;
    GAFSequence* seq=&sequence;
    world.InfoPanelBackgrounds[0]=seq;
    templates={};templates[1].name="Flash";templates[1].maxHealth=1000;
    templates[1].metalCost=100;templates[1].energyCost=1000;templates[1].los=300;
    templates[1].speed=30;templates[1].weapons[0].damage=100;templates[1].weapons[0].reloadTicks=30;
    templates[1].weapons[0].range=250;templates[2].name="Solar";templates[2].maxHealth=1200;
    templates[2].metalCost=150;templates[2].energyCost=100;templates[2].energyMake=20;
    config.enabled=true;MyConfig=&config;DataShare=&shared;shared.TAProgress=TAInGame;shared.ehaOff=0;
    LocalShare=&localShared;localShared.Dialog=&dialog;dialogShown=false;
    dataFail=dataThrow=paintThrow=nativeThrow=readFail=patchFail=constructorThrow=false;
    readCalls=failReadNumber=hookCreated=hookDestroyed=0;nativeCode=Original;
    nativeExtent=310;nativeHonorsCache=texturedBackground=false;nativeObservedUnit=0;nativeObservedButton=-1;
    SetSurface(1200);SetCache(&world,123);ClearFrame();
}
void Select(unsigned slot,unsigned type=1){
    units[slot].UnitSelected=UnitSelected_State|0x10000000u;
    units[slot].UnitID=static_cast<short>(type);units[slot].UnitType=&definitions[type];
}
void Hover(bool value=true){world.desktopGUI.field_68=value?1:-1;world.CurtMousePostion={20,30};}
bool TextContains(const std::string& part){for(const auto& t:texts)if(t.value.find(part)!=std::string::npos)return true;return false;}
void CheckGuards(int visibleWidth){
    for(std::size_t i=0;i<Guard;++i)assert(pixels[i]==Clean&&pixels[pixels.size()-1-i]==Clean);
    for(int y=0;y<surface.Height;++y)for(int x=visibleWidth;x<surface.lPitch;++x)
        assert(pixels[Guard+static_cast<std::size_t>(y)*surface.lPitch+x]==Clean);
    for(int y=0;y<surface.Height-32;++y)for(int x=0;x<visibleWidth;++x)
        assert(pixels[Guard+static_cast<std::size_t>(y)*surface.lPitch+x]==Clean);
    for(int y=surface.Height-32;y<surface.Height;++y)for(int x=0;x<std::min(129,visibleWidth);++x)
        assert(pixels[Guard+static_cast<std::size_t>(y)*surface.lPitch+x]==Clean);
}
void CheckOnlyCurrentAppend(){
    assert(!nativePixels.empty());
    for(int y=surface.Height-32;y<surface.Height;++y)for(int x=130;x<surface.Width;++x){
        const std::size_t offset=static_cast<std::size_t>(y)*surface.lPitch+x;
        if(pixels[Guard+offset]==nativePixels[offset])continue;
        bool glyph=false;
        for(const auto& text:texts)
            if(x>=text.x&&x<text.x+GetTextExtent(text.font,text.value.c_str())
                &&y>=text.y&&y<text.y+text.font[0])glyph=true;
        assert(glyph);
    }
}
void CheckNativePixels(){
    assert(!nativePixels.empty());
    for(int y=surface.Height-32;y<surface.Height;++y)
        for(int x=130;x<=std::min(lastNativeExtent,surface.Width-1);++x)
            assert(pixels[Guard+static_cast<std::size_t>(y)*surface.lPitch+x]
                ==nativePixels[static_cast<std::size_t>(y)*surface.lPitch+x]);
    for(const auto& text:texts)assert(text.x>lastNativeExtent);
}
void CheckFont(){assert(program.fontHandle==oldFont&&program.fontFrontColour==7&&program.fontBackColour==8&&program.fontAlpha==9);}

int main(){
#if UNIT_STATS_PANEL_ENABLE
    (void)kInfoPanel;
    Fixture();UnitStatsPanel::Install();assert(g_hook&&g_enabled&&hookCreated==1);
    UnitStatsPanel::Install();assert(hookCreated==1);assert(nativeCode!=Original);
    Select(0);units[0].health=333;
    RECT originalClip={130,20,1199,479};surface.ScreenRect=originalClip;
    DrawPanel(&surface);
    assert(nativeCalls==1&&dataCalls==1&&lastType==1&&lastCount==1&&lastSingle==units);
    assert(!TextContains("Flash")&&TextContains("HP 333/1000")&&TextContains("Selected: 1"));
    assert(TextContains("DPS~ 100")&&TextContains("Speed 30.0 m/s")&&TextContains("LOS 300"));
    for(const auto& text:texts)assert(text.font==sideFont);
    assert(texts.front().value.find("Selected: 1")==0);CheckNativePixels();
    assert(SameRect(surface.ScreenRect,originalClip));CheckFont();CheckGuards(surface.Width);

    ClearFrame();Select(1);Select(2);DrawPanel(&surface);
    assert(nativeCalls==1&&lastCount==3&&!lastSingle);
    assert(TextContains("HP 1000 (each)")&&TextContains("Selected: 3")&&TextContains("DPS~ 100"));
    assert(!TextContains("DPS~ 300"));
    ClearFrame();Hover();DrawPanel(&surface);
    assert(nativeCalls==1&&lastType==2&&lastCount==1&&!lastSingle);
    assert(TextContains("HP 1200")&&TextContains("Selected: 3")&&!TextContains("Flash"));CheckNativePixels();
    ClearFrame();Hover(false);units[2].UnitSelected=0;DrawPanel(&surface);
    assert(lastCount==2&&TextContains("Selected: 2"));

    ClearFrame();Select(1,2);DrawPanel(&surface);
    assert(nativeCalls==1&&dataCalls==0&&TextContains("Selected: 2"));
    assert(nativeCacheValues[0]==0xffffu);CheckFont();
    ClearFrame();DrawPanel(&surface);
    assert(nativeCalls==1&&TextContains("Selected: 2")&&nativeCacheValues[0]==0xffffu);
    ClearFrame();for(auto& unit:units)unit.UnitSelected=0;DrawPanel(&surface);
    assert(nativeCalls==1&&dataCalls==0&&texts.empty()&&nativeCacheValues[0]==0xffffu);
    assert(pixels[Guard+static_cast<std::size_t>(surface.Height-1)*surface.lPitch+140]==BackgroundColor);

    // Hover alone must not invent a selected unit or increment the total count.
    ClearFrame();Hover();DrawPanel(&surface);
    assert(nativeCalls==1&&lastCount==1&&lastType==2&&!TextContains("Selected:"));
    Hover(false);Select(0);Select(1);Select(2);
    // Foreign, dead, pending-death and inconsistent slots never inflate totals.
    units[1].Owner_PlayerPtr0=&world.Players[1];units[2].UnitSelected|=0x4000;
    Select(3);units[3].UnitSelected&=~0x10000000u;
    Select(4);units[4].UnitType=&definitions[2];
    ClearFrame();DrawPanel(&surface);assert(lastCount==1&&TextContains("Selected: 1"));

    for(unsigned mask:{3u,0x20u,0x40u}){
        ClearFrame();world.GameStateMask=mask;DrawPanel(&surface);
        assert(nativeCalls==1&&dataCalls==0&&texts.empty());CheckFont();
    }
    world.GameStateMask=0;dialogShown=true;ClearFrame();DrawPanel(&surface);
    assert(nativeCalls==1&&dataCalls==0&&texts.empty());dialogShown=false;
    controls[0].width=640;ClearFrame();DrawPanel(&surface);
    assert(nativeCalls==1&&dataCalls==0&&texts.empty());controls[0].width=128;
    playerInfo.PropertyMask=WATCH;ClearFrame();DrawPanel(&surface);
    assert(nativeCalls==1&&dataCalls==0&&texts.empty());playerInfo.PropertyMask=0;
    world.Players[0].My_PlayerType=2;ClearFrame();DrawPanel(&surface);
    assert(nativeCalls==1&&dataCalls==0&&texts.empty());world.Players[0].My_PlayerType=Player_LocalHuman;

    // Original native drawing happens exactly once, including backend/paint faults.
    dataFail=true;ClearFrame();DrawPanel(&surface);
    assert(nativeCalls==1&&dataCalls==1&&nativeCacheValues[0]==0xffffu);CheckFont();
    dataFail=false;dataThrow=true;ClearFrame();DrawPanel(&surface);
    assert(nativeCalls==1&&dataCalls==1);CheckFont();dataThrow=false;
    paintThrow=true;ClearFrame();DrawPanel(&surface);
    assert(nativeCalls==1&&nativeCacheValues[0]==0xffffu);CheckFont();
    assert(SameRect(surface.ScreenRect,originalClip));paintThrow=false;
    sequence.Frames=0;ClearFrame();DrawPanel(&surface);assert(nativeCalls==1);CheckFont();sequence.Frames=1;

    // Invalid hover indexes, disabled buttons, non-unit controls and stale mouse
    // positions fall back to the selected type rather than a previous hover.
    for(int index:{-1,0,5,257}){
        world.desktopGUI.field_68=index;ClearFrame();DrawPanel(&surface);assert(lastType==1);
    }
    Hover();controls[1].active=0;ClearFrame();DrawPanel(&surface);assert(lastType==1);controls[1].active=1;
    controls[1].id=2;ClearFrame();DrawPanel(&surface);assert(lastType==1);controls[1].id=1;
    world.CurtMousePostion={500,300};ClearFrame();DrawPanel(&surface);assert(lastType==1);Hover(false);

    // Text uses pixel width (including variable-width glyphs), keeps its original
    // font/clip and never paints the sidebar, the playfield or pitch padding.
    SetSurface(320);nativeExtent=220;templates[1].name=std::string(300,'W');ClearFrame();DrawPanel(&surface);
    assert(TextContains("Selected: 1"));CheckFont();CheckGuards(320);
    for(const auto& text:texts)assert(GetTextExtent(text.font,text.value.c_str())<=text.clip.right-text.x);
    SetSurface(1200);nativeExtent=310;templates[1].name="Flash";
    sideFont[0]=15;ClearFrame();DrawPanel(&surface);assert(nativeCalls==1);CheckFont();CheckGuards(1200);
    sideFont[0]=16;ClearFrame();DrawPanel(&surface);assert(nativeCalls==1);CheckFont();sideFont[0]=8;

    // Megamap contexts can advertise pitch as Width and retain a battlefield
    // clip: the public adapter bounds a copy without editing the supplied one.
    SetSurface(400,480,460);surface.Width=460;surface.ScreenRect={128,32,399,447};
    const OFFSCREEN supplied=surface;ClearFrame();UnitStatsPanel::Draw(&surface);
    assert(surface.Width==supplied.Width&&SameRect(surface.ScreenRect,supplied.ScreenRect));
    CheckFont();CheckGuards(400);
    shared.TAProgress=0;ClearFrame();UnitStatsPanel::Draw(&surface);
    assert(dataCalls==0&&texts.empty());shared.TAProgress=TAInGame;
    currentWorld=nullptr;ClearFrame();UnitStatsPanel::Draw(&surface);assert(nativeCalls==0&&dataCalls==0);
    currentWorld=&world;ClearFrame();UnitStatsPanel::Draw(&surface);assert(dataCalls==1&&lastWorld==&world);
    // A new match may reuse the same slot/type/count. Free the previous world
    // before drawing the replacement: no previous Request pointer may survive.
    for(unsigned slot=1;slot<6;++slot)units[slot].UnitSelected=0;
    auto priorWorld=std::unique_ptr<TAdynmemStruct>(new TAdynmemStruct(world));
    currentWorld=priorWorld.get();
    for(auto& unit:units)unit.Owner_PlayerPtr0=&currentWorld->Players[0];
    ClearFrame();UnitStatsPanel::Draw(&surface);assert(dataCalls==1&&lastWorld==currentWorld);
    auto nextWorld=std::unique_ptr<TAdynmemStruct>(new TAdynmemStruct(*priorWorld));
    currentWorld=nextWorld.get();
    for(auto& unit:units)unit.Owner_PlayerPtr0=&currentWorld->Players[0];
    priorWorld.reset();templates[1].name="Next match Flash";
    ClearFrame();UnitStatsPanel::Draw(&surface);
    assert(dataCalls==1&&lastWorld==currentWorld&&TextContains("Selected: 1")&&!TextContains("Next match Flash"));
    currentWorld=&world;
    for(auto& unit:units)unit.Owner_PlayerPtr0=&world.Players[0];
    // Native exceptions must retain native behavior, not disappear inside SEH.
    for(auto& unit:units)unit.UnitSelected=0;
    nativeThrow=true;ClearFrame();bool caught=false;
    try{UnitStatsPanel::Draw(&surface);}catch(const std::runtime_error&){caught=true;}
    assert(caught&&nativeCalls==1);nativeThrow=false;
    UnitStatsPanel::Shutdown();assert(nativeCode==Original&&!g_enabled&&!g_hook);

    // Native selected-unit information is used when there is no real hover.
    // Only the renderer's input is borrowed, including groups of the same type;
    // unit records, selection flags and the real GUI hover are left untouched.
    Fixture();UnitStatsPanel::Install();Select(1);units[1].health=333;
    world.desktopGUI.field_68=3;
    const unsigned selection=units[1].UnitSelected;
    DrawPanel(&surface);
    assert(nativeCalls==1&&nativeObservedUnit==1&&nativeObservedButton==-1);
    assert(world.MouseOverUnit==0&&world.desktopGUI.field_68==3);
    assert(units[1].UnitSelected==selection&&TextContains("HP 333/1000"));
    CheckNativePixels();CheckOnlyCurrentAppend();
    // Compare with the matching textured GAF strip, including tile origin and
    // row phase, rather than mistaking normal background pixels for native ink.
    texturedBackground=true;ClearFrame();DrawPanel(&surface);
    assert(texts.front().x==320&&TextContains("DPS~ 100"));
    CheckNativePixels();CheckOnlyCurrentAppend();
    ClearFrame();Select(2);DrawPanel(&surface);
    assert(nativeObservedUnit==1&&lastCount==2&&!lastSingle&&TextContains("HP 1000 (each)"));
    assert(world.MouseOverUnit==0&&world.desktopGUI.field_68==3);CheckNativePixels();
    nativeThrow=true;ClearFrame();caught=false;
    try{DrawPanel(&surface);}catch(const std::runtime_error&){caught=true;}
    assert(caught&&nativeObservedUnit==1&&world.MouseOverUnit==0&&world.desktopGUI.field_68==3);
    nativeThrow=false;

    // A different real unit or a map feature keeps its original native details.
    // The selected count still appears, without associating the group's stats
    // with an unrelated hovered object.
    units[2].UnitSelected=0;units[2].UnitID=2;units[2].UnitType=&definitions[2];
    world.MouseOverUnit=2;ClearFrame();DrawPanel(&surface);
    assert(nativeObservedUnit==2&&world.MouseOverUnit==2&&dataCalls==0);
    assert(TextContains("Selected: 1")&&!TextContains("HP ")&&!TextContains("DPS"));
    CheckNativePixels();CheckOnlyCurrentAppend();
    world.MouseOverUnit=0;world.MouseOverFeature=7;ClearFrame();DrawPanel(&surface);
    assert(nativeObservedUnit==0&&world.MouseOverFeature==7&&dataCalls==0);
    assert(TextContains("Selected: 1")&&!TextContains("HP "));CheckNativePixels();

    // Retain the framebuffer across frames like the real native cache does.
    // Switching tooltip, native status/target widths, selections or overlays
    // clears previous custom text through the original renderer, and moves all
    // extra text after the new native ink without changing any native pixels.
    world.MouseOverFeature=0xffff;nativeHonorsCache=true;
    ClearFrame();DrawPanel(&surface);assert(TextContains("DPS~ 100"));
    auto NextFrame=[](){nativeCalls=dataCalls=backgroundCalls=0;texts.clear();nativeCacheValues.clear();};
    NextFrame();world.MouseOverFeature=8;DrawPanel(&surface);
    assert(nativeCacheValues[0]==0xffff&&dataCalls==0&&TextContains("Selected: 1"));
    CheckNativePixels();CheckOnlyCurrentAppend();
    NextFrame();world.MouseOverFeature=0xffff;nativeExtent=650;DrawPanel(&surface);
    assert(texts.front().x==660&&TextContains("DPS~ 100"));
    CheckNativePixels();CheckOnlyCurrentAppend();
    NextFrame();units[1].UnitSelected=0;DrawPanel(&surface);
    assert(nativeCalls==1&&texts.empty()&&nativeCacheValues[0]==0xffff);
    CheckNativePixels();CheckOnlyCurrentAppend();
    NextFrame();Select(1);Select(2,2);nativeExtent=400;DrawPanel(&surface);
    assert(dataCalls==0&&texts.front().x==410&&TextContains("Selected: 2"));
    CheckNativePixels();CheckOnlyCurrentAppend();
    NextFrame();dialogShown=true;DrawPanel(&surface);
    assert(texts.empty()&&nativeCacheValues[0]==0xffff);CheckOnlyCurrentAppend();
    dialogShown=false;

    // Missing side assets and a full native status bar preserve every original
    // pixel, even when no safe room remains to append a label.
    NextFrame();nativeExtent=1195;DrawPanel(&surface);
    assert(texts.empty());CheckNativePixels();CheckOnlyCurrentAppend();
    NextFrame();nativeExtent=310;sequence.Frames=0;DrawPanel(&surface);
    assert(texts.empty());CheckNativePixels();CheckOnlyCurrentAppend();
    sequence.Frames=1;UnitStatsPanel::Shutdown();

    // The global all-enhancements-off command suppresses statistics while
    // the TAHook-only mode leaves this independent presentation feature on.
    Fixture();Select(1);UnitStatsPanel::Install();shared.ehaOff=1;DrawPanel(&surface);
    assert(nativeCalls==1&&dataCalls==0&&texts.empty());CheckNativePixels();
    NextFrame();shared.ehaOff=2;DrawPanel(&surface);
    assert(nativeCalls==1&&dataCalls==1&&TextContains("Selected: 1"));
    CheckNativePixels();CheckOnlyCurrentAppend();UnitStatsPanel::Shutdown();

    // Preflight failure changes no native bytes; failed write verification is
    // rolled back; allocation failure leaves the original call in place.
    Fixture();nativeCode[1]^=1;const auto wrong=nativeCode;UnitStatsPanel::Install();
    assert(!g_enabled&&!g_hook&&hookCreated==0&&nativeCode==wrong);
    Fixture();readFail=true;UnitStatsPanel::Install();assert(!g_enabled&&!g_hook&&hookCreated==0&&nativeCode==Original);
    Fixture();patchFail=true;UnitStatsPanel::Install();assert(!g_enabled&&!g_hook&&hookDestroyed==1&&nativeCode==Original);
    Fixture();failReadNumber=2;UnitStatsPanel::Install();assert(!g_enabled&&!g_hook&&hookDestroyed==1&&nativeCode==Original);
    Fixture();constructorThrow=true;UnitStatsPanel::Install();assert(!g_enabled&&!g_hook&&nativeCode==Original);
    Fixture();config.enabled=false;UnitStatsPanel::Install();UnitStatsPanel::Draw(&surface);
    assert(hookCreated==0&&nativeCalls==0&&dataCalls==0&&nativeCode==Original);
    std::cout<<"Native bottom-bar controller, counts, fallback, clipping and restoration passed\n";
#else
    Fixture();UnitStatsPanel::Install();UnitStatsPanel::Draw(&surface);UnitStatsPanel::Shutdown();
    assert(hookCreated==0&&nativeCalls==0&&dataCalls==0&&nativeCode==Original);
    std::cout<<"Compiled-out unit stats leaves native drawing untouched\n";
#endif
}
'''


def main():
    source = adapted_source()
    with tempfile.TemporaryDirectory(prefix='tadr-unit-stats-panel-') as temporary:
        path = Path(temporary)
        cpp = path / 'panel.cpp'
        cpp.write_text(HARNESS.replace('// PRODUCTION_SOURCE', source))
        environment = os.environ.copy()
        environment.setdefault('ASAN_OPTIONS', 'detect_leaks=0')
        # Native TA has deliberately unaligned packed x86 members. The harness
        # runs with 64-bit host pointers, so alignment is not an ABI test here.
        for enabled in (1, 0):
            executable = path / f'panel-{enabled}'
            command = shlex.split(os.environ.get('CXX', 'g++')) + [
                '-std=c++17', '-Wall', '-Wextra', '-Werror', '-g',
                '-fsanitize=address,undefined', '-fno-sanitize=alignment',
                f'-DUNIT_STATS_PANEL_ENABLE={enabled}', '-I', str(ROOT), str(cpp), '-o', str(executable)]
            subprocess.run(command, check=True, env=environment)
            subprocess.run([str(executable)], check=True, env=environment)


if __name__ == '__main__':
    main()
