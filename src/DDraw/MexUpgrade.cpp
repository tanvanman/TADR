#include "config.h"
#include "MexUpgrade.h"

#if MEX_UPGRADE_ENABLE
#include "MexUpgradeMenu.h"
#include "oddraw.h"
#include "iddrawsurface.h"
#include "tamem.h"
#include "tafunctions.h"
#include "taHPI.h"
#include "TAConfig.h"
#include "Gaf.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

namespace
{
    constexpr unsigned kDefinitionEnabled = 0x00800000;
    constexpr unsigned kMaxOrders = 8192;
    constexpr int kMaxUpgradeFootprint = 32;
    constexpr unsigned kFeedbackTicks = 45;
    // Cursor probes and build input run on TA's main thread.
    static bool checkingSite = false;

    struct SiteProbe
    {
        // TestGridSpot uses two shared scratch values. An upgrade probe runs
        // in the input handler and should leave the native build preview alone.
        DWORD* const scratch = reinterpret_cast<DWORD*>(0x0051E684u);
        const DWORD height = scratch[0];
        const DWORD flags = scratch[1];
        const bool previous = checkingSite;

        SiteProbe() { checkingSite = true; }
        ~SiteProbe()
        {
            scratch[0] = height;
            scratch[1] = flags;
            checkingSite = previous;
        }
    };

    struct UpgradeCursor
    {
        unsigned char pixels[13 * 14];
        GAFFrame frame = {};
        GAFSequence sequence = {};

        explicit UpgradeCursor(bool blocked)
        {
            static constexpr char arrow[] =
                "......#......"
                ".....#+#....."
                "....#+++#...."
                "...#+++++#..."
                "..#+++++++#.."
                ".#+++++++++#."
                "##+++++++++##"
                "..###+++###.."
                "....#+++#...."
                "....#+++#...."
                "....#+++#...."
                "....#+++#...."
                "....#+++#...."
                "....#####....";
            static constexpr char cross[] =
                "##.........##"
                "#++#.....#++#"
                ".#++#...#++#."
                "..#++#.#++#.."
                "...#+++++#..."
                "....#+++#...."
                ".....#+#....."
                "....#+++#...."
                "...#+++++#..."
                "..#++#.#++#.."
                ".#++#...#++#."
                "#++#.....#++#"
                "##.........##"
                ".............";
            static_assert(sizeof(arrow) == sizeof(pixels) + 1, "Upgrade arrow dimensions");
            static_assert(sizeof(cross) == sizeof(pixels) + 1, "Blocked upgrade cursor dimensions");
            const char* art = blocked ? cross : arrow;
            for (size_t i = 0; i < sizeof(pixels); ++i)
                // Use the same red/green palette ramps as TA's stock cursors.
                pixels[i] = art[i] == '+' ? (blocked ? 211 : 232) : art[i] == '#' ? 142 : 9;
            // Use the arrow tip or the X center as the hotspot. Index 9 is transparent.
            frame = {13, 14, 6, static_cast<unsigned short>(blocked ? 6 : 0), 9, 0, 0, 0, pixels, 0};
            sequence.Frames = 1;
            sequence.Signature = 1;
            sequence.PtrFrameAry[0] = {&frame, 10};
        }
    };

    struct Upgrade
    {
        UnitStruct* constructor;
        UnitStruct* target;
        const char* replacement;
        int type;
        Position_Dword buildPos;
    };

    // Keep a description of the order, not a pointer to an engine allocation.
    struct Feedback
    {
        const TAdynmemStruct* world;
        unsigned constructorSlot;
        short constructorType;
        unsigned action;
        unsigned start;
        int type;
        Position_Dword position;
    } feedback = {};

    struct UpgradePair
    {
        const char* from;
        const char* to;
    };
    constexpr UpgradePair kUpgrades[] = {
        {"ARMMEX", "ARMMOHO"}, {"CORMEX", "CORMOHO"},
        {"ARMUWMEX", "ARMUWMOHO"}, {"CORUWMEX", "CORUWMOHO"},
        {"ARMMOHO", "ARMMAS"}, {"CORMOHO", "CORMAS"},
        {"ARMUWMOHO", "ARMUWMAS"}, {"CORUWMOHO", "CORUWMAS"},
    };

