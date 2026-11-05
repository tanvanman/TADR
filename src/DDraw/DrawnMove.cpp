#include "config.h"
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
#include <mutex>

#ifndef USEMEGAMAP
#error "DrawnMove requires config.h to define USEMEGAMAP"
#endif

namespace
{
    using DrawnMoveGeometry::Point;
    // WinProc and the presentation callback may run on different threads.
    // Capture changes can re-enter WinProc synchronously on this thread.
    std::recursive_mutex gestureMutex;
    struct UnitRef { UnitStruct* unit; short id; };
    struct SelectionRef {
        UnitStruct* unit;
        short type;
        UnitDefStruct* definition;
        bool wasLive;
    };
    struct Gesture {
        HWND window = NULL;
        UINT button = 0;
        bool cancelled = false, dragged = false, shift = false;
        int player = -1, startX = 0, startY = 0;
        TAdynmemStruct* world = NULL;
        unsigned char prepared = 0;
        bool megamap = false;
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
        if (!ta || DataShare->TAProgress != TAInGame || DataShare->PlayingDemo) return false;
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
            || ta->PrepareOrder_Type != g.prepared || Megamap() != g.megamap) return false;
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
#if USEMEGAMAP
        if (Megamap()) {
            Position_Dword pos = {};
            if (!GUIExpander->myMinimap->Controler->FormationGroundPoint(&pos,x,y)) return false;
            out={double(pos.X),double(pos.Y)};
            return true;
        }
#endif
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
    bool IsLiveOwned(UnitStruct* unit, PlayerStruct* owner) {
        return unit->UnitID != 0 && unit->Owner_PlayerPtr0 == owner && unit->UnitType
            && (unit->UnitSelected & 0x10000000) && !(unit->UnitSelected & 0x4000);
    }
    SelectionRef CaptureSelection(UnitStruct* unit, PlayerStruct* owner) {
        return {unit,unit->UnitID,unit->UnitType,IsLiveOwned(unit,owner)};
    }
    bool SameLiveUnit(const SelectionRef& ref, PlayerStruct* owner) {
        // UnitID is a type id, not a generation: do not restore a slot that
        // became free, dead, changed definition, or changed owner during dispatch.
        return ref.wasLive && IsLiveOwned(ref.unit,owner) && ref.unit->UnitID == ref.type
            && ref.unit->UnitType == ref.definition;
    }
    bool SameSlotBlock(PlayerStruct* owner, UnitStruct* first, UnitStruct* last) {
        return owner->Units == first && owner->UnitsAry_End == last;
    }
    void ClearSelection(UnitStruct* first, UnitStruct* last) {
        // This is exactly the inclusive range scanned by TA's selected-unit
        // dispatcher. A free slot can retain selection in its stale bytes;
        // clearing only live UnitID entries leaves that slot receiving orders.
        for (UnitStruct* unit=first; unit<=last; ++unit)
            unit->UnitSelected &= ~UnitSelected_State;
    }
    std::vector<SelectionRef> SnapshotSelection(PlayerStruct* owner, UnitStruct* first, UnitStruct* last) {
        std::vector<SelectionRef> selection;
        unsigned staleFree=0, staleOther=0;
        for (UnitStruct* u=first; u<=last; ++u) {
            if (!(u->UnitSelected & UnitSelected_State)) continue;
            const SelectionRef ref=CaptureSelection(u,owner);
            selection.push_back(ref);
            if (!ref.wasLive) {
                if (u->UnitID == 0) ++staleFree;
                else ++staleOther;
            }
        }
        if (staleFree || staleOther)
            IDDrawSurface::OutptFmtTxt("[DrawnMove] quarantined selected slots: free=%u dead_or_foreign=%u",
                staleFree,staleOther);
        return selection;
    }
    void ApplyLiveSelection(PlayerStruct* owner, const std::vector<SelectionRef>& selection) {
        for (std::size_t i=0; i<selection.size(); ++i)
            if (SameLiveUnit(selection[i],owner)) selection[i].unit->UnitSelected |= UnitSelected_State;
    }
    void RestoreSelection(TAdynmemStruct* ta, PlayerStruct* owner, UnitStruct* first, UnitStruct* last,
        const std::vector<SelectionRef>& selection, bool shift, unsigned char prepared) {
        if (!SameSlotBlock(owner,first,last)) return;
        ClearSelection(first,last);
        ApplyLiveSelection(owner,selection);
        ta->PrepareOrder_Type=shift ? prepared : static_cast<unsigned char>(ordertype::STOP);
        UpdateSelectUnitEffect();
    }
    // Use the normal UI/network dispatcher, but isolate its exact local slot
    // block. Remote player selections are never changed.
    void DispatchFormation(TAdynmemStruct*, PlayerStruct*, UnitStruct*, UnitStruct*,
        const std::vector<SelectionRef>&, const std::vector<SelectionRef>&,
        const std::vector<Position_Dword>&, bool, unsigned char);
    bool CommitOrders() {
        static const bool noOrders=MyConfig->GetIniBool("DrawnMoveTestNoOrders",FALSE) != FALSE;
        return !noOrders;
    }
    void Issue(TAdynmemStruct* ta) {
        if (!CommitOrders()) return;
        std::vector<Point> origins;
        for (const UnitRef& ref : g.units) origins.push_back({double(ref.unit->XPos),double(ref.unit->YPos)});
        const std::vector<Point> slots=DrawnMoveGeometry::Resample(g.path,g.units.size());
        const std::vector<std::size_t> assignment=DrawnMoveGeometry::Assign(origins,slots);
        PlayerStruct* owner=&ta->Players[g.player];
        UnitStruct* first=owner->Units;
        UnitStruct* last=owner->UnitsAry_End;
        if (!first || !last || last < first) return;
        const std::vector<SelectionRef> selection=SnapshotSelection(owner,first,last);
        std::vector<SelectionRef> targets;
        for (const UnitRef& ref : g.units) targets.push_back(CaptureSelection(ref.unit,owner));
        // Precompute all allocations / terrain lookups before changing selection.
        std::vector<Position_Dword> destinations;
        for (std::size_t i=0; i<g.units.size(); ++i) destinations.push_back(TerrainPosition(slots[assignment[i]]));
        const unsigned char prepared=ta->PrepareOrder_Type;
        // __finally also restores selection if an engine call raises an SEH
        // exception; the outer WinProc exception handler must not strand it.
        // Kept in a separate function below to avoid MSVC C2712 (SEH + vectors).
        DispatchFormation(ta,owner,first,last,selection,targets,destinations,g.shift,prepared);
    }
    void Complete(TAdynmemStruct* ta) {
        __try { Issue(ta); }
        __finally { Finish(); }
    }
    void DispatchFormation(TAdynmemStruct* ta, PlayerStruct* owner, UnitStruct* first, UnitStruct* last,
        const std::vector<SelectionRef>& selection, const std::vector<SelectionRef>& targets,
        const std::vector<Position_Dword>& destinations, bool shift, unsigned char prepared) {
        __try {
            for (std::size_t i=0; i<targets.size(); ++i) {
                // Recheck after every engine call. Do not touch a retired slot
                // block, and do not dispatch to a unit freed/transferred by a
                // preceding callback.
                if (!SameSlotBlock(owner,first,last)) break;
                ClearSelection(first,last);
                if (!SameLiveUnit(targets[i],owner)) continue;
                UnitStruct* unit=targets[i].unit;
                unit->UnitSelected |= UnitSelected_State;
                UpdateSelectUnitEffect();
                const Position_Dword& p=destinations[i];
                SendOrder(p.X,p.Y,p.Z,ordertype::MOVE,shift);
            }
        }
        __finally {
            RestoreSelection(ta,owner,first,last,selection,shift,prepared);
        }
    }
    void DispatchGroup(TAdynmemStruct* ta, PlayerStruct* owner, UnitStruct* first, UnitStruct* last,
        const std::vector<SelectionRef>& selection, const Position_Dword& position, bool shift,
        unsigned char prepared) {
        __try {
            if (SameSlotBlock(owner,first,last)) {
                ClearSelection(first,last);
                ApplyLiveSelection(owner,selection);
                UpdateSelectUnitEffect();
                SendOrder(position.X,position.Y,position.Z,ordertype::MOVE,shift);
            }
        }
        __finally {
            RestoreSelection(ta,owner,first,last,selection,shift,prepared);
        }
    }
    void IssueClick(TAdynmemStruct* ta, const Position_Dword& position) {
        if (!CommitOrders()) return;
        PlayerStruct* owner=&ta->Players[g.player];
        UnitStruct* first=owner->Units;
        UnitStruct* last=owner->UnitsAry_End;
        if (!first || !last || last < first) return;
        const std::vector<SelectionRef> selection=SnapshotSelection(owner,first,last);
        DispatchGroup(ta,owner,first,last,selection,position,g.shift,ta->PrepareOrder_Type);
    }
    void CompleteClick(TAdynmemStruct* ta, const Position_Dword& position) {
        __try { IssueClick(ta,position); }
        __finally { Finish(); }
    }

}

