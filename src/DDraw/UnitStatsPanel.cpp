#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "config.h"
#include "UnitStatsPanel.h"

#if UNIT_STATS_PANEL_ENABLE
#include "oddraw.h"
#include "tamem.h"
#include "tafunctions.h"
#include "iddrawsurface.h"
#include "TAConfig.h"
#include "dialog.h"
#include "Gaf.h"
#include "hook/hook.h"
#include "UnitStatsCore.h"
#include "UnitStatsData.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace
{
    constexpr unsigned kInfoPanelCall = 0x00469615u;
    constexpr unsigned char kExpectedCall[5] = {0xE8,0x46,0x12,0x00,0x00};
    constexpr unsigned kInfoPanel = 0x0046A860u;
    constexpr unsigned kMaximumUnitSlots = 65535u;
    using NativeDraw = void (__stdcall*)(OFFSCREEN*);
    NativeDraw const DrawNative = reinterpret_cast<NativeDraw>(kInfoPanel);

    std::unique_ptr<SingleHook> g_hook;
    unsigned char g_callPatch[5] = {};
    bool g_enabled = false;
    bool g_customDisplayed = false;
    unsigned g_lastCount = 0;

    struct Request
    {
        TAdynmemStruct* world = nullptr;
        unsigned selected = 0;
        unsigned targetCount = 0;
        unsigned type = 0;
        const UnitStruct* single = nullptr;
        const UnitStruct* representative = nullptr;
        bool buildHover = false;
        bool stats = false;
        bool allowed = false;
        int side = -1;
    };

    bool InBank(const UnitStruct* unit, const UnitStruct* first, const UnitStruct* last)
    {
        const auto u = reinterpret_cast<std::uintptr_t>(unit);
        const auto a = reinterpret_cast<std::uintptr_t>(first);
        const auto b = reinterpret_cast<std::uintptr_t>(last);
        return first && last && a <= b && u >= a && u <= b
            && (u-a) % sizeof(UnitStruct) == 0
            && (b-a) / sizeof(UnitStruct) < kMaximumUnitSlots;
    }

    bool OwnSelected(const UnitStruct& unit, const PlayerStruct* owner)
    {
        return unit.Owner_PlayerPtr0 == owner && unit.UnitType
            && (unit.UnitSelected & UnitSelected_State)
            && (unit.UnitSelected & 0x10000000u) && !(unit.UnitSelected & 0x4000u);
    }

    bool NormalSidebar(const GUIMEMSTRUCT* gui)
    {
        if (!gui || !gui->ControlsAry) return false;
        const GUI0IDControl& panel = gui->ControlsAry[0];
        // The native build/order layers occupy the left sidebar. Chat and
        // runtime options are separate upper layers and retain their own UI.
        return panel.width > 0 && panel.width <= 128 && panel.xpos >= 0
            && panel.xpos < 128 && panel.height > 0;
    }

    int HoveredBuildType(TAdynmemStruct* ta, const GUIMEMSTRUCT* gui)
    {
        if (!NormalSidebar(gui)) return -1;
        const GUI0IDControl* controls = gui->ControlsAry;
        const int count = controls[0].totalgadgets;
        const int index = ta->desktopGUI.field_68; // native hoverGadgetIndex
        if (count <= 0 || count > 256 || index <= 0 || index > count) return -1;
        const GUI0IDControl& control = controls[index];
        if (control.id != 1 || !control.active || control.width <= 0 || control.height <= 0)
            return -1;
        const int x = ta->CurtMousePostion.x, y = ta->CurtMousePostion.y;
        const int left = controls[0].xpos + control.xpos;
        const int top = controls[0].ypos + control.ypos;
        if (x < left || y < top || x >= left + control.width || y >= top + control.height)
            return -1;
        char name[17] = {};
        std::memcpy(name, control.name, 16);
        if (!name[0] || _stricmp(name,"CORBUILD") == 0) return -1;
        for (unsigned i=0; i<ta->UNITINFOCount; ++i)
            if (_strnicmp(ta->UnitDef[i].UnitName,name,sizeof(ta->UnitDef[i].UnitName)) == 0)
                return static_cast<int>(i);
        return -1;
    }

    Request Resolve()
    {
        Request request;
        if (!g_enabled || !DataShare || DataShare->TAProgress != TAInGame
            || DataShare->ehaOff == 1) return request;
        TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
        if (!ta || !ta->UnitDef || ta->UNITINFOCount <= 0 || ta->UNITINFOCount > 65535)
            return request;
        request.world = ta;
        const int player = ta->LocalHumanPlayer_PlayerID;
        if (player < 0 || player >= 10 || !ta->Players[player].PlayerInfo) return request;
        const PlayerStruct* owner = &ta->Players[player];
        if (owner->My_PlayerType != Player_LocalHuman
            || (owner->PlayerInfo->PropertyMask & WATCH)) return request;
        request.side = static_cast<unsigned char>(owner->PlayerInfo->RaceSide);
        if (request.side >= 5) return request;

        bool sameType = true;
        if (InBank(owner->Units,ta->BeginUnitsArray_p,ta->EndOfUnitsArray_p)
            && InBank(owner->UnitsAry_End,ta->BeginUnitsArray_p,ta->EndOfUnitsArray_p)
            && InBank(owner->Units,owner->Units,owner->UnitsAry_End))
        {
            for (const UnitStruct* unit=owner->Units; unit<=owner->UnitsAry_End; ++unit)
            {
                if (!OwnSelected(*unit,owner)) continue;
                const unsigned type = static_cast<unsigned short>(unit->UnitID);
                if (type >= ta->UNITINFOCount || unit->UnitType != &ta->UnitDef[type]) continue;
                if (!request.selected)
                {
                    request.type=type;
                    request.single=unit;
                    request.representative=unit;
                }
                else if (request.type != type) sameType=false;
                ++request.selected;
            }
        }
        if (request.selected != 1) request.single=nullptr;

        // Leave PFSTATE, victory/defeat, chat and the native options UI in charge.
        if ((ta->GameStateMask & 3) == 3 || (ta->GameStateMask & 0x60)) return request;
        if (LocalShare && LocalShare->Dialog)
        {
            RECT rect = {};
            if (reinterpret_cast<Dialog*>(LocalShare->Dialog)->IsShow(&rect)) return request;
        }
        GUIMEMSTRUCT* gui = ta->desktopGUI.TheActive_GUIMEM;
        if (!NormalSidebar(gui)) return request;
        request.allowed=true;
        const int hover = HoveredBuildType(ta,gui);
        if (hover >= 0)
        {
            request.type=static_cast<unsigned>(hover);
            request.targetCount=1;
            request.single=nullptr;
            request.representative=nullptr;
            request.buildHover=true;
            request.stats=true;
        }
        else if (request.selected && sameType)
        {
            request.targetCount=request.selected;
            request.stats=true;
            // A real hovered object keeps its native tooltip. Do not label
            // an enemy/other unit's panel with the selected group's values.
            if (ta->MouseOverUnit)
            {
                const std::uintptr_t first=reinterpret_cast<std::uintptr_t>(ta->BeginUnitsArray_p);
                const std::uintptr_t address=first+static_cast<std::size_t>(ta->MouseOverUnit)*sizeof(UnitStruct);
                const auto* hovered=reinterpret_cast<const UnitStruct*>(address);
                if (!InBank(hovered,ta->BeginUnitsArray_p,ta->EndOfUnitsArray_p)
                    || !OwnSelected(*hovered,owner)
                    || static_cast<unsigned short>(hovered->UnitID) != request.type)
                    request.stats=false;
            }
            else if (ta->MouseOverFeature != 0xffffu) request.stats=false;
        }
        return request;
    }

    Request ResolveSafe()
    {
        __try { return Resolve(); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return Request(); }
    }

    void DirtyNativeCache()
    {
        __try
        {
            TAdynmemStruct* ta=*TAmainStruct_PtrPtr;
            if (ta && DataShare && DataShare->TAProgress == TAInGame)
            {
                // The cache's selected slot can never be 0xffff in the native
                // panel. Only local presentation state is invalidated.
                ta->InfoPanelCachedUnit = 0xffffu;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    struct NativeSelection
    {
        TAdynmemStruct* world = nullptr;
        unsigned short hovered = 0;
        int button = -1;
        bool changed = false;
    };

    NativeSelection PrepareNativeSelection(const Request& request)
    {
        NativeSelection saved;
        __try
        {
            TAdynmemStruct* ta=request.world;
            const UnitStruct* unit=request.representative;
            if (!request.allowed || !request.stats || request.buildHover || !ta || !unit
                || ta != *TAmainStruct_PtrPtr || ta->MouseOverUnit
                || ta->MouseOverFeature != 0xffffu
                || !InBank(unit,ta->BeginUnitsArray_p,ta->EndOfUnitsArray_p)
                || !OwnSelected(*unit,&ta->Players[ta->LocalHumanPlayer_PlayerID])) return saved;
            const std::uintptr_t index=(reinterpret_cast<std::uintptr_t>(unit)
                -reinterpret_cast<std::uintptr_t>(ta->BeginUnitsArray_p))/sizeof(UnitStruct);
            if (!index || index > 65535u) return saved;
            saved.world=ta;
            saved.hovered=ta->MouseOverUnit;
            saved.button=ta->desktopGUI.field_68;
            saved.changed=true;
            // These two fields select only the local information renderer.
            // No selection flag, unit record or order is changed.
            ta->MouseOverUnit=static_cast<unsigned short>(index);
            ta->desktopGUI.field_68=-1;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return saved;
    }

    void DrawNativeSelected(OFFSCREEN* surface, const Request& request)
    {
        NativeSelection saved=PrepareNativeSelection(request);
        __try { DrawNative(surface); }
        __finally
        {
            if (saved.changed && saved.world == *TAmainStruct_PtrPtr)
            {
                saved.world->MouseOverUnit=saved.hovered;
                saved.world->desktopGUI.field_68=saved.button;
            }
        }
    }

    struct FontState
    {
        TAProgramStruct* program = nullptr;
        unsigned char* font = nullptr;
        int front = 0, back = 0, alpha = 0;
        RECT clip = {};
        bool saved = false;
    };

    int Right(const OFFSCREEN& surface)
    {
        return std::min(surface.Width-1,static_cast<int>(surface.ScreenRect.right));
    }

    bool Background(OFFSCREEN* surface, TAdynmemStruct* ta, int side, RECT clip)
    {
        if (!surface || !surface->lpSurface || surface->Width <= 0 || surface->Height < 32
            || surface->lPitch < surface->Width) return false;
        GAFSequence* sequence=ta->InfoPanelBackgrounds[side];
        if (!sequence || !sequence->Frames) return false;
        GAFFrame* frame=sequence->PtrFrameAry[0].PtrFrame;
        if (!frame || !frame->Width || frame->Width > 4096 || !frame->Height || frame->Height > 4096)
            return false;
        const RECT saved=surface->ScreenRect;
        surface->ScreenRect=clip;
        for (int x=129; x<surface->Width; x+=frame->Width)
            CopyGafToContext(surface,frame,x+static_cast<short>(frame->xPosition),
                surface->Height-32+static_cast<short>(frame->yPosition));
        surface->ScreenRect=saved;
        return true;
    }

    int NativeRight(OFFSCREEN* surface, const Request& request)
    {
        const int right=Right(*surface);
        if (!surface->lpSurface || surface->Height < 32 || right < 130
            || right >= 16384 || surface->lPitch < right+1) return right;
        // Re-render only the native background into a private 32-pixel strip.
        // Comparing actual ink also covers localized orders, variable-width
        // numbers, colour/logo blocks, health/reload bars and target details.
        // The original bar is never cleared or copied over by this feature.
        std::vector<unsigned char> background(static_cast<std::size_t>(right+1)*32u,0);
        OFFSCREEN scratch = {};
        scratch.Width=right+1;
        scratch.Height=32;
        scratch.lPitch=right+1;
        scratch.lpSurface=background.data();
        scratch.ScreenRect={130,0,right,31};
        if (!Background(&scratch,request.world,request.side,scratch.ScreenRect)) return right;
        int occupied=129;
        const auto* pixels=static_cast<const unsigned char*>(surface->lpSurface)
            +static_cast<std::size_t>(surface->Height-32)*surface->lPitch;
        for (int y=0; y<32; ++y)
        {
            const auto* actual=pixels+static_cast<std::size_t>(y)*surface->lPitch;
            const auto* plain=background.data()+static_cast<std::size_t>(y)*(right+1);
            for (int x=right; x>occupied; --x)
                if (actual[x] != plain[x]) { occupied=x; break; }
        }
        return occupied;
    }

    // All allocations and ordinary C++ exceptions stay in this helper, outside
    // the SEH font/clip restoration envelope below.
    bool Paint(OFFSCREEN* surface, const Request& request, bool statistics)
    {
        try
        {
            auto* program=*TAProgramStruct_PtrPtr;
            const auto& side=request.world->RaceSideDataAry[request.side];
            auto* font=reinterpret_cast<unsigned char*>(side.Font_File);
            if (!font || font[0] <= 0 || font[0] > 24) return false;
            const int left=std::max(std::max(130,static_cast<int>(surface->ScreenRect.left)),
                NativeRight(surface,request)+10);
            const int right=Right(*surface)-3;
            const int top=surface->Height-32;
            if (right <= left || top < surface->ScreenRect.top) return false;
            RECT bar={left,top,right,surface->Height-1};
            program->fontHandle=font;
            program->fontFrontColour=0x53; // native side-panel text colour
            program->fontBackColour=program->fontAlpha;
            const int gap=1;
            const bool twoRows=2*font[0]+gap <= 32;
            const int firstY=top+std::max(0,(32-(twoRows ? 2*font[0]+gap : font[0]))/2);
            std::vector<std::string> tokens;
            if (request.selected)
            {
                std::string count="Selected: "+std::to_string(request.selected);
                if (GetTextExtent(font,count.c_str()) > right-left)
                    count="Sel: "+std::to_string(request.selected);
                if (GetTextExtent(font,count.c_str()) > right-left)
                    count="#"+std::to_string(request.selected);
                tokens.push_back(count);
            }
            if (statistics && twoRows)
            {
                // A failing stat snapshot must never hide the selection count
                // or replace the information already drawn by the game.
                UnitStatsCore::Stats stats;
                bool read=false;
                try { read=UnitStatsData::Read(request.world,request.type,
                    request.single,request.targetCount,stats); }
                catch (...) {}
                if (read)
                {
                    std::string hp="HP ";
                    if (stats.liveHealth && stats.count == 1)
                        hp+=std::to_string(stats.currentHealth)+"/";
                    hp+=std::to_string(stats.maxHealth);
                    if (stats.count > 1) hp+=" (each)";
                    tokens.push_back(hp);
                    const auto fields=UnitStatsCore::Fields(stats);
                    tokens.insert(tokens.end(),fields.begin(),fields.end());
                }
            }
            std::string rows[2];
            int row=0;
            for (std::size_t i=0; i<tokens.size() && i<64; ++i)
            {
                const auto& token=tokens[i];
                if (token.size() > 255 || GetTextExtent(font,token.c_str()) > right-left) continue;
                std::string next=rows[row].empty() ? token : rows[row]+"   "+token;
                if (next.size() > 255 || GetTextExtent(font,next.c_str()) > right-left)
                {
                    if (row || !twoRows) continue;
                    row=1;
                    next=token;
                }
                rows[row]=next;
            }
            surface->ScreenRect=bar;
            for (int i=0; i<2; ++i)
                if (!rows[i].empty()) DrawTextInScreen(surface,const_cast<char*>(rows[i].c_str()),
                    left,firstY+i*(font[0]+gap),-1);
            return !rows[0].empty() || !rows[1].empty();
        }
        catch (...) { return false; }
    }

    bool PaintRestored(OFFSCREEN* surface, const Request& request, bool statistics)
    {
        FontState saved;
        __try
        {
            if (!surface || !*TAProgramStruct_PtrPtr) return false;
            saved.program=*TAProgramStruct_PtrPtr;
            saved.font=saved.program->fontHandle;
            saved.front=saved.program->fontFrontColour;
            saved.back=saved.program->fontBackColour;
            saved.alpha=saved.program->fontAlpha;
            saved.clip=surface->ScreenRect;
            saved.saved=true;
            return Paint(surface,request,statistics);
        }
        __finally
        {
            if (saved.saved)
            {
                saved.program->fontHandle=saved.font;
                saved.program->fontFrontColour=saved.front;
                saved.program->fontBackColour=saved.back;
                saved.program->fontAlpha=saved.alpha;
                surface->ScreenRect=saved.clip;
            }
        }
    }

    bool PaintSafe(OFFSCREEN* surface, const Request& request, bool statistics)
    {
        __try { return PaintRestored(surface,request,statistics); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    void __stdcall DrawPanel(OFFSCREEN* surface)
    {
        const Request request=ResolveSafe();
        const bool append=request.allowed && (request.stats || request.selected);
        // The native renderer clears its own bar before drawing all original
        // details. Invalidate its local presentation cache while appending so
        // text from the previous frame cannot be mistaken for native ink.
        if (append || g_customDisplayed || g_lastCount != request.selected) DirtyNativeCache();
        DrawNativeSelected(surface,request);
        g_customDisplayed=append && PaintSafe(surface,request,request.stats);
        g_lastCount=request.selected;
    }

    bool ReadBytes(unsigned address, void* output, std::size_t size)
    {
        SIZE_T read=0;
        return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),
            output,size,&read) && read == size;
    }

    bool CopySurface(OFFSCREEN* source, OFFSCREEN& copy)
    {
        __try
        {
            TAdynmemStruct* ta=*TAmainStruct_PtrPtr;
            if (!ta || !source) return false;
            // MegamapTAStuff's source context uses pitch as Width and can
            // retain the battlefield-only clip. Draw on a bounded copy so the
            // right-hand padding and the caller's clipping remain untouched.
            copy=*source;
            copy.Width=std::min(copy.Width,copy.lPitch);
            copy.Width=std::min(copy.Width,static_cast<int>(ta->ScreenWidth));
            copy.Height=std::min(copy.Height,static_cast<int>(ta->ScreenHeight));
            if (copy.Width <= 130 || copy.Height < 32) return false;
            copy.ScreenRect={0,0,copy.Width-1,copy.Height-1};
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
}

namespace UnitStatsPanel
{
    void Install()
    {
        if (g_hook) return;
        g_enabled=!MyConfig || MyConfig->GetIniBool("UnitStatsPanel",TRUE) != FALSE;
        if (!g_enabled) return;
        unsigned char original[5] = {};
        if (!ReadBytes(kInfoPanelCall,original,sizeof(original))
            || std::memcmp(original,kExpectedCall,sizeof(original)) != 0)
        {
            g_enabled=false;
            IDDrawSurface::OutptTxt("[UnitStatsPanel] unexpected native UI call; skipped");
            return;
        }
        std::memset(g_callPatch,0,sizeof(g_callPatch));
        g_callPatch[0]=0xE8;
        const std::int32_t relative=static_cast<std::int32_t>(
            reinterpret_cast<std::uintptr_t>(&DrawPanel)-kInfoPanelCall-5);
        std::memcpy(g_callPatch+1,&relative,sizeof(relative));
        try
        {
            g_hook.reset(new SingleHook(kInfoPanelCall,5,INLINE_UNPROTECTEVINMENT,g_callPatch));
            unsigned char installed[5] = {};
            if (!ReadBytes(kInfoPanelCall,installed,sizeof(installed))
                || std::memcmp(installed,g_callPatch,sizeof(installed)) != 0)
            {
                g_hook.reset();
                g_enabled=false;
                return;
            }
        }
        catch (...) { g_hook.reset(); g_enabled=false; return; }
        IDDrawSurface::OutptTxt("[UnitStatsPanel] build=bottom-bar-v2");
    }

    void Shutdown()
    {
        if (g_customDisplayed) DirtyNativeCache();
        g_hook.reset();
        g_enabled=false;
        g_customDisplayed=false;
        g_lastCount=0;
    }

    void Draw(OFFSCREEN* surface)
    {
        if (!g_enabled) return;
        OFFSCREEN copy = {};
        if (CopySurface(surface,copy)) DrawPanel(&copy);
    }
}
#else
namespace UnitStatsPanel
{
    void Install() {}
    void Shutdown() {}
    void Draw(_OFFSCREEN*) {}
}
#endif