    bool LiveOwned(const UnitStruct& unit, const PlayerStruct& owner)
    {
        // IsUnit is the movement object pointer. Buildings can be live without it.
        return unit.UnitID && unit.UnitType && unit.Owner_PlayerPtr0 == &owner
            && (unit.UnitSelected & 0x10000000) && !(unit.UnitSelected & 0x4000);
    }

    UnitStruct* SingleSelectedUnit(PlayerStruct& owner)
    {
        UnitStruct* selected = nullptr;
        for (UnitStruct* unit = owner.Units; unit <= owner.UnitsAry_End; ++unit)
        {
            if (!(unit->UnitSelected & UnitSelected_State) || !LiveOwned(*unit, owner)) continue;
            if (selected) return nullptr;
            selected = unit;
        }
        return selected;
    }

    const char* ReplacementName(const UnitDefStruct& oldDef)
    {
        for (const auto& pair : kUpgrades)
            if (_strnicmp(oldDef.UnitName, pair.from, sizeof(oldDef.UnitName)) == 0)
                return pair.to;
        return nullptr;
    }

    const char* BuildMenuDirectory(const TAdynmemStruct& ta)
    {
        // GUI initialization (0x4914C2 -> 0x49FBA0) stores the mod's directory
        // here, with a trailing slash; GUI_Load reads it at 0x4AA989.
        static_assert(sizeof(ta.desktopGUI) >= 0x9B6 + 256, "GUI directory layout changed");
        return reinterpret_cast<const char*>(&ta.desktopGUI) + 0x9B6;
    }

    bool MenuFileContains(const std::string& path, const char* name)
    {
        char filename[260];
        if (path.size() >= sizeof(filename)) return false;
        std::memcpy(filename, path.c_str(), path.size() + 1);
        unsigned size = 0;
        char* data = readfile_HPI(filename, &size);
        if (!data) return false;
        const bool found = size <= 1024 * 1024
            && MexUpgradeMenu::ContainsBuildButton(data, size, name);
        freeTAMem(data);
        return found;
    }

    bool HasBuildMenuEntry(const TAdynmemStruct& ta, const UnitDefStruct& constructor,
        const char* name)
    {
        const char* directory = BuildMenuDirectory(ta);
        if (!directory || !constructor.GUINum) return false;
        size_t directoryLength = 0;
        while (directoryLength < 256 && directory[directoryLength]) ++directoryLength;
        if (!directoryLength || directoryLength == 256) return false;
        char unitName[sizeof(constructor.UnitName) + 1] = {};
        std::memcpy(unitName, constructor.UnitName, sizeof(constructor.UnitName));
        if (!unitName[0]) return false;

        // Cache only file names and answers, never live unit pointers. These
        // menus are immutable during a match; invalidate when its definitions
        // or world change, or the game clock restarts after returning to menu.
        static const TAdynmemStruct* world = nullptr;
        static const UnitDefStruct* definitions = nullptr;
        static unsigned lastTick = 0;
        static std::map<std::string, bool> menus;
        const unsigned tick = static_cast<unsigned>(ta.GameTime);
        if (world != &ta || definitions != ta.UnitDef || tick < lastTick)
        {
            menus.clear();
            world = &ta;
            definitions = ta.UnitDef;
        }
        lastTick = tick;
        std::string prefix(directory, directoryLength);
        if (prefix.back() != '\\' && prefix.back() != '/') prefix += '\\';
        prefix += unitName;
        const std::string key = prefix + ":" + std::to_string(constructor.GUINum)
            + ":" + std::to_string(constructor.UnitTypeMask_0 >> 31) + ":" + name;
        const auto cached = menus.find(key);
        if (cached != menus.end()) return cached->second;

        // The engine probes UnitName0.GUI separately, then records the first
        // missing numbered page in GUINum (0x42D80E through 0x42D8CB).
        // Its page-zero flag is also required by GUI selection (0x41B794).
        bool found = false;
        for (unsigned page = 0; page < constructor.GUINum && !found; ++page)
            if (page || (constructor.UnitTypeMask_0 & 0x80000000u))
                found = MenuFileContains(prefix + std::to_string(page) + ".GUI", name);
        if (menus.size() >= 512) menus.clear();
        menus.emplace(key, found);
        return found;
    }

