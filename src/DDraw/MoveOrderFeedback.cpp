#include "config.h"
#include "oddraw.h"
#include "MoveOrderFeedback.h"

#if DRAWN_MOVE_FORMATIONS_ENABLE
#include "tamem.h"
#include "tafunctions.h"
#include "iddrawsurface.h"
#include "MoveOrderFeedbackGeometry.h"
#if USEMEGAMAP
#include "GUIExpand.h"
#include "fullscreenminimap.h"
#include "MegamapControl.h"
#endif
#include <array>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include <vector>

namespace
{
    namespace Geometry = MoveOrderFeedbackGeometry;
    constexpr unsigned lifetime = 500;
    constexpr std::size_t capacity = 4096;
    struct FeedbackRecord
    {
        Position_Dword origin = {}, destination = {};
        MoveOrderFeedback::ActorIdentity identity = {};
        unsigned unitIndex = 0, at = 0;
        unsigned cacheIndex = 0, sequence = 0;
        bool active = false;
    };
    std::mutex feedbackMutex;
    std::array<FeedbackRecord, capacity> records;
    std::size_t nextRecord = 0;
    TAdynmemStruct* savedWorld = nullptr;
    UnitStruct* savedFirst = nullptr;
    UnitStruct* savedLast = nullptr;
    UnitStruct* savedOwnFirst = nullptr;
    UnitStruct* savedOwnLast = nullptr;
    int savedPlayer = -1, lastGameTime = -1;
    unsigned savedDpid = 0, contextSerial = 0, recordSequence = 0;
    unsigned diagnosticDraws = 0, diagnosticRejects = 0;

