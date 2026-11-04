#include "oddraw.h"
#include "iddrawsurface.h"
#include "tamem.h"
#include "tafunctions.h"
#include "TAConfig.h"
#include "fullscreenminimap.h"
#include "GUIExpand.h"
#include "MegamapControl.h"
#include "SmallMapViewport.h"
#include "DrawnMove.h"
#include "DrawnMoveGeometry.h"
#include <algorithm>

namespace
{
    using DrawnMoveGeometry::Point;
    struct UnitRef { UnitStruct* unit; short id; };
    struct Gesture {
        HWND window = NULL;
        UINT button = 0;
        bool cancelled = false, dragged = false, shift = false;
        int player = -1, startX = 0, startY = 0;
        TAdynmemStruct* world = NULL;
        unsigned char prepared = 0;
        std::vector<Point> path;
        std::vector<UnitRef> units;
    } g;
    UINT suppressedUp = 0;

    bool Megamap() {
#if USEMEGAMAP
        return GUIExpander && GUIExpander->myMinimap && GUIExpander->myMinimap->Controler
            && GUIExpander->myMinimap->Controler->IsBliting();
#else
        return false;
#endif
    }
    bool Playable(TAdynmemStruct* ta) {
        if (!ta || DataShare->TAProgress != TAInGame || DataShare->PlayingDemo || Megamap()) return false;
        const int player = ta->LocalHumanPlayer_PlayerID;
        return player >= 0 && player < 10 && ta->Players[player].PlayerInfo
            && !(ta->Players[player].PlayerInfo->PropertyMask & WATCH)
            && ta->Players[player].My_PlayerType == Player_LocalHuman;
    }
    bool SelectedMobile(UnitStruct* u, PlayerStruct* owner) {
        return u->UnitID && u->Owner_PlayerPtr0 == owner && u->UnitType
            && (u->UnitSelected & UnitSelected_State) && (u->UnitSelected & 0x10000000)
            && !(u->UnitSelected & 0x4000) && u->Nanoframe == 0.0f
            && (u->UnitType->UnitTypeMask_0 & canmove) && u->UnitType->bmcode != 0;
    }
    bool ValidGesture(TAdynmemStruct* ta) {
        if (!Playable(ta) || ta != g.world || ta->LocalHumanPlayer_PlayerID != g.player
            || ta->PrepareOrder_Type != g.prepared) return false;
        PlayerStruct* owner = &ta->Players[g.player];
        for (const UnitRef& ref : g.units)
            if (ref.unit->UnitID != ref.id || !SelectedMobile(ref.unit,owner)) return false;
        // Selection changes (including adding another mobile unit) invalidate
        // the frozen snapshot rather than unexpectedly ordering a new group.
        std::size_t count=0;
        if (!owner->Units || !owner->UnitsAry_End) return false;
        for (UnitStruct* u=owner->Units; u<=owner->UnitsAry_End; ++u)
            if (SelectedMobile(u,owner)) ++count;
        return count == g.units.size();
    }
    bool GroundPoint(TAdynmemStruct* ta, int x, int y, Point& out) {
        const RECT& r=ta->GameSreen_Rect;
        if (!SmallMapViewport::ContainsScreen({r.left,r.top,r.right+1,r.bottom+1},
            ta->EyeBallMapXPos,ta->EyeBallMapYPos,ta->MapSizeX,ta->MapSizeY,x,y)) return false;
        const int wx=x-r.left+ta->EyeBallMapXPos;
        const int projectedY=y-r.top+ta->EyeBallMapYPos;
        // Use the engine's terrain projection, with world/projected coordinates
        // exactly as its normal pointer update does. Do not reuse MouseMapPos:
        // WinProc runs before TA processes this event, so it can be one event old.
        Position_Dword pos = {};
        GetPosition_Dword(wx,projectedY,&pos);
        if (pos.X >= ta->FeatureMapSizeX*16 || pos.Y >= (ta->FeatureMapSizeY-8)*16) return false;
        out={double(pos.X),double(pos.Y)};
        return true;
    }
    Position_Dword TerrainPosition(Point p) {
        Position_Dword pos = {};
        pos.X=static_cast<unsigned short>(p.x+0.5);
        pos.Y=static_cast<unsigned short>(p.y+0.5);
        const int height=GetPosHeight(&pos);
        pos.Z=static_cast<unsigned short>(height < 0 ? g.world->SeaLevel : height);
        return pos;
    }
    void Cancel() {
        g.cancelled=true;
        g.path.clear();
        if (g.window && GetCapture() == g.window) ReleaseCapture();
    }
    void Finish() {
        HWND window=g.window;
        g=Gesture();
        if (window && GetCapture() == window) ReleaseCapture();
    }
    void Sample(Point point, bool endpoint) {
        if (!g.path.empty() && DrawnMoveGeometry::DistanceSquared(g.path.back(),point)<16 && !endpoint) return;
        if (g.path.size() >= 2048) {
            std::vector<Point> reduced;
            for (std::size_t i=0; i<g.path.size(); i+=2) reduced.push_back(g.path[i]);
            reduced.push_back(g.path.back());
            g.path.swap(reduced);
        }
        g.path.push_back(point);
    }
    // The normal selected-unit UI dispatcher also emits the normal network /
    // recorder commands. The lower-level SendOrder(unit,...) helper bypasses
    // that dispatcher. Restore only selection bits, not simulation state.
    void DispatchFormation(TAdynmemStruct*, const std::vector<UnitStruct*>&,
        const std::vector<UnitRef>&, const std::vector<Position_Dword>&, bool, unsigned char);
    void Issue(TAdynmemStruct* ta) {
        std::vector<Point> origins;
        for (const UnitRef& ref : g.units) origins.push_back({double(ref.unit->XPos),double(ref.unit->YPos)});
        const std::vector<Point> slots=DrawnMoveGeometry::Resample(g.path,g.units.size());
        const std::vector<std::size_t> assignment=DrawnMoveGeometry::Assign(origins,slots);
        std::vector<UnitStruct*> selection;
        for (UnitStruct* u=ta->BeginUnitsArray_p; u<ta->EndOfUnitsArray_p; ++u)
            if (u->UnitID && (u->UnitSelected & UnitSelected_State)) selection.push_back(u);
        // Precompute all allocations / terrain lookups before changing selection.
        std::vector<Position_Dword> destinations;
        for (std::size_t i=0; i<g.units.size(); ++i) destinations.push_back(TerrainPosition(slots[assignment[i]]));
        const unsigned char prepared=ta->PrepareOrder_Type;
        // __finally also restores selection if an engine call raises an SEH
        // exception; the outer WinProc exception handler must not strand it.
        // Kept in a separate function below to avoid MSVC C2712 (SEH + vectors).
        DispatchFormation(ta,selection,g.units,destinations,g.shift,prepared);
    }
    void Complete(TAdynmemStruct* ta) {
        __try { Issue(ta); }
        __finally { Finish(); }
    }
    void DispatchFormation(TAdynmemStruct* ta, const std::vector<UnitStruct*>& selection,
        const std::vector<UnitRef>& units, const std::vector<Position_Dword>& destinations, bool shift, unsigned char prepared) {
        __try {
            for (UnitStruct* u : selection) u->UnitSelected &= ~UnitSelected_State;
            for (std::size_t i=0; i<units.size(); ++i) {
                UnitStruct* u=units[i].unit;
                u->UnitSelected |= UnitSelected_State;
                UpdateSelectUnitEffect();
                const Position_Dword& p=destinations[i];
                SendOrder(p.X,p.Y,p.Z,ordertype::MOVE,shift);
                u->UnitSelected &= ~UnitSelected_State;
            }
        }
        __finally {
            for (const UnitRef& ref : units) ref.unit->UnitSelected &= ~UnitSelected_State;
            for (UnitStruct* u : selection) u->UnitSelected |= UnitSelected_State;
            ta->PrepareOrder_Type=shift ? prepared : ordertype::STOP;
            UpdateSelectUnitEffect();
        }
    }
}