    int FindReplacement(const TAdynmemStruct& ta, const UnitDefStruct& oldDef,
        const UnitDefStruct& constructor, const char* name)
    {
        // CANBUILD_ptr is the AI's construction plan, not the player's build
        // menu. Escalation intentionally excludes several human mex options
        // from that plan, including advanced construction vehicles.
        for (unsigned id = 1; id < ta.UNITINFOCount; ++id)
        {
            const UnitDefStruct& def = ta.UnitDef[id];
            if (_strnicmp(def.UnitName, name, sizeof(def.UnitName)) == 0
                && def.bmcode == 0 && def.extractsmetal > oldDef.extractsmetal
                && (def.UnitTypeMask_0 & kDefinitionEnabled)
                && HasBuildMenuEntry(ta, constructor, name)) return id;
        }
        return 0;
    }

    bool UnderBuildLimit(const PlayerStruct& owner, int type, int limit)
    {
        if (limit == -1) return true;
        if (limit <= 0) return false;
        // Native unit creation counts all occupied slots of this type, including nanoframes.
        for (const UnitStruct* unit = owner.Units; unit <= owner.UnitsAry_End; ++unit)
            if (unit->UnitID == type && --limit == 0) return false;
        return true;
    }

    bool ReplacementTile(const UnitStruct& old, const UnitDefStruct& def, int& left, int& top)
    {
        if (def.FootX <= 0 || def.FootY <= 0
            || def.FootX > kMaxUpgradeFootprint || def.FootY > kMaxUpgradeFootprint) return false;
        // Same grid rounding as GUI_IssueMobileBuildOrder, starting from the old center.
        const int x = static_cast<int>(old.XPos) + 8 - def.FootX * 8;
        const int y = static_cast<int>(old.YPos) + 8 - def.FootY * 8;
        if (x < 0 || y < 0) return false;
        left = x / 16;
        top = y / 16;
        return true;
    }

    bool ReplacementContainsTarget(const UnitStruct& old, const UnitDefStruct& def, int left, int top)
    {
        return def.FootX > 0 && def.FootY > 0
            && def.FootX <= kMaxUpgradeFootprint && def.FootY <= kMaxUpgradeFootprint
            && old.SizeFootX > 0 && old.SizeFootZ > 0
            && old.SizeFootX <= def.FootX && old.SizeFootZ <= def.FootY
            && old.XGridPos >= 0 && old.YGridPos >= 0 && left >= 0 && top >= 0
            && left <= old.XGridPos && top <= old.YGridPos
            && left + def.FootX >= old.XGridPos + old.SizeFootX
            && top + def.FootY >= old.YGridPos + old.SizeFootZ;
    }

    bool CheckReplacementTile(const TAdynmemStruct& ta, PlayerStruct& owner,
        const UnitStruct& old, unsigned slot, const UnitDefStruct& def, int left, int top,
        Position_Dword& position, unsigned& blocker)
    {
        if (!def.YardMap || def.FootX <= 0 || def.FootY <= 0
            || def.FootX > kMaxUpgradeFootprint || def.FootY > kMaxUpgradeFootprint
            || left < 0 || top < 0
            || left + def.FootX > ta.FeatureMapSizeX || top + def.FootY > ta.FeatureMapSizeY)
            return false;

        // TestGridSpot can ignore one occupant by slot. Its earlier yard check also
        // sees the old building's yard reservation, so mask that bit on a private
        // copy only where the clicked building occupies the proposed footprint.
        char yard[kMaxUpgradeFootprint * kMaxUpgradeFootprint] = {};
        const size_t yardSize = static_cast<size_t>(def.FootX) * def.FootY;
        std::memcpy(yard, def.YardMap, yardSize);
        for (int row = 0; row < def.FootY; ++row)
        {
            for (int column = 0; column < def.FootX; ++column)
            {
                const int tileX = left + column, tileY = top + row;
                const auto occupant = ta.FeatureMap[tileY * ta.FeatureMapSizeX + tileX].occupyingUnitNumber;
                // The native preview can omit occupancies in fog. Never reclaim
                // the extractor if a different ground occupant already blocks it.
                if (occupant && occupant != slot)
                {
                    blocker = occupant;
                    return false;
                }
                if (occupant == slot && tileX >= old.XGridPos && tileX < old.XGridPos + old.SizeFootX
                    && tileY >= old.YGridPos && tileY < old.YGridPos + old.SizeFootZ
                    && (yard[row * def.FootX + column] & 0x01))
                    yard[row * def.FootX + column] &= ~0x01;
            }
        }
        UnitDefStruct preview = def;
        preview.YardMap = yard;
        const unsigned packed = (static_cast<unsigned>(top) << 16) | static_cast<unsigned>(left);
        const SiteProbe probe;
        if (!TestGridSpot(&preview, packed, slot, &owner)) return false;

        // TestGridSpot publishes the validated terrain height used by native build orders.
        const auto gridHeight = reinterpret_cast<int (__cdecl*)()>(0x0047C780u);
        position.X = static_cast<unsigned short>(left * 16 + def.FootX * 8);
        position.Y = static_cast<unsigned short>(top * 16 + def.FootY * 8);
        position.Z = static_cast<unsigned short>(gridHeight());
        return true;
    }

