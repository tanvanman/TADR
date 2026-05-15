#ifndef unitrotateH
#define unitrotateH

#include <Windows.h>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "tamem.h"

class SingleHook;
class InlineSingleHook;

// Lets the player place buildings (bmcode==0) with a chosen facing (S/E/N/W)
// by pressing '/' to cycle rotation while the build cursor is active.
// Rotation is captured AT ORDER-ISSUE TIME and stored in a side-table keyed
// by OrderStruct*, so a player can queue factory A south, rotate, and queue
// factory B east — both build with their original facings regardless of
// current rotation when the builders arrive.
//
// Covers both footprint AND yardmap rotation, so asymmetric-yardmap factories
// (vehicle lab, advanced vehicle lab, etc.) pathfind correctly at 90°/180°/270°.
//
// Pure rotation/state plumbing — the placement-preview ghost rendering lives
// separately in CBuildGhost (buildghost.h), which queries CUnitRotate for the
// current rotation index and IsRotationAllowed.
class CUnitRotate
{
public:
    CUnitRotate();
    ~CUnitRotate();

    bool Message(HWND WinProchWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

    // Attempt to cycle the build-cursor rotation by one step in the given
    // direction (>0 = forward / CCW, <0 = backward / CW). Returns true if
    // the rotation actually changed (the unit type allowed >= 2 facings AND
    // a different facing was reachable). On success, plays the click sound
    // and flags the rotate-key tutorial nag as discovered. Caller pre-checks
    // game-in-progress + build cursor state.
    bool TryCycleRotation(int direction);

    static CUnitRotate* GetInstance();

    // Register FBI keys with UnitDefExtensions. Must be called during early
    // ddraw init (alongside VeterancyHack etc.), BEFORE TA loads unit defs.
    // Stores the key index in a static for the later CUnitRotate constructor.
    static void RegisterUnitDefKeys();

    int  GetRotation() const { return m_rotation; }
    void SetRotation(int r);

    // GameTime at the user's last rotation cycle — read by CBuildGhost to
    // restart the preview sweep. Not updated by the transient save/restore
    // in the per-order PreCreate path.
    unsigned GetRotationCycleGameTime() const { return m_rotationCycleGameTime; }

    // Footprint-dim swap state (used by _TestBuildSpot preview + CreateUnit).
    void ApplyRotationTo(unsigned int unitInfoIdx);
    void ClearRotation();

    // Yardmap-pointer swap state (parallel to the footprint swap above).
    // Swaps UNITINFO->p_YardMap to a pre-computed rotated copy.
    void ApplyYardmapRotationTo(unsigned int unitInfoIdx, int rotation);
    void ClearYardmapRotation();

    // Per-order rotation side-table.
    void TagOrderRotation(void* orderPtr, int rotation);
    int  TakeOrderRotation(void* orderPtr);
    void ClearOrderRotation(void* orderPtr);

    // Per-unit rotation side-set: which concrete UnitStruct instances had their
    // heading_angle written by our PostCreate. The runtime reader hook only
    // swaps yardmap for units in this set, so AI-placed structures with large
    // buildangle (e.g. CORSOLAR with buildangle=0x8000) don't get treated as
    // if we'd rotated them.
    void MarkUnitRotated(void* unitPtr);
    bool IsUnitRotated(void* unitPtr) const;
    void UnmarkUnitRotated(void* unitPtr);

    // Retrieve (or lazily build) a rotated yardmap copy for (unitTypeIdx, rotation).
    // Returns nullptr if no yardmap or rotation is 0. Caches indefinitely.
    BYTE* GetRotatedYardmap(unsigned int unitTypeIdx, int rotation);

    // True if the unit type allows the given rotation (0..3 = S/E/N/W).
    // Reads the "Rotations" FBI key (e.g. "SENW") via UnitDefExtensions.
    // Rotation 0 (S / default facing) is always allowed.
    bool IsRotationAllowed(unsigned int unitTypeIdx, int rotation) const;

    // Rotation cycle that respects IsRotationAllowed — returns the next allowed
    // rotation after `current`, or `current` itself if only S is allowed.
    // direction > 0 walks forward (S→E→N→W = CCW from above); direction < 0
    // walks backward (CW). Default is forward, matching the keyboard cycle.
    int NextAllowedRotation(unsigned int unitTypeIdx, int current, int direction = +1) const;

    // ---- Build-menu rotation-overlay prototype ----
    // Linear scan of TA's UnitDef array for the FBI name. -1 if not found.
    int  FindUnitTypeIdxByName(const char* name) const;

    // O(1) cached lookup, rotatable structures only. -1 for any other name.
    int  FindRotatableUnitIdxByName(const char* name) const;

    // True when the unit is a structure (bmcode==0) AND its Rotations= FBI
    // permits at least two facings — i.e. one of the buttons we want to
    // overlay quadrant arrows on.
    bool IsRotatableStructure(unsigned int unitTypeIdx) const;

    // Per-frame render of cardinal arrows on each rotatable-structure
    // button in the active GUI, plus a transient highlight on the
    // most-recently-clicked cardinal. Called from a DrawGameScreen post-
    // DrawGUI hook (ADDR_DrawGameScreen_PostDrawGUI) so the chevrons sit
    // on top of any panel repaint TA does on this frame.
    void DrawBuildMenuRotationOverlays();

    // Pre-placement rotation hook on left-click in the build menu. If the
    // click landed on a rotatable structure button at a permitted cardinal,
    // SetRotation is called and a feedback highlight is scheduled. Returns
    // true if rotation was set; the caller should NOT consume the click —
    // TA's own click handler still runs to enter the build cursor.
    bool OnBuildMenuClick(int screenX, int screenY);

    // Set by the pre-create hook, consumed by the post-create hook.
    int  m_pendingHeading;   // rotation 1..3 if pending, -1 otherwise

    // Capture a copy of TA's UnitDef array with any active footprint /
    // yardmap-pointer swaps reversed in the copy. Intended caller is
    // ChallengeResponse, which fires hash threads off the main message
    // thread; calling this synchronously on the main thread before the
    // detach guarantees a pristine snapshot the hash thread can read
    // without locking. The yardmap content (pointed-at buffer) is itself
    // unswapped — the snapshot's YardMap pointer is rewritten back to
    // m_yardmapCache[idx].origYardmap.
    void CaptureUnitDefSnapshot(std::vector<UnitDefStruct>& out) const;

private:
    int m_rotation;                     // 0=S, 1=E, 2=N, 3=W
    int m_activeRotatedUnitInfoIdx;     // footprint-swap active for which UNITINFO
    int m_activeYardmapRotatedIdx;      // yardmap-swap active for which UNITINFO

    std::unordered_map<void*, int> m_orderRotation;
    std::unordered_map<void*, int> m_rotatedUnits;  // UnitStruct* → rotation (1..3)

    struct YardmapCache
    {
        unsigned origW;
        unsigned origH;
        BYTE* origYardmap;
        std::array<BYTE*, 4> rotated;   // rotated[0] == origYardmap
    };
    std::unordered_map<unsigned, YardmapCache> m_yardmapCache;

    std::vector< std::unique_ptr<InlineSingleHook> > m_hooks;

    // Key index registered with UnitDefExtensions for the "Rotations" FBI field.
    unsigned m_rotationsKeyIdx;

    // Build-menu click feedback: the cardinal that was last clicked on which
    // control, plus a timestamp used to fade out the highlight.
    int   m_menuClickFeedbackControlIdx;
    int   m_menuClickFeedbackCardinal;
    DWORD m_menuClickFeedbackTimestamp;

    // Lowercase UnitName -> UnitDef idx, rotatable structures only.
    // Rebuilt when ta->UNITINFOCount changes.
    void EnsureRotatableNameCache() const;
    mutable std::unordered_map<std::string, int> m_rotatableUnitIdxByLowerName;
    mutable unsigned m_rotatableCacheUnitInfoCount = 0;

    unsigned m_rotationCycleGameTime;

    // Detects on→off transition to dirty panels and overpaint chevrons.
    bool m_lastOverlayEnabled = true;
};

#endif