namespace DrawnMove
{
    bool OwnsInput() { return g.button != 0 || suppressedUp != 0; }
    bool Message(HWND window, UINT message, WPARAM keys, LPARAM position, bool allowStart) {
        TAdynmemStruct* ta=*TAmainStruct_PtrPtr;
        const int x=static_cast<short>(LOWORD(position)), y=static_cast<short>(HIWORD(position));
        if (message == suppressedUp && suppressedUp != 0) { suppressedUp=0; return true; }
        // Capture loss may mean the button was released outside this window.
        // A fresh press must not leave the next gesture stuck in cancelled state.
        if (g.button && g.cancelled && (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN)) Finish();
        if (g.button) {
            if ((message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN) && message != g.button)
                suppressedUp=message == WM_LBUTTONDOWN ? WM_LBUTTONUP : WM_RBUTTONUP;
            const UINT up=g.button == WM_LBUTTONDOWN ? WM_LBUTTONUP : WM_RBUTTONUP;
            if (message == WM_KILLFOCUS || message == WM_CANCELMODE || message == WM_CAPTURECHANGED
                || (message == WM_ACTIVATEAPP && !keys)
                || (message == WM_KEYDOWN && keys != VK_SHIFT)
                || message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MOUSEWHEEL)
                Cancel();
            if (!g.cancelled && !ValidGesture(ta)) Cancel();
            if (message == up) {
                const int dx=x-g.startX, dy=y-g.startY;
                if (dx*dx+dy*dy >= 36) g.dragged=true;
                Point point;
                if (!g.cancelled && GroundPoint(ta,x,y,point)) {
                    if (g.dragged) { Sample(point,true); Complete(ta); return true; }
                    else {
                        const Position_Dword p=TerrainPosition(g.path.front());
                        SendOrder(p.X,p.Y,p.Z,ordertype::MOVE,g.shift);
                        ta->PrepareOrder_Type=g.shift ? g.prepared : ordertype::STOP;
                    }
                }
                Finish();
                return true;
            }
            if (message == WM_MOUSEMOVE) {
                if (!g.cancelled) {
                    const int dx=x-g.startX, dy=y-g.startY;
                    if (dx*dx+dy*dy >= 36) g.dragged=true;
                    Point point;
                    if (GroundPoint(ta,x,y,point)) Sample(point,false);
                    else Cancel(); // includes UI and centred-map black margins
                }
                return true;
            }
            // Eat the other mouse button too: it cancels without issuing a
            // context order on its later release. Keyboard events still reach TA.
            return message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN
                || message == WM_LBUTTONUP || message == WM_RBUTTONUP;
        }
        if (!allowStart || !Playable(ta) || (message != WM_LBUTTONDOWN && message != WM_RBUTTONDOWN)) return false;
        static const bool enabled=MyConfig->GetIniBool("DrawnMoveFormations",TRUE) != FALSE;
        if (!enabled || ta->MouseOverUnit || ta->CurrentCursora_Index != cursormove) return false;
        const bool explicitMove=ta->PrepareOrder_Type == ordertype::MOVE;
        if (!explicitMove && ta->PrepareOrder_Type != ordertype::STOP) return false;
        const UINT moveButton=explicitMove || ta->InterfaceType == 0 ? WM_LBUTTONDOWN : WM_RBUTTONDOWN;
        if (message != moveButton || (keys & MK_CONTROL)) return false;
        Point point;
        if (!GroundPoint(ta,x,y,point)) return false;
        const int player=ta->LocalHumanPlayer_PlayerID;
        PlayerStruct* owner=&ta->Players[player];
        if (!owner->Units || !owner->UnitsAry_End) return false;
        Gesture next;
        for (UnitStruct* u=owner->Units; u<=owner->UnitsAry_End; ++u)
            if (SelectedMobile(u,owner)) next.units.push_back({u,u->UnitID});
        if (next.units.size() < 2) return false;
        next.window=window; next.button=message; next.player=player; next.world=ta;
        next.prepared=ta->PrepareOrder_Type; next.shift=(keys & MK_SHIFT) != 0;
        next.startX=x; next.startY=y; next.path.push_back(point);
        g=next;
        SetCapture(window);
        return true;
    }