    bool CheckReplacementSite(const TAdynmemStruct& ta, PlayerStruct& owner,
        const UnitStruct& old, unsigned slot, const UnitDefStruct& def, Position_Dword& position, unsigned& blocker)
    {
        // TA extracts from the footprint's metal cells. Cover every cell used by
        // the old extractor, even when moving the replacement around a windmill.
        if (!def.YardMap || !ReplacementContainsTarget(old, def, old.XGridPos, old.YGridPos))
            return false;

        const int centeredLeft = (static_cast<int>(old.XPos) + 8 - def.FootX * 8) / 16;
        const int centeredTop = (static_cast<int>(old.YPos) + 8 - def.FootY * 8) / 16;
        int left, top;
        const bool centered = ReplacementTile(old, def, left, top)
            && ReplacementContainsTarget(old, def, left, top);
        if (centered
            && CheckReplacementTile(ta, owner, old, slot, def, left, top, position, blocker))
            return true;

        struct Site { int left, top, distance; };
        Site sites[kMaxUpgradeFootprint * kMaxUpgradeFootprint];
        unsigned count = 0;
        for (top = old.YGridPos + old.SizeFootZ - def.FootY; top <= old.YGridPos; ++top)
        {
            for (left = old.XGridPos + old.SizeFootX - def.FootX; left <= old.XGridPos; ++left)
            {
                if ((centered && left == centeredLeft && top == centeredTop) || left < 0 || top < 0
                    || left + def.FootX > ta.FeatureMapSizeX || top + def.FootY > ta.FeatureMapSizeY)
                    continue;
                const int dx = left - centeredLeft, dy = top - centeredTop;
                sites[count++] = {left, top, dx * dx + dy * dy};
            }
        }
        std::sort(sites, sites + count, [](const Site& a, const Site& b) {
            if (a.distance != b.distance) return a.distance < b.distance;
            if (a.top != b.top) return a.top < b.top;
            return a.left < b.left;
        });
        for (unsigned i = 0; i < count; ++i)
            if (CheckReplacementTile(ta, owner, old, slot, def,
                sites[i].left, sites[i].top, position, blocker)) return true;
        return false;
    }

    bool CanReclaim(UnitStruct* constructor, UnitStruct* target)
    {
        const auto canReclaim = reinterpret_cast<int (__thiscall*)(UnitStruct*, UnitStruct*)>(0x00489960u);
        return canReclaim(constructor, target) != 0;
    }

