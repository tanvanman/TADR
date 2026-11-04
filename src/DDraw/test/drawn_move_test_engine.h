#pragma once
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <vector>
#include "../DrawnMoveGeometry.h"
using HWND=void*;
using UINT=unsigned;
using WPARAM=unsigned;
using LPARAM=unsigned;
using DWORD=unsigned;
using BOOL=int;
#undef NULL
#define NULL 0
#define TRUE 1
#define FALSE 0
#ifndef USEMEGAMAP
#define USEMEGAMAP 1
#endif
#define LOWORD(x) ((x)&0xffff)
#define HIWORD(x) (((x)>>16)&0xffff)
constexpr UINT WM_LBUTTONDOWN=1, WM_LBUTTONUP=2, WM_RBUTTONDOWN=3, WM_RBUTTONUP=4,
    WM_MOUSEMOVE=5, WM_KILLFOCUS=6, WM_CANCELMODE=7, WM_CAPTURECHANGED=8,
    WM_ACTIVATEAPP=9, WM_KEYDOWN=10, WM_MOUSEWHEEL=11;
constexpr unsigned MK_SHIFT=4, MK_CONTROL=8, VK_SHIFT=16;
constexpr int TAInGame=6, Player_LocalHuman=1, WATCH=0x40, UnitSelected_State=0x10,
    canmove=1, cursormove=14, cursornormal=19;
namespace ordertype { enum { STOP=1, MOVE=2, ATTACK=3 }; }
struct RECT { int left,top,right,bottom; };
struct POINT { int x,y; };
struct Position_Dword { unsigned short X=0,Y=0,Z=0; };
struct UnitDef { unsigned UnitTypeMask_0=1; int bmcode=1; };
struct PlayerStruct;
struct UnitStruct {
    short UnitID=1;
    PlayerStruct* Owner_PlayerPtr0=nullptr;
    UnitDef* UnitType=nullptr;
    unsigned UnitSelected=0x10000010;
    float Nanoframe=0;
    unsigned short XPos=100,YPos=100;
};
struct PlayerInfo { unsigned PropertyMask=0; };
struct PlayerStruct {
    struct PlayerInfo* PlayerInfo=nullptr;
    int My_PlayerType=1;
    UnitStruct* Units=nullptr;
    UnitStruct* UnitsAry_End=nullptr;
};
struct TAdynmemStruct {
    int LocalHumanPlayer_PlayerID=0;
    unsigned char PrepareOrder_Type=1;
    PlayerStruct Players[10];
    RECT GameSreen_Rect={128,32,1023,735};
    int EyeBallMapXPos=0,EyeBallMapYPos=0,MapSizeX=4096,MapSizeY=4096;
    int FeatureMapSizeX=256,FeatureMapSizeY=264,SeaLevel=0;
    int MouseOverUnit=0,CurrentCursora_Index=cursormove,InterfaceType=1;
    UnitStruct* BeginUnitsArray_p=nullptr;
    UnitStruct* EndOfUnitsArray_p=nullptr;
};
TAdynmemStruct world;
TAdynmemStruct* worldPtr=&world;
TAdynmemStruct** TAmainStruct_PtrPtr=&worldPtr;
struct Share { int TAProgress=TAInGame; bool PlayingDemo=false; } share;
Share* DataShare=&share;
struct Config { BOOL GetIniBool(const char*, BOOL) { return TRUE; } } config;
Config* MyConfig=&config;
HWND capture=0;
HWND GetCapture() { return capture; }
void ReleaseCapture() { capture=0; }
void SetCapture(HWND w) { capture=w; }
void GetPosition_Dword(int x,int y,Position_Dword* p) { p->X=x; p->Y=y; p->Z=0; }
int GetPosHeight(Position_Dword*) { return 0; }
void UpdateSelectUnitEffect() {}
struct Order { unsigned x,y,z; bool shift; std::vector<int> selected; };
std::vector<Order> orders;
void SendOrder(unsigned x,unsigned y,unsigned z,int kind,bool shift) {
    assert(kind==ordertype::MOVE);
    Order order={x,y,z,shift,{}};
    for (auto* u=world.BeginUnitsArray_p;u<world.EndOfUnitsArray_p;++u)
        if (u->UnitSelected & UnitSelected_State) order.selected.push_back(int(u-world.BeginUnitsArray_p));
    orders.push_back(order);
    // The dispatcher is allowed to change other state. Restoration must not
    // overwrite the whole UnitSelected word from an earlier snapshot.
    if (world.BeginUnitsArray_p) world.BeginUnitsArray_p->UnitSelected |= 0x100;
}
struct DDSURFACEDESC {
    unsigned dwSize=0,dwWidth=1024,dwHeight=768;
    int lPitch=1024;
    void* lpSurface=nullptr;
    struct { int dwRGBBitCount=8; } ddpfPixelFormat;
};
constexpr int DDLOCK_WAIT=1,DDLOCK_SURFACEMEMORYPTR=2,DD_OK=0;
struct FakeSurface {
    std::vector<unsigned char> pixels=std::vector<unsigned char>(1024*768);
    bool locked=false;
    int Lock(void*,DDSURFACEDESC* d,int,void*) { assert(!locked);locked=true;d->lpSurface=pixels.data();return DD_OK; }
    void Unlock(void*) { assert(locked);locked=false; }
};
using LPDIRECTDRAWSURFACE=FakeSurface*;
#include "../SmallMapViewport.h"

int CorretCursor_InGame(unsigned char) { return world.CurrentCursora_Index; }
// Coordinate projection is provided by the real megamap controller in-game.
// This fake verifies the gesture selects that interface, not normal-camera
// offsets, and that overlay painting uses the same inverse projection.
struct MegaMapControl {
    bool visible=false;
    int cursorX=0,cursorY=0;
    RECT viewport={200,100,800,600};
    bool IsBliting() { return visible; }
    bool FormationGroundPoint(Position_Dword* point,int x,int y) {
        if (!visible || x<viewport.left || x>=viewport.right || y<viewport.top || y>=viewport.bottom) return false;
        point->X=(x-viewport.left)*4;
        point->Y=(y-viewport.top)*6;
        point->Z=0;
        cursorX=x;cursorY=y;
        return true;
    }
    bool FormationScreenPoint(POINT* screen,RECT* clip,const Position_Dword& point) {
        screen->x=viewport.left+point.X/4;
        screen->y=viewport.top+(point.Y-point.Z/2)/6;
        *clip=viewport;--clip->right;--clip->bottom;
        return visible;
    }
} megaController;
struct FakeMinimap { MegaMapControl* Controler=&megaController; } fakeMinimap;
struct FakeExpander { FakeMinimap* myMinimap=&fakeMinimap; } fakeExpander;
FakeExpander* GUIExpander=&fakeExpander;

struct IDDrawSurface {
    static void OutptTxt(const char*) {}
    static void OutptFmtTxt(const char*,...) {}
};