    void Draw(LPDIRECTDRAWSURFACE surface) {
        if (!OwnsInput() || g.cancelled || !g.dragged || !ValidGesture(*TAmainStruct_PtrPtr)) return;
        const std::vector<Point> markers=DrawnMoveGeometry::Resample(g.path,g.units.size());
        std::vector<Position_Dword> positions;
        for (const Point& point : markers) positions.push_back(TerrainPosition(point));
        DDSURFACEDESC desc = {};
        desc.dwSize=sizeof(desc);
        if (surface->Lock(NULL,&desc,DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR,NULL) != DD_OK) return;
        if (desc.ddpfPixelFormat.dwRGBBitCount != 8) { surface->Unlock(NULL); return; }
        const RECT& r=g.world->GameSreen_Rect;
        auto pixel=[&](int x,int y,int color) {
            if (x>=r.left && x<=r.right && y>=r.top && y<=r.bottom && x>=0 && y>=0
                && x<static_cast<int>(desc.dwWidth) && y<static_cast<int>(desc.dwHeight))
                static_cast<unsigned char*>(desc.lpSurface)[y*desc.lPitch+x]=static_cast<unsigned char>(color);
        };
        for (const Position_Dword& p : positions) {
            const int x=p.X-g.world->EyeBallMapXPos+r.left;
            const int y=p.Y-g.world->EyeBallMapYPos+r.top-p.Z/2;
            for (int d=-3; d<=3; ++d) {
                pixel(x+d,y-3,234); pixel(x+d,y+3,234);
                pixel(x-3,y+d,234); pixel(x+3,y+d,234);
            }
        }
        surface->Unlock(NULL);
    }
}