    bool FindUpgradeTarget(TAdynmemStruct* ta, Upgrade& upgrade)
    {
        if (!ta || !DataShare || DataShare->TAProgress != TAInGame || DataShare->PlayingDemo
            || ta->PrepareOrder_Type != ordertype::STOP || (ta->BuildSpotState & 10) != 2) return false;
        if (ta->InterfaceType != 0 && (ta->InterfaceType != 1 || !(ta->BuildSpotState & 4))) return false;

        const int player = ta->LocalHumanPlayer_PlayerID;
        if (player < 0 || player >= 10 || !ta->UnitDef || !ta->FeatureMap
            || !ta->BeginUnitsArray_p || !ta->EndOfUnitsArray_p
            || ta->EndOfUnitsArray_p < ta->BeginUnitsArray_p) return false;
        auto& owner = ta->Players[player];
        if (!owner.PlayerInfo || (owner.PlayerInfo->PropertyMask & WATCH) || owner.My_PlayerType != Player_LocalHuman
            || !owner.Units || !owner.UnitsAry_End || owner.Units < ta->BeginUnitsArray_p
            || owner.UnitsAry_End > ta->EndOfUnitsArray_p || owner.UnitsAry_End < owner.Units) return false;

        // Both global and per-player end pointers name the last valid record.
        // The final player's whole block may end at the global end pointer.
        const unsigned slot = ta->MouseOverUnit;
        if (!slot || slot > static_cast<size_t>(ta->EndOfUnitsArray_p - ta->BeginUnitsArray_p)) return false;
        auto& target = ta->BeginUnitsArray_p[slot];
        if (!LiveOwned(target, owner) || static_cast<unsigned short>(target.UnitInGameIndex) != slot
            || target.Nanoframe != 0.0f || target.UnitType->bmcode != 0 || target.UnitType->extractsmetal <= 0)
            return false;
        const char* replacement = ReplacementName(*target.UnitType);
        if (!replacement) return false;
        auto* constructor = SingleSelectedUnit(owner);
        if (!constructor || constructor == &target || !constructor->IsUnit || constructor->Nanoframe != 0.0f
            || !(constructor->UnitType->UnitTypeMask_0 & builder) || constructor->UnitType->bmcode == 0)
            return false;

        upgrade = {constructor, &target, replacement, 0, {}};
        return true;
    }

    bool PrepareUpgrade(TAdynmemStruct& ta, Upgrade& upgrade, bool reportBlocked)
    {
        auto& target = *upgrade.target;
        auto& constructor = *upgrade.constructor;
        auto& owner = *target.Owner_PlayerPtr0;
        const int type = FindReplacement(ta, *target.UnitType, *constructor.UnitType, upgrade.replacement);
        if (!type || !CanReclaim(&constructor, &target)) return false;
        const auto& def = ta.UnitDef[type];
        if (!UnderBuildLimit(owner, type, def.buildLimit)) return false;
        const unsigned slot = static_cast<unsigned short>(target.UnitInGameIndex);
        unsigned blocker = 0;
        if (!CheckReplacementSite(ta, owner, target, slot, def, upgrade.buildPos, blocker))
        {
            if (reportBlocked)
                IDDrawSurface::OutptFmtTxt("[MexUpgrade] replacement site blocked: builder=%u extractor=%u build=%d occupant=%u",
                    static_cast<unsigned short>(constructor.UnitInGameIndex), slot, type, blocker);
            return false;
        }
        upgrade.type = type;
        return true;
    }

    unsigned char ActionIndex(const char* name)
    {
        const auto lookup = reinterpret_cast<unsigned char* (__thiscall*)(unsigned char*, const char*)>(0x00438760u);
        unsigned char index = 0;
        lookup(&index, name);
        return index;
    }

    // ORDERS_NewMainOrder2Unit, also used by native GUI_IssueMobileBuildOrder.
    using Dispatch = void (__stdcall*)(unsigned, int, UnitStruct*, UnitStruct*, const Position_Dword*, int, int);
    const auto Send = reinterpret_cast<Dispatch>(0x0043AFC0u);
    const auto Remove = reinterpret_cast<void (__stdcall*)(UnitStruct*, UnitOrdersStruct*)>(0x00439F80u);

    struct Actions { unsigned reclaim, build; };

    Actions UpgradeActions(const UnitStruct& constructor)
    {
        const bool air = (constructor.UnitType->UnitTypeMask_0 & canfly) != 0;
        return {ActionIndex(air ? "VTOL_RECLAIMUNIT" : "RECLAIMUNIT"),
            ActionIndex(air ? "VTOL_MOBILEBUILD" : "MOBILEBUILD")};
    }

    UnitOrdersStruct* FindReclaim(const UnitStruct& constructor, unsigned action, const UnitStruct& target)
    {
        unsigned scanned = 0;
        for (auto* order = constructor.UnitOrders; order && scanned++ < kMaxOrders; order = order->NextOrder)
            if (order->COBHandler_index == action && order->AttackTargat == &target) return order;
        return nullptr;
    }

    bool IsBuildAt(const UnitOrdersStruct* order, unsigned action, int type, const Position_Dword& position)
    {
        return order && order->COBHandler_index == action && order->BuildUnitID == static_cast<unsigned>(type)
            && order->Pos.X == position.X && order->Pos.Y == position.Y && order->Pos.Z == position.Z;
    }