namespace DrawnMove
{
    void LogBuild() {
        IDDrawSurface::OutptTxt("[DrawnMove] build=alt-v6");
#if USEMEGAMAP
        IDDrawSurface::OutptTxt("[DrawnMove] compiled-megamap=enabled");
#else
        IDDrawSurface::OutptTxt("[DrawnMove] compiled-megamap=disabled");
#endif
    }
    bool OwnsInput() {
        std::lock_guard<std::recursive_mutex> lock(gestureMutex);
        return g.button != 0 || suppressedUp != 0;
    }
    bool Message(HWND window, UINT message, WPARAM keys, LPARAM position, bool allowStart, bool fromMegamap) {
        std::lock_guard<std::recursive_mutex> lock(gestureMutex);
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
                || ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
                    && keys != VK_SHIFT && keys != VK_MENU)
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
                        CompleteClick(ta,p);
                        return true;
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
        // Activation is sampled only at the press. Once captured, always
        // handle release/cancellation even if Alt is released first. Mouse
        // wParam has Shift/Ctrl bits but no Alt bit, so query VK_MENU.
        if (!allowStart || (GetAsyncKeyState(VK_MENU) & 0x8000) == 0
            || !Playable(ta) || Megamap() != fromMegamap
            || (message != WM_LBUTTONDOWN && message != WM_RBUTTONDOWN)) return false;
        static unsigned megamapPressLogs=0;
        const bool trace=fromMegamap && megamapPressLogs++ < 32;
        static const bool enabled=MyConfig->GetIniBool("DrawnMoveFormations",TRUE) != FALSE;
        // Megamap input refreshes MouseOverUnit for this exact press before
        // entering here. Resolve its contextual cursor now instead of using
        // the previous frame's cursor (which may still indicate a unit).
        const int cursor=fromMegamap ? CorretCursor_InGame(ta->PrepareOrder_Type) : ta->CurrentCursora_Index;
        // In the right-button interface, STOP means contextual input: the
        // megamap can legitimately retain the ordinary arrow over empty ground
        // until a movement-button press. It is not an invalid move cursor.
        const bool movementCursor=cursor == cursormove
            || (fromMegamap && cursor == cursornormal && ta->PrepareOrder_Type == ordertype::STOP);
        if (!enabled || ta->MouseOverUnit || !movementCursor) {
            if (trace) IDDrawSurface::OutptFmtTxt(
                "[DrawnMove] megamap rejected: enabled=%d hover=%u cursor=%d prepared=%u",
                enabled,ta->MouseOverUnit,cursor,ta->PrepareOrder_Type);
            return false;
        }
        const bool explicitMove=ta->PrepareOrder_Type == ordertype::MOVE;
        if (!explicitMove && ta->PrepareOrder_Type != ordertype::STOP) return false;
        const UINT moveButton=explicitMove || ta->InterfaceType == 0 ? WM_LBUTTONDOWN : WM_RBUTTONDOWN;
        if (message != moveButton) return false;
        Point point;
        if (!GroundPoint(ta,x,y,point)) {
            if (trace) IDDrawSurface::OutptTxt("[DrawnMove] megamap rejected: outside playable map");
            return false;
        }
        const int player=ta->LocalHumanPlayer_PlayerID;
        PlayerStruct* owner=&ta->Players[player];
        if (!owner->Units || !owner->UnitsAry_End) return false;
        Gesture next;
        for (UnitStruct* u=owner->Units; u<=owner->UnitsAry_End; ++u)
            if (SelectedMobile(u,owner)) next.units.push_back({u,u->UnitID});
        if (next.units.size() < 2) {
            if (trace) IDDrawSurface::OutptFmtTxt(
                "[DrawnMove] megamap rejected: eligible mobile units=%u",static_cast<unsigned>(next.units.size()));
            return false;
        }
        next.window=window; next.button=message; next.player=player; next.world=ta;
        next.prepared=ta->PrepareOrder_Type; next.shift=(keys & MK_SHIFT) != 0;
        next.megamap=fromMegamap;
        next.startX=x; next.startY=y; next.path.push_back(point);
        g=next;
        SetCapture(window);
        IDDrawSurface::OutptFmtTxt("[DrawnMove] begin view=%s units=%u shift=%d cursor=%d preview=%d orders=%d",
            fromMegamap ? "megamap" : "battlefield",static_cast<unsigned>(g.units.size()),g.shift,cursor,
            MyConfig->GetIniBool("DrawnMovePreview",TRUE),CommitOrders());
        return true;
    }

    void Draw(LPDIRECTDRAWSURFACE surface) {
        static const bool preview=MyConfig->GetIniBool("DrawnMovePreview",TRUE) != FALSE;
        if (!preview) return;
        std::lock_guard<std::recursive_mutex> lock(gestureMutex);
        if (!OwnsInput() || g.cancelled || !g.dragged || !ValidGesture(*TAmainStruct_PtrPtr)) return;
        const std::vector<Point> markers=DrawnMoveGeometry::Resample(g.path,g.units.size());
        std::vector<POINT> positions;
        RECT clip=g.world->GameSreen_Rect;
        for (const Point& point : markers) {
            const Position_Dword p=TerrainPosition(point);
            POINT screen = {};
#if USEMEGAMAP
            if (g.megamap) {
                if (!GUIExpander->myMinimap->Controler->FormationScreenPoint(&screen,&clip,p)) continue;
            } else
#endif
            {
                screen.x=p.X-g.world->EyeBallMapXPos+clip.left;
                screen.y=p.Y-g.world->EyeBallMapYPos+clip.top-p.Z/2;
            }
            positions.push_back(screen);
        }
        DDSURFACEDESC desc = {};
        desc.dwSize=sizeof(desc);
        if (surface->Lock(NULL,&desc,DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR,NULL) != DD_OK) return;
        if (desc.ddpfPixelFormat.dwRGBBitCount != 8) { surface->Unlock(NULL); return; }
        const RECT& r=clip;
        auto pixel=[&](int x,int y,int color) {
            if (x>=r.left && x<=r.right && y>=r.top && y<=r.bottom && x>=0 && y>=0
                && x<static_cast<int>(desc.dwWidth) && y<static_cast<int>(desc.dwHeight))
                static_cast<unsigned char*>(desc.lpSurface)[y*desc.lPitch+x]=static_cast<unsigned char>(color);
        };
        for (const POINT& p : positions) {
            const int x=p.x, y=p.y;
            for (int d=-3; d<=3; ++d) {
                pixel(x+d,y-3,234); pixel(x+d,y+3,234);
                pixel(x-3,y+d,234); pixel(x+3,y+d,234);
            }
        }
        surface->Unlock(NULL);
    }
}
