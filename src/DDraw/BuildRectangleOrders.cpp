#include "BuildRectangleOrders.h"

#include "tamem.h"
#include "tafunctions.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace
{
    const unsigned kSelected = 0x10;
    const unsigned kAlive = 0x10000000;
    const unsigned kDying = 0x4000;
    const unsigned kNativeCanMove = 0x80;
    const unsigned kMaxOrders = 8192;

    struct Point { int x, y; };
    struct Bounds { int left, top, right, bottom; };
    struct UnitRef
    {
        UnitStruct* unit;
        short type;
        UnitDefStruct* definition;
        bool live;
    };
    struct Retreat
    {
        UnitRef ref;
        int footX, footY;
        std::vector<Point> existingMoves;
        Position_Dword destination;
        Bounds bounds;
    };
    struct Context
    {
        TAdynmemStruct* world;
        PlayerStruct* owner;
        UnitStruct* first;
        UnitStruct* last;
        int player;
        std::vector<UnitRef> selection;
        std::vector<Retreat> retreats;
        Position_Dword mouse;
        unsigned char prepared, cursor;
        short buildUnit;
        char buildState;
    };

    TAdynmemStruct* World()
    {
        return *reinterpret_cast<TAdynmemStruct**>(0x00511DE8u);
    }

    bool LiveOwned(const UnitStruct& unit, const PlayerStruct& owner)
    {
        return unit.UnitID != 0 && unit.Owner_PlayerPtr0 == &owner && unit.UnitType
            && (unit.UnitSelected & kAlive) && !(unit.UnitSelected & kDying);
    }

    UnitRef Capture(UnitStruct& unit, const PlayerStruct& owner)
    {
        return {&unit, unit.UnitID, unit.UnitType, LiveOwned(unit, owner)};
    }

    bool SameUnit(const UnitRef& ref, const PlayerStruct& owner)
    {
        // UnitID is a type, not a generation. Commands run on the UI thread;
        // nevertheless do not restore dead, transferred or repurposed slots.
        return ref.live && LiveOwned(*ref.unit, owner) && ref.unit->UnitID == ref.type
            && ref.unit->UnitType == ref.definition;
    }

    bool GroundBuilder(const UnitRef& ref)
    {
        return ref.live && ref.unit->IsUnit && ref.unit->Nanoframe == 0.0f
            && ref.definition->bmcode != 0
            && (ref.definition->UnitTypeMask_0 & builder)
            && (ref.definition->UnitTypeMask_0 & canmove)
            && !(ref.definition->UnitTypeMask_0 & canfly);
    }

    bool SameContext(const Context& context)
    {
        return World() == context.world
            && context.world->LocalHumanPlayer_PlayerID == context.player
            && context.owner->Units == context.first && context.owner->UnitsAry_End == context.last;
    }

    void ClearSelection(UnitStruct* first, UnitStruct* last)
    {
        // Match the engine dispatcher's inclusive range, including stale free slots.
        for (UnitStruct* unit = first; unit <= last; ++unit)
            unit->UnitSelected &= ~kSelected;
    }

    bool IsMove(const UnitOrdersStruct& order, unsigned handlerCount)
    {
        return order.COBHandler_index < handlerCount
            && (*COBSciptHandler_Begin)[order.COBHandler_index].cursorIndex == cursormove;
    }

    bool CaptureMoves(Retreat& retreat, unsigned handlerCount)
    {
        unsigned scanned = 0;
        for (const UnitOrdersStruct* order = retreat.ref.unit->UnitOrders; order; order = order->NextOrder)
        {
            if (++scanned > kMaxOrders || order->COBHandler_index >= handlerCount) return false;
            if (IsMove(*order, handlerCount))
                retreat.existingMoves.push_back({order->Pos.X, order->Pos.Y});
        }
        return true;
    }

    int FloorGrid(int value)
    {
        return value >= 0 ? value / 16 : -((-value + 15) / 16);
    }

    int SnapCentre(int value, int footprint, int direction)
    {
        const int phase = footprint * 8;
        // Normal coordinates round outward. Tangent coordinates use TA's
        // nearest-cell rule. Exact phase alignment prevents a later MOVE snap
        // from consuming the clearance that was just checked.
        const int adjusted = value - phase;
        const int tile = direction > 0 ? -FloorGrid(-adjusted)
            : direction < 0 ? FloorGrid(adjusted) : FloorGrid(adjusted + 8);
        return tile * 16 + phase;
    }

    bool Overlap(const Bounds& a, const Bounds& b)
    {
        return a.left < b.right && a.right > b.left && a.top < b.bottom && a.bottom > b.top;
    }

    bool SelectedBuilderOccupant(unsigned index, const Context& context)
    {
        for (const Retreat& retreat : context.retreats)
            if (static_cast<unsigned short>(retreat.ref.unit->UnitInGameIndex) == index) return true;
        return false;
    }

    bool ClearFootprint(const Context& context, const Retreat& retreat, int x, int y, Bounds& bounds)
    {
        const TAdynmemStruct& world = *context.world;
        bounds = {x - retreat.footX * 8, y - retreat.footY * 8,
            x + retreat.footX * 8, y + retreat.footY * 8};
        // Keep the full constructor inside the normal two-column/eight-row
        // playable border, and inside any tighter map-specified world limits.
        // Never clamp an outside retreat back into the completed strip.
        const int limitX = std::min(world.FeatureMapSizeX - 2, world.MapSizeX / 16);
        const int limitY = std::min(world.FeatureMapSizeY - 8, world.MapSizeY / 16);
        if (bounds.left < 0 || bounds.top < 0 || bounds.right > limitX * 16
            || bounds.bottom > limitY * 16 || x > 32767 || y > 32767) return false;
        for (int row = bounds.top / 16; row < bounds.bottom / 16; ++row)
            for (int column = bounds.left / 16; column < bounds.right / 16; ++column)
            {
                FeatureStruct& tile = world.FeatureMap[column + row * world.FeatureMapSizeX];
                if (tile.occupyingUnitNumber && !SelectedBuilderOccupant(tile.occupyingUnitNumber, context))
                    return false;
                const unsigned feature = GetGridPosFeature(&tile);
                if (feature < static_cast<unsigned>(world.NumFeatureDefs)
                    && world.FeatureDef[feature].Height != 0) return false;
                if (static_cast<int>(tile.maxHeight2x2) - tile.minHeight2x2 > retreat.ref.definition->maxslope)
                    return false;
                if (!(retreat.ref.definition->UnitTypeMask_0 & canhover))
                {
                    const int depth = std::max(0, static_cast<int>(world.SeaLevel) - tile.height);
                    if (depth < retreat.ref.definition->minwaterdepth || depth > retreat.ref.definition->maxwaterdepth)
                        return false;
                }
            }
        return true;
    }

    bool Candidate(Context& context, std::size_t index, int x, int y,
        int targetX, int targetY, bool finishing)
    {
        Retreat& retreat = context.retreats[index];
        const std::int64_t dx = std::int64_t(x) - targetX, dy = std::int64_t(y) - targetY;
        const std::int64_t range = retreat.ref.definition->builddistance;
        if (!finishing && dx * dx + dy * dy > range * range) return false;
        // Native Shift-click scans the entire main queue with inclusive +/-16
        // tolerance, so even a nearby MOVE would remove an existing command.
        for (const Point& move : retreat.existingMoves)
            if (std::abs(x - move.x) <= 16 && std::abs(y - move.y) <= 16) return false;
        Bounds bounds;
        if (!ClearFootprint(context, retreat, x, y, bounds)) return false;
        for (std::size_t prior = 0; prior < index; ++prior)
            if (Overlap(bounds, context.retreats[prior].bounds)) return false;
        retreat.destination = {};
        retreat.destination.X = static_cast<unsigned short>(x);
        retreat.destination.Y = static_cast<unsigned short>(y);
        const int height = GetPosHeight(&retreat.destination);
        retreat.destination.Z = static_cast<unsigned short>(height < 0 ? context.world->SeaLevel : height);
        retreat.bounds = bounds;
        return true;
    }

    bool FindDestination(Context& context, std::size_t index, int targetX, int targetY,
        int outwardX, int outwardY, int buildingFootX, int buildingFootY, bool finishing)
    {
        const Retreat& retreat = context.retreats[index];
        const bool horizontal = outwardX != 0;
        const int direction = horizontal ? outwardX : outwardY;
        const int normalFoot = horizontal ? retreat.footX : retreat.footY;
        const int tangentFoot = horizontal ? retreat.footY : retreat.footX;
        const int targetNormal = horizontal ? targetX : targetY;
        const int targetTangent = horizontal ? targetY : targetX;
        const int buildingNormal = horizontal ? buildingFootX : buildingFootY;
        const int buildingTangent = horizontal ? buildingFootY : buildingFootX;
        const int offset = (buildingNormal + normalFoot) * 8 + 32;
        const int normal = SnapCentre(targetNormal + direction * offset, normalFoot, direction);
        const int currentTangent = horizontal ? retreat.ref.unit->YPos : retreat.ref.unit->XPos;
        const int preferred = finishing ? targetTangent
            : std::max(targetTangent - buildingTangent * 8,
                std::min(currentTangent, targetTangent + buildingTangent * 8));
        const int tangent = SnapCentre(preferred, tangentFoot, 0);
        const int extent = horizontal ? context.world->MapSizeY : context.world->MapSizeX;
        // Visit the closest tangent first, then both directions along the clear
        // exterior band. A 32-unit stride also separates ordinary MOVE clicks.
        for (int distance = 0; distance <= extent + 32; distance += 32)
            for (int side = 0; side < (distance == 0 ? 1 : 2); ++side)
            {
                const int candidateTangent = tangent + (side == 0 ? distance : -distance);
                const int x = horizontal ? normal : candidateTangent;
                const int y = horizontal ? candidateTangent : normal;
                if (Candidate(context, index, x, y, targetX, targetY, finishing)) return true;
            }
        return false;
    }

    void Restore(Context& context)
    {
        if (!SameContext(context)) return;
        ClearSelection(context.first, context.last);
        for (const UnitRef& ref : context.selection)
            if (SameUnit(ref, *context.owner)) ref.unit->UnitSelected |= kSelected;
        context.world->MouseMapPos = context.mouse;
        context.world->PrepareOrder_Type = context.prepared;
        context.world->CurrentCursora_Index = context.cursor;
        context.world->BuildUnitID = context.buildUnit;
        context.world->BuildSpotState = context.buildState;
        UpdateSelectUnitEffect();
    }

    bool Dispatch(Context& context)
    {
        // Keep STL objects in the caller: MSVC forbids their destructors in a
        // function containing SEH. Always restore the player's selection/UI.
        __try
        {
            for (std::size_t index = 0; index < context.retreats.size(); ++index)
            {
                if (!SameContext(context)) return false;
                const Retreat& retreat = context.retreats[index];
                if (!SameUnit(retreat.ref, *context.owner)) return false;
                ClearSelection(context.first, context.last);
                retreat.ref.unit->UnitSelected |= kSelected;
                UpdateSelectUnitEffect();
                MOUSEEVENT event = {};
                event.fwKeys = 4;
                context.world->MouseMapPos = retreat.destination;
                // Explicit normal UI dispatch avoids TAMapClick's green/red
                // build-cursor branch and prevents group formation offsets.
                MOUSE_EVENT_2UnitOrder_(&event, ordertype::MOVE, 0,
                    &context.world->MouseMapPos, 0, 0);
            }
            return true;
        }
        __finally
        {
            Restore(context);
        }
    }
}