    bool IsReplacementBuild(const TAdynmemStruct& ta, const UnitOrdersStruct& build,
        const UnitStruct& target, const char* replacement)
    {
        if (!ta.UnitDef || !build.BuildUnitID || build.BuildUnitID >= ta.UNITINFOCount
            || build.State != 0 || build.AttackTargat) return false;
        const auto& def = ta.UnitDef[build.BuildUnitID];
        if (_strnicmp(def.UnitName, replacement, sizeof(def.UnitName)) != 0) return false;
        const int x = static_cast<int>(build.Pos.X) - def.FootX * 8;
        const int y = static_cast<int>(build.Pos.Y) - def.FootY * 8;
        if (x < 0 || y < 0 || x % 16 || y % 16 || build.Pos.x_ || build.Pos.y_) return false;
        const int left = x / 16, top = y / 16;
        return ReplacementContainsTarget(target, def, left, top)
            && left + def.FootX <= ta.FeatureMapSizeX && top + def.FootY <= ta.FeatureMapSizeY;
    }

    bool CancelUpgrade(TAdynmemStruct& ta, const Upgrade& upgrade, Actions actions)
    {
        auto& constructor = *upgrade.constructor;
        auto* reclaim = FindReclaim(constructor, actions.reclaim, *upgrade.target);
        auto* build = reclaim ? reclaim->NextOrder : nullptr;
        if (!build || reclaim->Unit_ptr != &constructor || build->Unit_ptr != &constructor
            || build->COBHandler_index != actions.build
            || !IsReplacementBuild(ta, *build, *upgrade.target, upgrade.replacement)) return false;

        // Remove the exact native nodes. The usual Shift toggle matches build
        // positions within one tile and could cancel an unrelated nearby order.
        Remove(&constructor, build);
        Remove(&constructor, reclaim);
        static unsigned cancelledLogs = 0;
        if (cancelledLogs < 32)
        {
            ++cancelledLogs;
            IDDrawSurface::OutptFmtTxt("[MexUpgrade] cancelled builder=%u extractor=%u",
                static_cast<unsigned short>(constructor.UnitInGameIndex),
                static_cast<unsigned short>(upgrade.target->UnitInGameIndex));
        }
        return true;
    }

    bool QueueUpgrade(TAdynmemStruct& ta, const Upgrade& upgrade, Actions actions, bool shift)
    {
        auto& constructor = *upgrade.constructor;
        auto& target = *upgrade.target;
        const int type = upgrade.type;
        const auto& buildPos = upgrade.buildPos;
        const unsigned reclaim = actions.reclaim, build = actions.build;

        // Shift may toggle an existing or nearby order. A plain click replaces
        // the queue, including when the clicked extractor was already queued.
        unsigned scanned = 0;
        for (auto* order = constructor.UnitOrders; order; order = order->NextOrder)
        {
            if (++scanned >= kMaxOrders) return false;
            if (!shift) continue;
            if ((order->COBHandler_index == reclaim && order->AttackTargat == &target)
                || (order->COBHandler_index == build
                    && std::abs(static_cast<int>(order->Pos.X) - buildPos.X) <= 16
                    && std::abs(static_cast<int>(order->Pos.Y) - buildPos.Y) <= 16)) return true;
        }

        Position_Dword reclaimPos = {};
        reclaimPos.X = target.XPos;
        reclaimPos.Y = target.YPos;
        reclaimPos.Z = target.ZPos;
        Send(reclaim, shift, &constructor, &target, &reclaimPos, 0, 0);
        auto* order = FindReclaim(constructor, reclaim, target);
        if (!order) return true;

        Send(build, 1, &constructor, nullptr, &buildPos, type, 0);
        order = FindReclaim(constructor, reclaim, target);
        if (order && IsBuildAt(order->NextOrder, build, type, buildPos))
        {
            feedback = {&ta, static_cast<unsigned short>(constructor.UnitInGameIndex),
                constructor.UnitID, build, order->NextOrder->StartTime, type, buildPos};
            static unsigned queuedLogs = 0;
            if (queuedLogs < 32)
            {
                ++queuedLogs;
                IDDrawSurface::OutptFmtTxt("[MexUpgrade] queued builder=%u extractor=%u build=%d shift=%d first=%d",
                    static_cast<unsigned short>(constructor.UnitInGameIndex),
                    static_cast<unsigned short>(target.UnitInGameIndex), type, shift, constructor.UnitOrders == order);
                if (!shift && constructor.UnitOrders && constructor.UnitOrders != order)
                {
                    const auto& head = *constructor.UnitOrders;
                    IDDrawSurface::OutptFmtTxt("[MexUpgrade] native order still ahead of replacement: action=%u state=%u flags=%08X",
                        static_cast<unsigned>(head.COBHandler_index), static_cast<unsigned>(head.State), head.Order_State);
                }
            }
            return true;
        }

        // Both commands run on the UI thread before the engine executes an order.
        // If the build didn't follow reclaim, remove the orders this click added.
        scanned = 0;
        for (auto* queued = constructor.UnitOrders; queued && scanned++ < kMaxOrders; queued = queued->NextOrder)
        {
            if (!IsBuildAt(queued, build, type, buildPos)) continue;
            Remove(&constructor, queued);
            break;
        }
        if (order) Remove(&constructor, order);
        IDDrawSurface::OutptTxt("[MexUpgrade] rejected incomplete order pair");
        return true;
    }
}