    void LogRecordRejection(unsigned unitIndex, short type, unsigned reason)
    {
        __try
        {
            IDDrawSurface::OutptFmtTxt("[MoveOrderFeedback] record-rejected unit=%u type=%d reason=%u (1=context 2=position)",
                unitIndex, static_cast<int>(type), reason);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void Clear()
    {
        for (auto& item : records) item.active = false;
        nextRecord = 0;
        savedWorld = nullptr; savedFirst = savedLast = savedOwnFirst = savedOwnLast = nullptr;
        savedPlayer = -1; savedDpid = 0; lastGameTime = -1;
        ++contextSerial;
    }
    bool Playable(TAdynmemStruct* ta)
    {
        if (!ta || !DataShare || DataShare->TAProgress != TAInGame
            || DataShare->PlayingDemo || DataShare->ehaOff) return false;
        const int player = ta->LocalHumanPlayer_PlayerID;
        return player >= 0 && player < 10 && ta->Players[player].PlayerInfo
            && !(ta->Players[player].PlayerInfo->PropertyMask & WATCH)
            && ta->Players[player].My_PlayerType == Player_LocalHuman;
    }
    bool Context(TAdynmemStruct* ta)
    {
        __try
        {
            if (!Playable(ta) || !ta->BeginUnitsArray_p || !ta->EndOfUnitsArray_p
                || ta->EndOfUnitsArray_p < ta->BeginUnitsArray_p)
            {
                Clear();
                return false;
            }
            const int player = ta->LocalHumanPlayer_PlayerID;
            const auto& owner = ta->Players[player];
            if (!owner.Units || !owner.UnitsAry_End || owner.UnitsAry_End < owner.Units)
            {
                Clear();
                return false;
            }
            const unsigned dpid = static_cast<unsigned>(owner.DirectPlayID);
            if (ta != savedWorld || player != savedPlayer || dpid != savedDpid
                || ta->BeginUnitsArray_p != savedFirst || ta->EndOfUnitsArray_p != savedLast
                || owner.Units != savedOwnFirst || owner.UnitsAry_End != savedOwnLast
                || ta->GameTime < lastGameTime)
            {
                Clear();
                savedWorld = ta; savedPlayer = player; savedDpid = dpid;
                savedFirst = ta->BeginUnitsArray_p; savedLast = ta->EndOfUnitsArray_p;
                savedOwnFirst = owner.Units; savedOwnLast = owner.UnitsAry_End;
            }
            lastGameTime = ta->GameTime;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { Clear(); return false; }
    }
    bool InMap(const TAdynmemStruct& ta, const Position_Dword& p)
    {
        return ta.FeatureMapSizeX > 0 && ta.FeatureMapSizeY > 8
            && p.X < static_cast<std::int64_t>(ta.FeatureMapSizeX) * 16
            && p.Y < static_cast<std::int64_t>(ta.FeatureMapSizeY - 8) * 16;
    }
    TAdynmemStruct* World()
    {
        __try { return TAmainStruct_PtrPtr ? *TAmainStruct_PtrPtr : nullptr; }
        __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }
    bool ReadPalette(TAdynmemStruct* ta, Geometry::Palette& palette)
    {
        __try
        {
            if (World() != ta || !Playable(ta) || !ta->TAProgramStruct_Ptr) return false;
            // Same active base R,G,B,0 colormap used by MapParse::MegamapColormap.
            // Copy values only: no native palette pointer survives this helper.
            const unsigned char* data = reinterpret_cast<const unsigned char*>(ta->TAProgramStruct_Ptr) + 0x214;
            for (std::size_t i = 0; i < palette.size(); ++i)
                palette[i] = {data[i * 4], data[i * 4 + 1], data[i * 4 + 2]};
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    bool SamePalette(const Geometry::Palette& a, const Geometry::Palette& b)
    {
        for (std::size_t i = 0; i < a.size(); ++i)
            if (a[i].red != b[i].red || a[i].green != b[i].green || a[i].blue != b[i].blue)
                return false;
        return true;
    }
    Geometry::BlendTable PaletteBlend(const Geometry::Palette& palette)
    {
        static std::mutex paletteMutex;
        static Geometry::Palette cachedPalette = {};
        static Geometry::BlendTable cachedBlend = {};
        static bool ready = false;
        std::lock_guard<std::mutex> lock(paletteMutex);
        if (!ready || !SamePalette(cachedPalette, palette))
        {
            cachedBlend = Geometry::MakeBlendTable(palette);
            cachedPalette = palette;
            ready = true;
        }
        // Rendering receives an immutable copy, so no cache mutex crosses a
        // surface Lock or native projection and no pixel performs a color search.
        return cachedBlend;
    }
    bool CapturePositions(TAdynmemStruct* ta, unsigned unitIndex, const Position_Dword& origin,
        const Position_Dword& destination, Position_Dword& from, Position_Dword& to)
    {
        __try
        {
            if (unitIndex > static_cast<std::size_t>(ta->EndOfUnitsArray_p - ta->BeginUnitsArray_p)
                || !InMap(*ta, origin) || !InMap(*ta, destination)) return false;
            from = origin; to = destination;
            if (static_cast<short>(from.Z) < 0) from.Z = static_cast<unsigned short>(ta->SeaLevel);
            if (static_cast<short>(to.Z) < 0) to.Z = static_cast<unsigned short>(ta->SeaLevel);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    bool SameDestination(const Position_Dword& a, const Position_Dword& b)
    {
        // Match the native Shift toggle's sixteen-world-unit tolerance.
        return std::abs(static_cast<int>(a.X) - b.X) <= 16
            && std::abs(static_cast<int>(a.Y) - b.Y) <= 16;
    }
    Geometry::Rect HalfOpen(const RECT& r)
    {
        return {r.left, r.top,
            SmallMapViewport::Bounded(static_cast<std::int64_t>(r.right) + 1,
                (std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)()),
            SmallMapViewport::Bounded(static_cast<std::int64_t>(r.bottom) + 1,
                (std::numeric_limits<int>::min)(), (std::numeric_limits<int>::max)())};
    }
    int ScreenCoordinate(std::int64_t value)
    {
        return SmallMapViewport::Bounded(value, (std::numeric_limits<int>::min)(),
            (std::numeric_limits<int>::max)());
    }
    Geometry::Rect PopupRect(const TAdynmemStruct& ta)
    {
        const auto* gui = ta.desktopGUI.TheActive_GUIMEM;
        if (!gui || !gui->per_active || !gui->ControlsAry) return {};
        const auto& panel = *gui->ControlsAry;
        return {panel.xpos, panel.ypos, panel.xpos + panel.width, panel.ypos + panel.height};
    }
    struct ScreenRecord
    {
        POINT from, to;
        Geometry::Rect clip;
        unsigned at;
        FeedbackRecord actor;
    };
    struct Scene
    {
        TAdynmemStruct* world = nullptr;
        RECT viewport = {};
        Geometry::Rect clip = {}, excluded = {};
        int cameraX = 0, cameraY = 0, mapWidth = 0, mapHeight = 0;
        UnitStruct* first = nullptr;
        UnitStruct* last = nullptr;
        UnitStruct* ownerFirst = nullptr;
        UnitStruct* ownerLast = nullptr;
        int player = -1;
        unsigned dpid = 0;
        bool megamap = false;
#if USEMEGAMAP
        MegaMapControl* map = nullptr;
        POINT projectionOrigin = {}, projectionFar = {};
        Position_Dword farPoint = {};
        RECT megaClip = {};
#endif
    };
    struct PaintStats
    {
        const char* stage = "scene-changed";
        unsigned submitted = 0, liveRejected = 0, expired = 0;
    };
    void LogRender(bool enabled, const char* stage, unsigned count, unsigned projected,
        const Scene& scene, const DDSURFACEDESC& desc, const ScreenRecord& first, const PaintStats& stats)
    {
        if (!enabled) return;
        __try
        {
            IDDrawSurface::OutptFmtTxt("[MoveOrderFeedback] render stage=%s records=%u projected=%u submitted=%u live-rejected=%u expired=%u view=%s clip=%d,%d,%d,%d popup=%d,%d,%d,%d first=%ld,%ld->%ld,%ld surface=%lu,%lu pitch=%ld bpp=%lu",
                stage, count, projected, stats.submitted, stats.liveRejected, stats.expired,
                scene.megamap ? "mega" : "battle", scene.clip.left, scene.clip.top, scene.clip.right, scene.clip.bottom,
                scene.excluded.left, scene.excluded.top, scene.excluded.right, scene.excluded.bottom,
                static_cast<long>(first.from.x), static_cast<long>(first.from.y),
                static_cast<long>(first.to.x), static_cast<long>(first.to.y),
                static_cast<unsigned long>(desc.dwWidth), static_cast<unsigned long>(desc.dwHeight),
                static_cast<long>(desc.lPitch), static_cast<unsigned long>(desc.ddpfPixelFormat.dwRGBBitCount));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
    bool ReadScene(TAdynmemStruct* ta, Scene& out)
    {
        __try
        {
            if (!Playable(ta)) return false;
            out.world = ta; out.viewport = ta->GameSreen_Rect;
            out.first = ta->BeginUnitsArray_p; out.last = ta->EndOfUnitsArray_p;
            out.player = ta->LocalHumanPlayer_PlayerID;
            out.ownerFirst = ta->Players[out.player].Units;
            out.ownerLast = ta->Players[out.player].UnitsAry_End;
            out.dpid = ta->Players[out.player].DirectPlayID;
            out.cameraX = ta->EyeBallMapXPos; out.cameraY = ta->EyeBallMapYPos;
            out.mapWidth = ta->MapSizeX; out.mapHeight = ta->MapSizeY;
            out.clip = SmallMapViewport::MapClip(HalfOpen(out.viewport), out.cameraX, out.cameraY,
                out.mapWidth, out.mapHeight);
            out.excluded = PopupRect(*ta);
#if USEMEGAMAP
            out.map = GUIExpander && GUIExpander->myMinimap ? GUIExpander->myMinimap->Controler : nullptr;
            out.megamap = out.map && out.map->IsBliting();
            if (out.megamap)
            {
                const Position_Dword origin = {};
                out.farPoint.X = static_cast<unsigned short>(SmallMapViewport::Bounded(
                    static_cast<std::int64_t>(ta->FeatureMapSizeX) * 16 - 1, 0, 65535));
                out.farPoint.Y = static_cast<unsigned short>(SmallMapViewport::Bounded(
                    static_cast<std::int64_t>(ta->FeatureMapSizeY) * 16 - 129, 0, 65535));
                if (!out.map->MoveFeedbackScreenPoint(&out.projectionOrigin, &out.megaClip, origin)
                    || !out.map->MoveFeedbackScreenPoint(&out.projectionFar, &out.megaClip, out.farPoint)) return false;
                out.clip = HalfOpen(out.megaClip);
            }
#endif
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    bool LiveOrigin(const Scene& scene, const FeedbackRecord& record, Position_Dword& origin)
    {
        __try
        {
            TAdynmemStruct* ta = scene.world;
            if (World() != ta || !Playable(ta) || ta->LocalHumanPlayer_PlayerID != scene.player
                || ta->BeginUnitsArray_p != scene.first || ta->EndOfUnitsArray_p != scene.last)
                return false;
            const PlayerStruct* owner = &ta->Players[scene.player];
            if (owner->Units != scene.ownerFirst || owner->UnitsAry_End != scene.ownerLast
                || owner->DirectPlayID != scene.dpid) return false;
            const std::uintptr_t first = reinterpret_cast<std::uintptr_t>(scene.first);
            const std::uintptr_t last = reinterpret_cast<std::uintptr_t>(scene.last);
            const std::uintptr_t ownFirst = reinterpret_cast<std::uintptr_t>(scene.ownerFirst);
            const std::uintptr_t ownLast = reinterpret_cast<std::uintptr_t>(scene.ownerLast);
            if (!first || !last || first > last || !ownFirst || ownFirst > ownLast
                || record.unitIndex > (last - first) / sizeof(UnitStruct)) return false;
            const std::uintptr_t address = first + record.unitIndex * sizeof(UnitStruct);
            if (address < ownFirst || address > ownLast || (address - ownFirst) % sizeof(UnitStruct))
                return false;
            // Resolve the actor anew from the current bounded bank. Cached model,
            // script and definition addresses are opaque identity values only.
            const UnitStruct* unit = reinterpret_cast<const UnitStruct*>(address);
            unsigned short birthSalt = 0;
            std::memcpy(&birthSalt, unit->data9, sizeof(birthSalt));
            if (!unit->UnitID || unit->UnitID != record.identity.type
                || static_cast<unsigned short>(unit->UnitInGameIndex) != record.unitIndex
                || reinterpret_cast<std::uintptr_t>(unit->UnitType) != record.identity.definition
                || reinterpret_cast<std::uintptr_t>(unit->UnkPTR2) != record.identity.script
                || reinterpret_cast<std::uintptr_t>(unit->Object3do) != record.identity.model
                || birthSalt != record.identity.birthSalt || unit->Owner_PlayerPtr0 != owner
                || !(unit->UnitSelected & 0x10000000u) || (unit->UnitSelected & 0x4000u)
                || !unit->UnitType)
                return false;
            origin = Position_Dword{};
            origin.x_ = unit->XPos__; origin.X = unit->XPos;
            origin.z_ = unit->ZPos__; origin.Z = unit->ZPos;
            origin.y_ = unit->YPos__; origin.Y = unit->YPos;
            if (!InMap(*ta, origin)) return false;
            if (static_cast<short>(origin.Z) < 0) origin.Z = static_cast<unsigned short>(ta->SeaLevel);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    void Forget(const FeedbackRecord& record, unsigned serial)
    {
        std::lock_guard<std::mutex> lock(feedbackMutex);
        if (serial != contextSerial || record.cacheIndex >= capacity) return;
        auto& saved = records[record.cacheIndex];
        if (saved.sequence == record.sequence) saved.active = false;
    }
    bool Project(const Scene& scene, const FeedbackRecord& record, ScreenRecord& out)
    {
        __try
        {
            Position_Dword origin = {};
            if (!LiveOrigin(scene, record, origin)) return false;
            out.at = record.at; out.clip = scene.clip; out.actor = record;
#if USEMEGAMAP
            if (scene.megamap)
            {
                RECT clip = {};
                return scene.map->MoveFeedbackScreenPoint(&out.from, &clip, origin)
                    && scene.map->MoveFeedbackScreenPoint(&out.to, &clip, record.destination);
            }
#endif
            out.from.x = ScreenCoordinate(static_cast<std::int64_t>(origin.X)
                - scene.cameraX + scene.viewport.left);
            out.from.y = ScreenCoordinate(static_cast<std::int64_t>(origin.Y)
                - scene.cameraY + scene.viewport.top - origin.Z / 2);
            out.to.x = ScreenCoordinate(static_cast<std::int64_t>(record.destination.X)
                - scene.cameraX + scene.viewport.left);
            out.to.y = ScreenCoordinate(static_cast<std::int64_t>(record.destination.Y)
                - scene.cameraY + scene.viewport.top - record.destination.Z / 2);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    bool SameRect(Geometry::Rect a, Geometry::Rect b)
    {
        return a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom;
    }
    bool CurrentScene(const Scene& before, unsigned serial)
    {
        {
            std::lock_guard<std::mutex> lock(feedbackMutex);
            if (World() != before.world) { Clear(); return false; }
            if (!Context(before.world) || serial != contextSerial) return false;
        }
        Scene after = {};
        if (!ReadScene(before.world, after)) return false;
        if (before.megamap != after.megamap || before.cameraX != after.cameraX
            || before.cameraY != after.cameraY || before.mapWidth != after.mapWidth
            || before.mapHeight != after.mapHeight || !SameRect(before.clip, after.clip)
            || !SameRect(before.excluded, after.excluded)) return false;
#if USEMEGAMAP
        if (before.megamap && (before.map != after.map
            || before.projectionOrigin.x != after.projectionOrigin.x
            || before.projectionOrigin.y != after.projectionOrigin.y
            || before.projectionFar.x != after.projectionFar.x
            || before.projectionFar.y != after.projectionFar.y)) return false;
#endif
        std::lock_guard<std::mutex> lock(feedbackMutex);
        return serial == contextSerial && before.world == savedWorld;
    }
    bool LockSurface(LPDIRECTDRAWSURFACE surface, DDSURFACEDESC& desc)
    {
        __try
        {
            return surface->Lock(nullptr, &desc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, nullptr) == DD_OK;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    void PaintWithUnlock(LPDIRECTDRAWSURFACE surface, const DDSURFACEDESC& desc, const Scene& scene,
        const std::vector<ScreenRecord>& projected, unsigned serial, PaintStats& stats,
        const Geometry::Palette& palette, const Geometry::BlendTable& blend)
    {
        __try
        {
            if (!CurrentScene(scene, serial)) return;
            stats.stage = "palette-changed";
            Geometry::Palette currentPalette = {};
            if (!ReadPalette(scene.world, currentPalette) || !SamePalette(palette, currentPalette)) return;
            stats.stage = "surface-format";
            if (desc.ddpfPixelFormat.dwRGBBitCount == 8 && desc.dwWidth <= 65536 && desc.dwHeight <= 65536)
            {
                stats.stage = "painted";
                const unsigned now = GetTickCount();
                for (std::size_t i = 0; i < projected.size(); ++i)
                {
                    ScreenRecord item = {};
                    if (!Project(scene, projected[i].actor, item))
                    {
                        ++stats.liveRejected;
                        Forget(projected[i].actor, serial);
                        continue;
                    }
                    if (now - item.at >= lifetime) { ++stats.expired; continue; }
                    const Geometry::Surface target = {static_cast<unsigned char*>(desc.lpSurface),
                        static_cast<int>(desc.dwWidth), static_cast<int>(desc.dwHeight), static_cast<int>(desc.lPitch),
                        item.clip, scene.excluded, &blend};
                    if (!Geometry::Valid(target)) { stats.stage = "surface-invalid"; continue; }
                    Geometry::Draw(target, item.from.x, item.from.y, item.to.x, item.to.y, scene.megamap);
                    ++stats.submitted;
                }
            }
        }
        __finally { surface->Unlock(nullptr); }
    }
    void PaintProtected(LPDIRECTDRAWSURFACE surface, const DDSURFACEDESC& desc, const Scene& scene,
        const std::vector<ScreenRecord>& projected, unsigned serial, PaintStats& stats,
        const Geometry::Palette& palette, const Geometry::BlendTable& blend)
    {
        __try { PaintWithUnlock(surface, desc, scene, projected, serial, stats, palette, blend); }
        __except (EXCEPTION_EXECUTE_HANDLER) { stats.stage = "paint-fault"; }
    }
}

namespace MoveOrderFeedback
{
    void Record(TAdynmemStruct* world, unsigned unitIndex,
        const ActorIdentity& identity, const Position_Dword& origin, const Position_Dword& destination)
    {
        unsigned rejected = 0;
        bool diagnostic = false;
        {
            std::lock_guard<std::mutex> lock(feedbackMutex);
            Position_Dword from = {}, to = {};
            if (!Context(world)) rejected = 1;
            else if (!CapturePositions(world, unitIndex, origin, destination, from, to)) rejected = 2;
            if (rejected)
            {
                diagnostic = diagnosticRejects < 6;
                if (diagnostic) ++diagnosticRejects;
            }
            else
            {
                // Fixed storage makes capture constant-time and allocation-free
                // for a large selection. Oldest feedback is replaced at the cap.
                auto& item = records[nextRecord];
                item.cacheIndex = static_cast<unsigned>(nextRecord); item.sequence = ++recordSequence;
                nextRecord = (nextRecord + 1) % capacity;
                item.origin = from; item.destination = to;
                item.identity = identity;
                item.unitIndex = unitIndex; item.at = GetTickCount(); item.active = true;
            }
        }
        if (diagnostic) LogRecordRejection(unitIndex, identity.type, rejected);
    }
    void Cancel(TAdynmemStruct* world, unsigned unitIndex, const Position_Dword& destination)
    {
        std::lock_guard<std::mutex> lock(feedbackMutex);
        if (!Context(world)) return;
        for (auto& item : records)
            if (item.active && item.unitIndex == unitIndex && SameDestination(item.destination, destination))
                item.active = false;
    }
    void Draw(LPDIRECTDRAWSURFACE surface)
    {
        TAdynmemStruct* ta = World();
        std::vector<FeedbackRecord> snapshot;
        unsigned serial = 0;
        bool diagnostic = false;
        try
        {
            std::lock_guard<std::mutex> lock(feedbackMutex);
            if (!Context(ta)) return;
            const unsigned now = GetTickCount();
            for (auto& item : records)
            {
                if (!item.active) continue;
                if (now - item.at >= lifetime) { item.active = false; continue; }
                snapshot.push_back(item);
            }
            serial = contextSerial;
            diagnostic = !snapshot.empty() && diagnosticDraws < 6;
            if (diagnostic) ++diagnosticDraws;
        }
        catch (...) { return; }
        if (!surface || snapshot.empty()) return;

        // All native reads and projections are guarded POD helpers. DirectDraw
        // Lock may yield: the epoch/view is validated again before painting.
        Scene scene = {};
        DDSURFACEDESC desc = {};
        ScreenRecord first = {};
        PaintStats stats = {};
        if (!ReadScene(ta, scene))
        {
            LogRender(diagnostic, "scene-unavailable", static_cast<unsigned>(snapshot.size()), 0,
                scene, desc, first, stats);
            return;
        }
        std::vector<ScreenRecord> projected;
        try
        {
            projected.reserve(snapshot.size());
            for (const auto& item : snapshot)
            {
                ScreenRecord out = {};
                if (Project(scene, item, out)) projected.push_back(out);
                else Forget(item, serial);
            }
        }
        catch (...)
        {
            LogRender(diagnostic, "projection-allocation", static_cast<unsigned>(snapshot.size()), 0,
                scene, desc, first, stats);
            return;
        }
        if (!projected.empty()) first = projected.front();
        if (projected.empty())
        {
            stats.liveRejected = static_cast<unsigned>(snapshot.size());
            LogRender(diagnostic, "no-live-projection", static_cast<unsigned>(snapshot.size()), 0,
                scene, desc, first, stats);
            return;
        }
        Geometry::Palette palette = {};
        if (!ReadPalette(ta, palette))
        {
            LogRender(diagnostic, "palette-unavailable", static_cast<unsigned>(snapshot.size()),
                static_cast<unsigned>(projected.size()), scene, desc, first, stats);
            return;
        }
        Geometry::BlendTable blend = {};
        try { blend = PaletteBlend(palette); }
        catch (...)
        {
            LogRender(diagnostic, "palette-cache-fault", static_cast<unsigned>(snapshot.size()),
                static_cast<unsigned>(projected.size()), scene, desc, first, stats);
            return;
        }
        desc.dwSize = sizeof(desc);
        if (!LockSurface(surface, desc))
        {
            LogRender(diagnostic, "lock-failed", static_cast<unsigned>(snapshot.size()),
                static_cast<unsigned>(projected.size()), scene, desc, first, stats);
            return;
        }
        // Separate POD SEH envelopes avoid MSVC C2712 and always release a
        // successfully locked surface, including on a native rendering fault.
        PaintProtected(surface, desc, scene, projected, serial, stats, palette, blend);
        stats.liveRejected += static_cast<unsigned>(snapshot.size() - projected.size());
        LogRender(diagnostic, stats.stage, static_cast<unsigned>(snapshot.size()),
            static_cast<unsigned>(projected.size()), scene, desc, first, stats);
    }
}
#else
namespace MoveOrderFeedback
{
    void Record(TAdynmemStruct*, unsigned, const ActorIdentity&, const _Position_Dword&, const _Position_Dword&) {}
    void Cancel(TAdynmemStruct*, unsigned, const _Position_Dword&) {}
    void Draw(LPDIRECTDRAWSURFACE) {}
}
#endif