bool BuildRectangleOrders::QueueRetreat(int targetX, int targetY, int outwardX, int outwardY,
    int buildingFootX, int buildingFootY, bool finishing)
{
    if (!((outwardY == 0 && (outwardX == -1 || outwardX == 1))
        || (outwardX == 0 && (outwardY == -1 || outwardY == 1)))
        || targetX < 0 || targetX > 32767 || targetY < 0 || targetY > 32767
        || buildingFootX <= 0 || buildingFootX > 2048 || buildingFootY <= 0 || buildingFootY > 2048)
        return false;
    TAdynmemStruct* world = World();
    if (!world || world->LocalHumanPlayer_PlayerID < 0 || world->LocalHumanPlayer_PlayerID >= 10)
        return false;
    PlayerStruct* owner = &world->Players[world->LocalHumanPlayer_PlayerID];
    if (owner->My_PlayerType != Player_LocalHuman || !owner->PlayerInfo
        || (owner->PlayerInfo->PropertyMask & WATCH) || !owner->Units || !owner->UnitsAry_End
        || !world->BeginUnitsArray_p || !world->EndOfUnitsArray_p) return false;
    const std::uintptr_t first = reinterpret_cast<std::uintptr_t>(owner->Units);
    const std::uintptr_t last = reinterpret_cast<std::uintptr_t>(owner->UnitsAry_End);
    const std::uintptr_t begin = reinterpret_cast<std::uintptr_t>(world->BeginUnitsArray_p);
    const std::uintptr_t end = reinterpret_cast<std::uintptr_t>(world->EndOfUnitsArray_p);
    if (first < begin || last < first || last >= end || (last - first) % sizeof(UnitStruct)
        || (last - first) / sizeof(UnitStruct) >= 65536) return false;

    Context context = {};
    context.world = world;
    context.owner = owner;
    context.first = owner->Units;
    context.last = owner->UnitsAry_End;
    context.player = world->LocalHumanPlayer_PlayerID;
    context.mouse = world->MouseMapPos;
    context.prepared = world->PrepareOrder_Type;
    context.cursor = world->CurrentCursora_Index;
    context.buildUnit = world->BuildUnitID;
    context.buildState = world->BuildSpotState;
    for (UnitStruct* unit = context.first; unit <= context.last; ++unit)
    {
        if (!(unit->UnitSelected & kSelected)) continue;
        const UnitRef ref = Capture(*unit, *owner);
        context.selection.push_back(ref);
        if (GroundBuilder(ref))
        {
            if (!(ref.definition->UnitTypeMask_1 & kNativeCanMove)
                || unit->SizeFootX <= 0 || unit->SizeFootX > 2048
                || unit->SizeFootZ <= 0 || unit->SizeFootZ > 2048) return false;
            Retreat retreat = {};
            retreat.ref = ref;
            retreat.footX = unit->SizeFootX;
            retreat.footY = unit->SizeFootZ;
            context.retreats.push_back(retreat);
        }
    }
    if (context.retreats.empty()) return true;
    if (!world->FeatureMap || world->FeatureMapSizeX <= 2 || world->FeatureMapSizeY <= 8
        || world->MapSizeX <= 0 || world->MapSizeX > 32768
        || world->MapSizeY <= 0 || world->MapSizeY > 32768
        || world->NumFeatureDefs < 0 || (world->NumFeatureDefs && !world->FeatureDef)
        || !GetGridPosFeature || !GetPosHeight || !MOUSE_EVENT_2UnitOrder_
        || !COBSciptHandler_Begin || !COBSciptHandler_End
        || !*COBSciptHandler_Begin || !*COBSciptHandler_End) return false;
    const std::ptrdiff_t handlers = *COBSciptHandler_End - *COBSciptHandler_Begin;
    if (handlers <= 0 || handlers > 256) return false;
    for (std::size_t index = 0; index < context.retreats.size(); ++index)
        if (!CaptureMoves(context.retreats[index], static_cast<unsigned>(handlers))
            || !FindDestination(context, index, targetX, targetY, outwardX, outwardY,
                buildingFootX, buildingFootY, finishing)) return false;
    return Dispatch(context);
}