bool MexUpgrade::Enabled()
{
    if (!MyConfig) return false;
    static const bool enabled = [] {
        const bool active = MyConfig->GetIniBool("MexUpgrade", TRUE) != FALSE;
        if (active) IDDrawSurface::OutptTxt("[MexUpgrade] build=optional-cursors-v10");
        return active;
    }();
    return enabled;
}

bool MexUpgrade::CheckingSite()
{
    return checkingSite;
}

void MexUpgrade::UpdateCursor(TAdynmemStruct* ta)
{
    if (!ta) return;
    const unsigned index = ta->CurrentCursora_Index;
    if (index >= sizeof(ta->cursor_ary) / sizeof(ta->cursor_ary[0])) return;
    auto* stock = ta->cursor_ary[index];
    auto* displayed = reinterpret_cast<GAFSequence*>(ta->desktopGUI.Cursor_3);
    static UpgradeCursor available(false), blocked(true);
    // Respect custom cursors owned by other patches, such as wreck snapping.
    if (!stock || (displayed != stock && displayed != &available.sequence && displayed != &blocked.sequence)) return;
    Upgrade upgrade = {};
    auto* desired = stock;
    if (!(GetAsyncKeyState(VK_CONTROL) & 0x8000) && !(GetAsyncKeyState(VK_MENU) & 0x8000)
        && FindUpgradeTarget(ta, upgrade)
        && FindReplacement(*ta, *upgrade.target->UnitType, *upgrade.constructor->UnitType, upgrade.replacement))
        desired = PrepareUpgrade(*ta, upgrade, false) ? &available.sequence : &blocked.sequence;
    if (displayed != desired) SetUICursor(&ta->desktopGUI, desired);
}

void MexUpgrade::DrawFeedback(TAdynmemStruct* ta, _OFFSCREEN* surface)
{
    if (!feedback.world) return;
    if (!ta || ta != feedback.world || !DataShare || DataShare->TAProgress != TAInGame
        || DataShare->PlayingDemo || static_cast<unsigned>(ta->GameTime) - feedback.start >= kFeedbackTicks
        || !ta->BeginUnitsArray_p || !ta->EndOfUnitsArray_p || !ta->UnitDef
        || ta->EndOfUnitsArray_p < ta->BeginUnitsArray_p)
    {
        feedback = {};
        return;
    }
    // Native DrawGameScreen already draws the queue while this key is held.
    const auto keyDown = reinterpret_cast<int (__stdcall*)(int)>(0x004C1B80u);
    if (!surface || !surface->lpSurface || keyDown(0xF9)) return;

    const int player = ta->LocalHumanPlayer_PlayerID;
    if (player < 0 || player >= 10 || !feedback.constructorSlot
        || feedback.constructorSlot > static_cast<size_t>(ta->EndOfUnitsArray_p - ta->BeginUnitsArray_p)
        || static_cast<unsigned>(feedback.type) >= ta->UNITINFOCount) return;
    auto& constructor = ta->BeginUnitsArray_p[feedback.constructorSlot];
    if (!LiveOwned(constructor, ta->Players[player]) || constructor.UnitID != feedback.constructorType
        || !(constructor.UnitSelected & UnitSelected_State)) return;

    // Five stack arguments. The native renderer computes the 10-tick expansion
    // from StartTime and uses the selected builder's own palette colors.
    using DrawBuildSpotQueue = void (__stdcall*)(_OFFSCREEN*, const void*, UnitOrdersStruct*, Position_Dword*, int);
    const auto draw = reinterpret_cast<DrawBuildSpotQueue>(0x00438C00u);
    unsigned scanned = 0;
    for (auto* order = constructor.UnitOrders; order && scanned++ < kMaxOrders; order = order->NextOrder)
    {
        if (order->Unit_ptr != &constructor || order->StartTime != feedback.start
            || !IsBuildAt(order, feedback.action, feedback.type, feedback.position)) continue;
        Position_Dword out = {};
        draw(surface, &ta->CameraToUnit, order, &out, 0);
        return;
    }
    feedback = {};
}

bool MexUpgrade::IsQueuedBuild(const TAdynmemStruct* ta, const UnitStruct* constructor,
    const UnitOrdersStruct* preceding)
{
    if (!preceding || !preceding->AttackTargat || !preceding->NextOrder
        || !preceding->NextOrder->BuildUnitID || !ta || !ta->BeginUnitsArray_p || !ta->EndOfUnitsArray_p
        || ta->EndOfUnitsArray_p < ta->BeginUnitsArray_p
        || !constructor || preceding->Unit_ptr != constructor || preceding->NextOrder->Unit_ptr != constructor
        || !constructor->UnitType || !constructor->Owner_PlayerPtr0 || !Enabled()) return false;
    const auto* target = preceding->AttackTargat;
    if (target < ta->BeginUnitsArray_p || target > ta->EndOfUnitsArray_p
        || !LiveOwned(*target, *constructor->Owner_PlayerPtr0)) return false;
    const auto* replacement = ReplacementName(*target->UnitType);
    if (!replacement) return false;
    const auto actions = UpgradeActions(*constructor);
    return actions.reclaim && actions.build && preceding->COBHandler_index == actions.reclaim
        && preceding->NextOrder->COBHandler_index == actions.build
        && IsReplacementBuild(*ta, *preceding->NextOrder, *target, replacement);
}

bool MexUpgrade::TryCommand(TAdynmemStruct* ta, const _MOUSEEVENT* mouse)
{
    if (!Enabled() || !ta || !mouse || (mouse->fwKeys & MK_CONTROL)
        || (GetAsyncKeyState(VK_MENU) & 0x8000)) return false;
    const bool shift = (mouse->fwKeys & MK_SHIFT) != 0;
    // An eligible extractor takes precedence over additive selection. All
    // other clicks fall through to the stock selection or order handler.
    const bool leftCommand = ta->InterfaceType == 0 && mouse->Msg == WM_LBUTTONUP && !(mouse->fwKeys & MK_RBUTTON);
    const bool rightCommand = ta->InterfaceType == 1 && mouse->Msg == WM_RBUTTONDOWN
        && !(mouse->fwKeys & MK_LBUTTON);
    if (!leftCommand && !rightCommand) return false;

    Upgrade upgrade = {};
    if (!FindUpgradeTarget(ta, upgrade)) return false;
    const auto actions = UpgradeActions(*upgrade.constructor);
    if (!actions.reclaim || !actions.build) return false;
    // Existing orders can still be cancelled if the site is now blocked.
    if (shift && CancelUpgrade(*ta, upgrade, actions)) return true;
    return PrepareUpgrade(*ta, upgrade, true) && QueueUpgrade(*ta, upgrade, actions, shift);
}

#else

namespace MexUpgrade
{
    bool Enabled() { return false; }
    bool CheckingSite() { return false; }
    void UpdateCursor(TAdynmemStruct*) {}
    void DrawFeedback(TAdynmemStruct*, _OFFSCREEN*) {}
    bool IsQueuedBuild(const TAdynmemStruct*, const UnitStruct*, const UnitOrdersStruct*) { return false; }
    bool TryCommand(TAdynmemStruct*, const _MOUSEEVENT*) { return false; }
}

#endif // MEX_UPGRADE_ENABLE
