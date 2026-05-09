#include "buildghost.h"
#include "unitrotate.h"

#include "dialog.h"
#include "iddrawsurface.h"
#include "tafunctions.h"
#include "tahook.h"
#include "tamem.h"
#include "UnitDefExtensions.h"
#include "hook/etc.h"
#include "hook/hook.h"

#include <cctype>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_set>

// =============================================================================
// CBuildGhost — placement-preview ghost renderer.
//
// Pulls 3DO vertex data from the static MODEL_PTRS array, applies the
// current CUnitRotate rotation around the Y-axis, projects with TA's
// per-unit oblique iso (matches Render_DrawSpriteGroupUnit at 0x4584d0),
// caches the result per (unitType, rotation), and blits via TA's own
// CopyGafToContext (0x4b7f90) so we get TA's color-keyed clipped blit
// "for free" — no engine state touched.
//
// Mode is fixed at compile time via TDRAW_BUILDGHOST_MODE; the bulk of
// this file is wrapped in `#if MODE == ...` blocks. RECT mode reduces to
// empty stubs so the DLL still builds without the cache or rasterisers.
// =============================================================================

namespace
{
    // --- File-local helpers shared by both rendering modes -------------------
    constexpr unsigned TA_MAIN_PTR_ADDR        = 0x00511de8;

    TAdynmemStruct* GetTA()
    {
        return *reinterpret_cast<TAdynmemStruct**>(TA_MAIN_PTR_ADDR);
    }

    UnitDefStruct* GetUnitDef(unsigned int idx)
    {
        TAdynmemStruct* ta = GetTA();
        if (!ta || !ta->UnitDef) return nullptr;
        return &ta->UnitDef[idx];
    }
}

// =============================================================================
// Singleton plumbing — Meyers singleton, lazy-constructed on first call.
// =============================================================================

CBuildGhost* CBuildGhost::GetInstance()
{
    static CBuildGhost instance;
    return &instance;
}

// FBI key indices, registered at early ddraw init via RegisterUnitDefKeys.
// Sentinel 0 = not registered (the registerXxxKey APIs never return 0 for
// a real key — the high bits are tagged).
//   PreviewPieces=        comma/space-separated piece-name whitelist. When
//                         non-empty, ONLY listed pieces render in the ghost.
//                         Used as the default for every rotation.
//   PreviewPiecesS=       per-rotation whitelist override for cardinals
//   PreviewPiecesE=       S=0, E=1, N=2, W=3. When set (non-empty) the
//   PreviewPiecesN=       per-rotation key takes precedence over
//   PreviewPiecesW=       PreviewPieces= for that rotation; when unset
//                         the global PreviewPieces= applies.
//   PreviewFaceOpponent=  0/1. When 1, override the user's chosen rotation
//                         to face the nearest enemy commander, snapped to
//                         nearest cardinal. For units whose Create() script
//                         auto-rotates the body toward the nearest enemy
//                         commander — keeps the preview consistent with
//                         the script's final heading instead of always
//                         showing the default southern facing.
static unsigned g_previewPiecesKeyIdx        = 0;
static unsigned g_previewPiecesByRotKeyIdx[4] = { 0, 0, 0, 0 };
static unsigned g_previewFaceOpponentKeyIdx  = 0;

void CBuildGhost::RegisterUnitDefKeys()
{
    if (g_previewPiecesKeyIdx == 0)
    {
        g_previewPiecesKeyIdx =
            UnitDefExtensions::GetInstance()->registerStringKey("PreviewPieces", "");
    }
    static const char* const kRotKeyNames[4] = {
        "PreviewPiecesS", "PreviewPiecesE", "PreviewPiecesN", "PreviewPiecesW"
    };
    for (int r = 0; r < 4; ++r)
    {
        if (g_previewPiecesByRotKeyIdx[r] == 0)
        {
            g_previewPiecesByRotKeyIdx[r] =
                UnitDefExtensions::GetInstance()->registerStringKey(kRotKeyNames[r], "");
        }
    }
    if (g_previewFaceOpponentKeyIdx == 0)
    {
        g_previewFaceOpponentKeyIdx =
            UnitDefExtensions::GetInstance()->registerIntKey("PreviewFaceOpponent", 0);
    }
}

#if TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_FULL3D
// Inside DrawGameScreen, at the join point reached both when TA drew its
// build rect and when the JZ at 0x469e0d skipped it (e.g. while mex-snap
// is active and Enable/DisableTABuildRect has zeroed the gate at
// 0x469e0b). Rendering here puts the ghost at the same pipeline stage as
// the native rect — GUI dialogs / menus drawn afterwards composite on
// top instead of being painted under by a later DirectDraw Lock — and
// keeps it firing regardless of the rect-on/off toggle. Adjacent
// patches that must stay intact: 0x469e0b (rect on/off) and 0x469e7f
// (rect colour AND-immediate flipped by ConstructionKickout) — both
// upstream of this address.
static const unsigned int kBuildRectHookAddr = 0x469f30;

static int __stdcall BuildRectAfterHookProc(PInlineX86StackBuffer)
{
    // Overlay rects first — VisualizeMexSnapPreview moves CircleSelect_Pos1
    // to the snap target via TestBuildSpot, which the cursor ghost reads.
    if (LocalShare && LocalShare->TAHook)
        reinterpret_cast<CTAHook*>(LocalShare->TAHook)->DrawBuildOverlays();
    if (CBuildGhost* g = CBuildGhost::GetInstance()) g->RenderNanoframeGhost();
    return 0;
}
#endif

CBuildGhost::CBuildGhost()
{
    ReadRotateKeyDiscovered();
#if TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_RECT
    IDDrawSurface::OutptTxt("[BuildGhost] mode=RECT (no model preview)");
#elif TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_FULL3D
    IDDrawSurface::OutptTxt("[BuildGhost] mode=FULL3D");
    m_hooks.push_back(std::make_shared<InlineSingleHook>(
        kBuildRectHookAddr, 5, INLINE_5BYTESLAGGERJMP, BuildRectAfterHookProc));
#endif
}

void CBuildGhost::ReadRotateKeyDiscovered()
{
    HKEY hKey;
    DWORD dwDisposition;
    std::string SubKey = CompanyName_CCSTR;
    SubKey += "\\Eye";
    if (RegCreateKeyEx(HKEY_CURRENT_USER, SubKey.c_str(), NULL, TADRCONFIGREGNAME,
            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition)
        == ERROR_SUCCESS)
    {
        DWORD value = 0;
        DWORD size  = sizeof(value);
        if (RegQueryValueEx(hKey, "RotateBuildKeyDiscovered", NULL, NULL,
                reinterpret_cast<BYTE*>(&value), &size) == ERROR_SUCCESS)
        {
            m_rotateBuildKeyDiscovered = (value != 0);
        }
        RegCloseKey(hKey);
    }
}

void CBuildGhost::WriteRotateKeyDiscovered()
{
    HKEY hKey1;
    HKEY hKey;
    DWORD dwDisposition;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, CompanyName_CCSTR, NULL, TADRCONFIGREGNAME,
            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey1, &dwDisposition)
        != ERROR_SUCCESS) return;
    if (RegCreateKeyEx(hKey1, "Eye", NULL, TADRCONFIGREGNAME,
            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition)
        == ERROR_SUCCESS)
    {
        DWORD value = m_rotateBuildKeyDiscovered ? 1u : 0u;
        RegSetValueEx(hKey, "RotateBuildKeyDiscovered", NULL, REG_DWORD,
            reinterpret_cast<const BYTE*>(&value), sizeof(value));
        RegCloseKey(hKey);
    }
    RegCloseKey(hKey1);
}

void CBuildGhost::SetRotateKeyDiscovered()
{
    if (m_rotateBuildKeyDiscovered) return;
    m_rotateBuildKeyDiscovered = true;
    WriteRotateKeyDiscovered();
}

// =============================================================================
// 3DO helpers (FULL3D only)
// =============================================================================
#if TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_FULL3D

namespace
{
    // Model3DONode and Model3DOFace are defined in tamem.h alongside
    // TAdynmemStruct (which holds MODEL_PTRS as Model3DONode**). Belt-and-
    // braces size checks here so any future drift in tamem.h shows up at
    // this TU's compile time.
    static_assert(sizeof(Model3DONode) == 0x40, "Model3DONode size mismatch");
    static_assert(sizeof(Model3DOFace) == 0x20, "Model3DOFace size mismatch");

    // Apply a cardinal rotation around the Y-axis to a (x, z) coord pair.
    // rot 0 (heading 0x8000, 180°): (x, z) -> (-x, -z)
    // rot 1 (heading 0xC000, 270°): (x, z) -> ( z, -x)
    // rot 2 (heading 0x0000,   0°): (x, z) -> ( x,  z)
    // rot 3 (heading 0x4000,  90°): (x, z) -> (-z,  x)
    inline void RotateXZ(int& x, int& z, int rotation)
    {
        int ox = x, oz = z;
        switch (rotation & 3)
        {
        case 0: x = -ox; z = -oz; break;
        case 1: x =  oz; z = -ox; break;
        case 2: x =  ox; z =  oz; break;
        case 3: x = -oz; z =  ox; break;
        }
    }

    // Project a TA world-space point to 2D screen offset (relative to unit
    // origin). Matches Render_DrawSpriteGroupUnit's per-vertex arithmetic
    // at 0x4584d0:
    //   screen_x = world_x
    //   screen_y = -world_z - world_y / 2
    // Vertex coords are 16.16 fixed-point (high word = integer).
    inline void ProjectToScreen(int wx_fxp, int wy_fxp, int wz_fxp,
                                int& outSx, int& outSy)
    {
        int ix      = wx_fxp >> 16;
        int iz      = wz_fxp >> 16;
        int iy_div2 = wy_fxp >> 17;     // world-y / 2
        outSx =  ix;
        outSy = -iz - iy_div2;
    }

    // Native TA color-keyed sprite blit. Locks attack surface when dst==NULL,
    // clips against dst->ScreenRect, copies pixels that differ from
    // frame.ColorKey. __stdcall — ends in `ret 10h`.
    using Fn_CopyGafToContext = void (__stdcall*)(void* dst, const void* gafFrame,
                                                  int x, int y);
    const Fn_CopyGafToContext kFn_CopyGafToContext =
        (Fn_CopyGafToContext)0x004b7f90;

    using Fn_LockAttackSurface = int (__stdcall*)(void* outOffscreen);
    const Fn_LockAttackSurface kFn_LockAttackSurface =
        (Fn_LockAttackSurface)0x004c5e70;

    using Fn_UnlockAttackedSurface = void (__stdcall*)(void* ctx);
    const Fn_UnlockAttackedSurface kFn_UnlockAttackedSurface =
        (Fn_UnlockAttackedSurface)0x004c5fa0;
}
#endif // shared


// =============================================================================
// FULL3D mode
// =============================================================================
#if TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_FULL3D
namespace
{
    // Skip-the-faces test for cosmetic / ephemeral pieces. Match TA's COB
    // Create() convention: pieces whose names contain these substrings are
    // typically hidden at unit creation and only briefly Show()n during a
    // fire / death script. Includes muzzle flares ("flare", "flarea"…),
    // muzzle flashes ("flash", "mlasflsh", "rbigflash"), firing-point
    // markers ("lfirept", "rfire"…), engine flame trails ("flame1"…), and
    // water wakes ("wake1", "wake2"). Sampled across ProTA48 scripts —
    // no actual body part of any unit shares these substrings, so the
    // false-positive risk is low. Children of a skipped piece are still
    // traversed (a child piece is independently visible in TA's COB model).
    inline bool IsEphemeralEffectPieceName(const char* name)
    {
        if (!name) return false;
        char lower[64] = {0};
        for (int i = 0; i < 63 && name[i]; ++i)
            lower[i] = static_cast<char>(::tolower(static_cast<unsigned char>(name[i])));
        return std::strstr(lower, "flare") ||
               std::strstr(lower, "flash") ||
               std::strstr(lower, "muzzle") ||
               std::strstr(lower, "fire")  ||
               std::strstr(lower, "flame") ||
               std::strstr(lower, "wake");
    }

    // Parse a "PreviewPieces=" FBI value into a lowercase set of piece names
    // that should be the *only* pieces visible in the nanoframe preview.
    // Empty / unset → no whitelist (fall back to ephemeral-effect filter).
    // Separator: any combination of whitespace, commas, semicolons. Names
    // are case-folded so the FBI author can write either case.
    std::unordered_set<std::string> ParsePreviewPiecesWhitelist(const std::string& s)
    {
        std::unordered_set<std::string> out;
        std::string token;
        auto flush = [&]() {
            if (!token.empty()) {
                for (char& c : token) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
                out.insert(std::move(token));
                token.clear();
            }
        };
        for (char c : s) {
            if (c == ' ' || c == '\t' || c == ',' || c == ';' || c == '\r' || c == '\n')
                flush();
            else
                token.push_back(c);
        }
        flush();
        return out;
    }

    // depth = world-y (elevation) mapped to 1..255 — for hidden-surface
    //         removal during scanline fill / edge raster.
    // zCoord = world-z (north/south) mapped to 1..255 — for the
    //         z-plane sweep effect at render time.
    struct ProjVert { int sx, sy; int depth; int zCoord; };

    inline void RasterEdge(unsigned char* dst,
                           const unsigned char* depthBuf,
                           int bufW, int bufH,
                           int x0, int y0, int d0,
                           int x1, int y1, int d1,
                           unsigned char color)
    {
        const int kZBias = 2;
        int dx =  std::abs(x1 - x0);
        int dy = -std::abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        int totalSteps = (dx > -dy ? dx : -dy);
        if (totalSteps < 1) totalSteps = 1;
        int step = 0;
        for (;;)
        {
            if ((unsigned)x0 < (unsigned)bufW && (unsigned)y0 < (unsigned)bufH)
            {
                int z = d0 + (d1 - d0) * step / totalSteps;
                if (z < 0)   z = 0;
                if (z > 255) z = 255;
                int idx = y0 * bufW + x0;
                if ((unsigned char)z + kZBias >= depthBuf[idx])
                    dst[idx] = color;
            }
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
            ++step;
        }
    }

    inline uint64_t EdgeKey3D(short x1, short y1, short x2, short y2)
    {
        if (y1 > y2 || (y1 == y2 && x1 > x2))
        {
            short t;
            t = x1; x1 = x2; x2 = t;
            t = y1; y1 = y2; y2 = t;
        }
        return (static_cast<uint64_t>(static_cast<uint16_t>(x1)) << 48)
             | (static_cast<uint64_t>(static_cast<uint16_t>(y1)) << 32)
             | (static_cast<uint64_t>(static_cast<uint16_t>(x2)) << 16)
             | (static_cast<uint64_t>(static_cast<uint16_t>(y2)));
    }

    // Convex polygon scanline fill with z-test. zCoordBuf is optional —
    // when non-null, each accepted (closer-than-existing) pixel also gets
    // the per-pixel ProjVert::zCoord interpolated and written there.
    void FillConvexPolyZ(unsigned char* colorBuf,
                         unsigned char* depthBuf,
                         unsigned char* zCoordBuf,
                         int bufW, int bufH,
                         const ProjVert* verts, int n,
                         unsigned char polyColor)
    {
        if (n < 3) return;
        int minY = verts[0].sy, maxY = verts[0].sy;
        for (int i = 1; i < n; ++i)
        {
            if (verts[i].sy < minY) minY = verts[i].sy;
            if (verts[i].sy > maxY) maxY = verts[i].sy;
        }
        if (maxY < 0 || minY >= bufH) return;
        if (minY < 0) minY = 0;
        if (maxY >= bufH) maxY = bufH - 1;

        for (int y = minY; y <= maxY; ++y)
        {
            int leftX = INT_MAX, rightX = INT_MIN;
            int leftDepth = 0, rightDepth = 0;
            int leftZc    = 0, rightZc    = 0;
            for (int i = 0; i < n; ++i)
            {
                int j = (i + 1) % n;
                int y0 = verts[i].sy, y1 = verts[j].sy;
                int yLo, yHi;
                if (y0 < y1) { yLo = y0; yHi = y1; }
                else         { yLo = y1; yHi = y0; }
                if (y < yLo || y > yHi) continue;
                if (yLo == yHi) continue;
                int dy = y1 - y0;
                int t  = y - y0;
                int x  = verts[i].sx     + (verts[j].sx     - verts[i].sx)     * t / dy;
                int d  = verts[i].depth  + (verts[j].depth  - verts[i].depth)  * t / dy;
                int zc = verts[i].zCoord + (verts[j].zCoord - verts[i].zCoord) * t / dy;
                if (x < leftX)  { leftX  = x; leftDepth = d; leftZc  = zc; }
                if (x > rightX) { rightX = x; rightDepth = d; rightZc = zc; }
            }
            if (leftX > rightX) continue;
            int spanLeft  = leftX  < 0 ? 0 : leftX;
            int spanRight = rightX >= bufW ? bufW - 1 : rightX;
            if (spanLeft > spanRight) continue;

            int spanW = rightX - leftX;
            int rowBase = y * bufW;
            for (int x = spanLeft; x <= spanRight; ++x)
            {
                int d  = (spanW > 0) ? leftDepth + (rightDepth - leftDepth) * (x - leftX) / spanW : leftDepth;
                int zc = (spanW > 0) ? leftZc    + (rightZc    - leftZc)    * (x - leftX) / spanW : leftZc;
                if (d < 0)   d = 0;
                if (d > 255) d = 255;
                if (zc < 1)   zc = 1;
                if (zc > 255) zc = 255;
                int idx = rowBase + x;
                if ((unsigned char)d >= depthBuf[idx])
                {
                    depthBuf[idx] = (unsigned char)d;
                    colorBuf[idx] = polyColor;
                    if (zCoordBuf) zCoordBuf[idx] = (unsigned char)zc;
                }
            }
        }
    }
}

const CBuildGhost::NanoframeSprite3D* CBuildGhost::GetNanoframeSprite3D(
    unsigned unitInfoIdx, int rotation)
{
    rotation &= 3;
    unsigned key = (unitInfoIdx << 2) | rotation;

    auto it = m_nanoframe3DCache.find(key);
    if (it != m_nanoframe3DCache.end())
        return it->second.frame.Width == 0 ? nullptr : &it->second;

    TAdynmemStruct* ta = GetTA();
    if (!ta || !ta->MODEL_PTRS) return nullptr;
    Model3DONode* rootNode = ta->MODEL_PTRS[unitInfoIdx];
    if (!rootNode)
    {
        m_nanoframe3DCache[key] = NanoframeSprite3D{};
        return nullptr;
    }

    // FBI-driven piece whitelist: when the mod author has set
    // PreviewPieces= (or one of the per-rotation overrides
    // PreviewPiecesS/E/N/W=), ONLY listed pieces are rendered (everything
    // else is hidden). Empty / unset → no whitelist, fall back to the
    // hardcoded ephemeral filter for cosmetic pieces (flare/flash/etc.).
    //
    // Lookup order for the active rotation:
    //   1. PreviewPieces<rot>=<list>   (S=0, E=1, N=2, W=3)
    //   2. PreviewPieces=<list>        (default for every rotation)
    //   3. nothing                     (use ephemeral filter)
    //
    // Useful for upgrade-style units whose 3DO model contains pieces
    // that the COB script hides at runtime (e.g. bundled basic and
    // upgraded geometry), and for units whose pieces differ visually
    // by facing.
    UnitDefExtensions* ude = UnitDefExtensions::GetInstance();
    const std::string* whitelistSrc = nullptr;
    if (g_previewPiecesByRotKeyIdx[rotation] != 0)
    {
        const std::string& s = ude->getString(unitInfoIdx, g_previewPiecesByRotKeyIdx[rotation]);
        if (!s.empty()) whitelistSrc = &s;
    }
    if (!whitelistSrc && g_previewPiecesKeyIdx != 0)
    {
        const std::string& s = ude->getString(unitInfoIdx, g_previewPiecesKeyIdx);
        if (!s.empty()) whitelistSrc = &s;
    }
    const std::unordered_set<std::string> previewWhitelist =
        whitelistSrc ? ParsePreviewPiecesWhitelist(*whitelistSrc)
                     : std::unordered_set<std::string>{};
    const bool hasWhitelist = !previewWhitelist.empty();

    // -------- Pass 1: traverse, project, classify, collect faces + edges.
    struct StashedFace { std::vector<ProjVert> verts; };
    struct StashedEdge { ProjVert a, b; };
    std::vector<StashedFace> faces;
    std::vector<StashedEdge> visibleEdges;
    faces.reserve(64);
    visibleEdges.reserve(256);

    std::unordered_set<uint64_t> seenEdges;
    seenEdges.reserve(512);

    int minSX = INT_MAX, minSY = INT_MAX;
    int maxSX = INT_MIN, maxSY = INT_MIN;
    int minWY_int = INT_MAX, maxWY_int = INT_MIN;
    // zCoord encoding tracks the model's LOCAL +Z extent (front-back of
    // the unit, pre-rotation) so the z-plane sweep follows the unit's
    // depth axis regardless of cardinal facing.
    int minLZ_int = INT_MAX, maxLZ_int = INT_MIN;

    struct PieceFrame {
        Model3DONode* node;
        int origX, origY, origZ;     // rotated cumulative origin (for projection)
        int unrotOrigZ;              // unrotated cumulative origin Z (for local-z encoding)
    };
    std::vector<PieceFrame> stack;
    stack.reserve(32);
    stack.push_back({ rootNode, 0, 0, 0, 0 });

    std::vector<int> projVx, projVy, projVz_world_y, projVz_local_z;

    while (!stack.empty())
    {
        PieceFrame f = stack.back();
        stack.pop_back();
        if (!f.node) continue;

        for (Model3DONode* node = f.node; node; )
        {
            int vertCount             = node->VertexCount;
            int faceCount             = node->FaceCount;
            int ofsX                  = node->OffsetX;
            int ofsY                  = node->OffsetY;
            int ofsZ                  = node->OffsetZ;
            int* vertArray            = node->pVertexArray;
            Model3DOFace* faceArray   = node->pFaceArray;
            Model3DONode* child       = node->pChild;
            Model3DONode* sibling     = node->pSibling;

            int rOfsX = ofsX, rOfsZ = ofsZ;
            RotateXZ(rOfsX, rOfsZ, rotation);
            int pieceOrigX = f.origX + rOfsX;
            int pieceOrigY = f.origY + ofsY;
            int pieceOrigZ = f.origZ + rOfsZ;
            int unrotPieceOrigZ = f.unrotOrigZ + ofsZ;

            // Per-piece visibility for the preview:
            //   - If the FBI sets PreviewPieces=, render ONLY pieces in that
            //     whitelist (case-insensitive match against pNameStr).
            //   - Otherwise, render every piece except the hard-coded
            //     ephemeral-effect names (flare/flash/muzzle/fire/flame/wake).
            // Children of a skipped piece are still walked — a sibling/child
            // piece is independently visible in TA's COB model.
            bool skipFaces;
            if (hasWhitelist) {
                if (!node->pNameStr) {
                    skipFaces = true;
                } else {
                    char lowered[64] = {0};
                    for (int i = 0; i < 63 && node->pNameStr[i]; ++i)
                        lowered[i] = static_cast<char>(::tolower(static_cast<unsigned char>(node->pNameStr[i])));
                    skipFaces = (previewWhitelist.find(lowered) == previewWhitelist.end());
                }
            } else {
                skipFaces = IsEphemeralEffectPieceName(node->pNameStr);
            }

            if (!skipFaces && vertCount > 0 && vertArray && faceCount > 0 && faceArray)
            {
                projVx.resize(vertCount);
                projVy.resize(vertCount);
                projVz_world_y.resize(vertCount);
                projVz_local_z.resize(vertCount);
                for (int v = 0; v < vertCount; ++v)
                {
                    int vx = vertArray[v * 3 + 0];
                    int vy = vertArray[v * 3 + 1];
                    int vz = vertArray[v * 3 + 2];
                    int rx = vx, rz = vz;
                    RotateXZ(rx, rz, rotation);
                    int wx = pieceOrigX + rx;
                    int wy = pieceOrigY + vy;
                    int wz = pieceOrigZ + rz;
                    int psx, psy;
                    ProjectToScreen(wx, wy, wz, psx, psy);
                    projVx[v] = psx;
                    projVy[v] = psy;
                    int wy_int = wy >> 16;
                    // Local +Z is the model's front-axis (3DO convention).
                    // Stored unrotated so the sweep follows the unit body
                    // when it's drawn at any cardinal.
                    int lz_int = (unrotPieceOrigZ + vz) >> 16;
                    projVz_world_y[v] = wy_int;
                    projVz_local_z[v] = lz_int;
                    if (psx < minSX) minSX = psx;
                    if (psx > maxSX) maxSX = psx;
                    if (psy < minSY) minSY = psy;
                    if (psy > maxSY) maxSY = psy;
                    if (wy_int < minWY_int) minWY_int = wy_int;
                    if (wy_int > maxWY_int) maxWY_int = wy_int;
                    if (lz_int < minLZ_int) minLZ_int = lz_int;
                    if (lz_int > maxLZ_int) maxLZ_int = lz_int;
                }

                for (int fi = 0; fi < faceCount; ++fi)
                {
                    Model3DOFace& faceP = faceArray[fi];
                    int fVertCount = faceP.VertexCount;
                    unsigned short* idx = faceP.pVertexIndices;
                    if (fVertCount < 3 || !idx) continue;

                    long long area2 = 0;
                    bool valid = true;
                    for (int i = 0; i < fVertCount; ++i)
                    {
                        int a = idx[i];
                        int b = idx[(i + 1) % fVertCount];
                        if (a >= vertCount || b >= vertCount) { valid = false; break; }
                        area2 += (long long)projVx[a] * projVy[b]
                               - (long long)projVx[b] * projVy[a];
                    }
                    if (!valid || area2 <= 0) continue;

                    StashedFace sf;
                    sf.verts.reserve(fVertCount);
                    for (int i = 0; i < fVertCount; ++i)
                    {
                        int a = idx[i];
                        ProjVert pv;
                        pv.sx = projVx[a];
                        pv.sy = projVy[a];
                        pv.depth  = projVz_world_y[a];
                        pv.zCoord = projVz_local_z[a];
                        sf.verts.push_back(pv);
                    }
                    faces.push_back(std::move(sf));

                    for (int i = 0; i < fVertCount; ++i)
                    {
                        int a = idx[i];
                        int b = idx[(i + 1) % fVertCount];
                        if (a >= vertCount || b >= vertCount) continue;
                        short ax = (short)projVx[a], ay = (short)projVy[a];
                        short bx = (short)projVx[b], by = (short)projVy[b];
                        uint64_t ck = EdgeKey3D(ax, ay, bx, by);
                        if (!seenEdges.insert(ck).second) continue;
                        StashedEdge se{};
                        se.a.sx = projVx[a]; se.a.sy = projVy[a];
                        se.a.depth = projVz_world_y[a];
                        se.b.sx = projVx[b]; se.b.sy = projVy[b];
                        se.b.depth = projVz_world_y[b];
                        // edges don't participate in the z-sweep
                        se.a.zCoord = se.b.zCoord = 0;
                        visibleEdges.push_back(se);
                    }
                }
            }

            if (child)
            {
                PieceFrame childFrame;
                childFrame.node = child;
                childFrame.origX = pieceOrigX;
                childFrame.origY = pieceOrigY;
                childFrame.origZ = pieceOrigZ;
                childFrame.unrotOrigZ = unrotPieceOrigZ;
                stack.push_back(childFrame);
            }
            node = sibling;
        }
    }

    if (faces.empty())
    {
        m_nanoframe3DCache[key] = NanoframeSprite3D{};
        return nullptr;
    }

    int w = maxSX - minSX + 1;
    int h = maxSY - minSY + 1;
    if (w <= 0 || h <= 0 || w > 1024 || h > 1024)
    {
        m_nanoframe3DCache[key] = NanoframeSprite3D{};
        return nullptr;
    }

    NanoframeSprite3D& out = m_nanoframe3DCache[key];
    out.pixels.assign(static_cast<size_t>(w) * static_cast<size_t>(h), 0u);
    out.depth.assign(static_cast<size_t>(w) * static_cast<size_t>(h), 0u);
    out.zCoord.assign(static_cast<size_t>(w) * static_cast<size_t>(h), 0u);
    out.edgePixels.assign(static_cast<size_t>(w) * static_cast<size_t>(h), 0u);

    int wyRange = maxWY_int - minWY_int;
    if (wyRange <= 0) wyRange = 1;
    int lzRange = maxLZ_int - minLZ_int;
    if (lzRange <= 0) lzRange = 1;

    const unsigned char kColorKey  = 0;
    const unsigned char kBaseColor = 234;

    for (const StashedFace& sf : faces)
    {
        std::vector<ProjVert> remapped;
        remapped.reserve(sf.verts.size());
        for (const ProjVert& pv : sf.verts)
        {
            ProjVert r;
            r.sx = pv.sx - minSX;
            r.sy = pv.sy - minSY;
            int d = ((pv.depth  - minWY_int) * 254 + (wyRange / 2)) / wyRange + 1;
            int z = ((pv.zCoord - minLZ_int) * 254 + (lzRange / 2)) / lzRange + 1;
            if (d < 1)   d = 1;
            if (d > 255) d = 255;
            if (z < 1)   z = 1;
            if (z > 255) z = 255;
            r.depth  = d;
            r.zCoord = z;
            remapped.push_back(r);
        }
        FillConvexPolyZ(out.pixels.data(), out.depth.data(), out.zCoord.data(),
                        w, h,
                        remapped.data(), (int)remapped.size(),
                        kBaseColor);
    }

    for (const StashedEdge& se : visibleEdges)
    {
        int x0 = se.a.sx - minSX;
        int y0 = se.a.sy - minSY;
        int x1 = se.b.sx - minSX;
        int y1 = se.b.sy - minSY;
        int d0 = ((se.a.depth - minWY_int) * 254 + (wyRange / 2)) / wyRange + 1;
        int d1 = ((se.b.depth - minWY_int) * 254 + (wyRange / 2)) / wyRange + 1;
        if (d0 < 1) d0 = 1; if (d0 > 255) d0 = 255;
        if (d1 < 1) d1 = 1; if (d1 > 255) d1 = 255;
        RasterEdge(out.edgePixels.data(), out.depth.data(),
                   w, h, x0, y0, d0, x1, y1, d1, kBaseColor);
    }

    out.frame.Width        = static_cast<unsigned short>(w);
    out.frame.Height       = static_cast<unsigned short>(h);
    out.frame.HotspotX     = static_cast<short>(-minSX);
    out.frame.HotspotY     = static_cast<short>(-minSY);
    out.frame.ColorKey     = kColorKey;
    out.frame.Compressed   = 0;
    out.frame.SubFrames    = 0;
    out.frame.AlphaBlend   = 0;
    out.frame.Unknown_0C   = 0;
    out.frame.PtrFrameBits = out.pixels.data();
    out.frame.Bits2_Ptr    = out.depth.data();

    IDDrawSurface::OutptFmtTxt(
        "[BuildGhost-3D] cached %dx%d (%u faces, %u edges) typeIdx=%u rot=%d wy=[%d,%d]",
        w, h, (unsigned)faces.size(), (unsigned)visibleEdges.size(),
        unitInfoIdx, rotation, minWY_int, maxWY_int);
    return &out;
}
#endif // FULL3D

// =============================================================================
// Public render entry points
// =============================================================================

void CBuildGhost::RenderNanoframeGhost()
{
#if TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_RECT
    return;

#elif TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_FULL3D
    // Suppress the cursor ghost when the mouse is outside the game area
    // (e.g. hovering the build menu after a right-click cancel + reselect).
    // CircleSelect_Pos1/Pos2 still hold the last in-game build spot at this
    // point, but the engine's DrawTranspRectangle is correctly gated by
    // IsPositionInRect(GameSreen_Rect, CurtMousePostion), and we mirror
    // that gating here so we don't draw a ghost where no rect exists.
    TAdynmemStruct* ta = GetTA();
    if (ta)
    {
        const RECT& gs = ta->GameSreen_Rect;
        long mx = ta->CurtMousePostion.x;
        long my = ta->CurtMousePostion.y;
        if (mx < gs.left || mx >= gs.right || my < gs.top || my >= gs.bottom)
            return;
    }

    // When CTAHook is in line-build mode, VisualizeRow's per-position
    // loop drives RenderGhostAtCurrentBuildSpot directly; skip the
    // cursor render here to avoid double-blit at the trailing line slot
    // (where CircleSelect_Pos1 ends up after the loop).
    if (LocalShare && LocalShare->TAHook
        && reinterpret_cast<CTAHook*>(LocalShare->TAHook)->IsLineBuilding())
    {
        return;
    }
    RenderGhostAtCurrentBuildSpot(/*showNag=*/true);
#endif
}

#if TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_FULL3D
void CBuildGhost::RenderGhostAtCurrentBuildSpot(bool showNag)
{
    TAdynmemStruct* ta = GetTA();
    if (!ta) return;
    if (ta->PrepareOrder_Type != ordertype::BUILD) return;

    // Skip the ghost on red build rectangles. 0x40 set = OK / green;
    // cleared = blocked / red.
    if ((ta->BuildSpotState & 0x40) == 0) return;

    unsigned buildIdx = static_cast<unsigned>(ta->BuildUnitID);
    if (buildIdx == 0) return;

    UnitDefStruct* ui = GetUnitDef(buildIdx);
    if (!ui) return;
    if (ui->bmcode != 0) return;

    int      rotation            = 0;
    int      allowedCount         = 1;
    unsigned rotationCycleGameTime = 0;
    if (CUnitRotate* rot = CUnitRotate::GetInstance())
    {
        rotation = rot->GetRotation() & 3;
        if (!rot->IsRotationAllowed(buildIdx, rotation)) rotation = 0;
        allowedCount = 0;
        for (int r = 0; r < 4; ++r)
            if (rot->IsRotationAllowed(buildIdx, r)) ++allowedCount;
        rotationCycleGameTime = rot->GetRotationCycleGameTime();
    }

    // Build-rect centre = midpoint of (CircleSelect_Pos1, Pos2). Pos1 is
    // the snapped TILE corner (TestBuildSpot writes `tile_idx << 4`),
    // Pos2 = Pos1 + footPrint*16. Centre = Pos1 + footPrint*8 — already
    // on the right snap grid for odd/even footprints.
    int p1x = static_cast<int>(ta->CircleSelect_Pos1TAx);
    int p1y = static_cast<int>(ta->CircleSelect_Pos1TAy);
    int p2x = static_cast<int>(ta->CircleSelect_Pos2TAx);
    int p2y = static_cast<int>(ta->CircleSelect_Pos2TAy);
    int elev = static_cast<int>(ta->CircleSelect_Pos1TAz);
    int centreX = (p1x + p2x) / 2;
    int centreY = (p1y + p2y) / 2;

    // PreviewFaceOpponent=1 — override rotation to face the nearest enemy
    // commander's current location, snapped to the nearest cardinal
    // {S=0, E=1, N=2, W=3}. For units whose Create() script auto-rotates
    // the body toward the nearest enemy commander, this keeps the
    // preview consistent with the script's final heading instead of
    // showing the unit at its default southern facing.
    //
    // We look up each enemy player's first slot in their Units array as
    // a stand-in for the commander — in normal play the commander is
    // always Units[0], and the rare edge case (commander died, slot
    // recycled) just means the preview falls back to whichever unit got
    // the first slot, which is still close enough for a cardinal snap.
    //
    // INFO LEAK MITIGATION: only apply the override when the build cursor
    // is within the selected builder's own builddistance. Without this
    // gate a player could wave the build cursor across fog of war and
    // triangulate the nearest enemy commander's position from how the
    // preview rotates. Tying the radius to each builder's builddistance
    // means genuine builds always benefit from the orientation hint
    // (since you only place there if the unit can reach), while remote
    // scans get no signal.
    //
    // The Rotations= gate is bypassed: setting PreviewFaceOpponent is the
    // FBI author's explicit signal that this unit's heading is script-
    // driven, not player-controlled.
    //
    // Falls back to the user's rotation if no enemy commander is found
    // (single-player, all enemies dead, etc.) or if the cursor is too
    // far from any selected builder.
    if (g_previewFaceOpponentKeyIdx != 0
        && UnitDefExtensions::GetInstance()->getInt(buildIdx, g_previewFaceOpponentKeyIdx) != 0)
    {
        int myIdx = ta->LocalHumanPlayer_PlayerID;
        if (myIdx >= 0 && myIdx < 10)
        {
            PlayerStruct* me = &ta->Players[myIdx];

            // At least one selected, completed local unit must have the
            // cursor inside its own builddistance for the override to
            // apply. Each unit gets its own threshold from UnitType.
            bool cursorNearBuilder = false;
            if (me->Units && me->UnitsAry_End)
            {
                for (UnitStruct* u = me->Units; u < me->UnitsAry_End; ++u)
                {
                    if (!u->IsUnit || u->UnitID <= 0) continue;
                    if (!(u->UnitSelected & 0x10)) continue;       // not selected
                    if (u->Nanoframe != 0.0f) continue;            // still being built
                    if (!u->UnitType) continue;
                    long long radius   = static_cast<long long>(u->UnitType->builddistance);
                    if (radius <= 0) continue;                     // not a builder
                    long long radiusSq = radius * radius;
                    long long dx = static_cast<long long>(u->XPos) - centreX;
                    long long dy = static_cast<long long>(u->YPos) - centreY;
                    if (dx * dx + dy * dy <= radiusSq)
                    {
                        cursorNearBuilder = true;
                        break;
                    }
                }
            }

            if (cursorNearBuilder)
            {
                long long bestDsq    = LLONG_MAX;
                int       bestDx     = 0;
                int       bestDy     = 0;
                bool      foundEnemy = false;
                for (int i = 0; i < 10; ++i)
                {
                    if (i == myIdx) continue;
                    PlayerStruct* other = &ta->Players[i];
                    if (!other->PlayerActive) continue;
                    if (other->PlayerInfo
                        && (other->PlayerInfo->PropertyMask & PlayerPropertyMask::WATCH)) continue;
                    if (me->AllyFlagAry[i]) continue;             // skip allies (from my POV)
                    UnitStruct* cmdr = other->Units;
                    if (!cmdr) continue;
                    if (!cmdr->IsUnit || cmdr->UnitID <= 0) continue;
                    long long dx = static_cast<long long>(cmdr->XPos) - centreX;
                    long long dy = static_cast<long long>(cmdr->YPos) - centreY;
                    long long dsq = dx * dx + dy * dy;
                    if (dsq < bestDsq)
                    {
                        bestDsq    = dsq;
                        bestDx     = static_cast<int>(dx);
                        bestDy     = static_cast<int>(dy);
                        foundEnemy = true;
                    }
                }
                if (foundEnemy)
                {
                    if (std::abs(bestDx) > std::abs(bestDy))
                        rotation = bestDx > 0 ? 1 : 3;   // east / west
                    else
                        rotation = bestDy > 0 ? 0 : 2;   // south / north
                }
            }
        }
    }
    int cx = (centreX - ta->EyeBallMapXPos) + 128;
    int cy = (centreY - ta->EyeBallMapYPos) + 32 - (elev / 2);

    const NanoframeSprite3D* sprite = GetNanoframeSprite3D(buildIdx, rotation);
    if (!sprite) return;

    const RECT& gs = ta->GameSreen_Rect;
    int spriteLeft   = cx - sprite->frame.HotspotX;
    int spriteTop    = cy - sprite->frame.HotspotY;
    int spriteRight  = spriteLeft + sprite->frame.Width;
    int spriteBottom = spriteTop  + sprite->frame.Height;
    if (spriteRight <= gs.left || spriteLeft >= gs.right ||
        spriteBottom <= gs.top || spriteTop  >= gs.bottom)
        return;

    // Render mode toggle: when "Edges-only build preview" is ticked in
    // the Ctrl-F2 menu we render just the visible-edge wireframe in a
    // static palette index 250 (pure green), no fill, no animation. The
    // default is the filled silhouette + edge highlight, both colour-
    // cycled in TA's nanoframe ramp 0xa0..0xaf.
    bool edgesOnly = false;
    if (LocalShare && LocalShare->Dialog)
        edgesOnly = reinterpret_cast<Dialog*>(LocalShare->Dialog)->GetEdgesOnlyBuildPreview();

    auto rampColor = [](unsigned step) -> unsigned char {
        unsigned s = step & 0x1F;
        return (s & 0x10)
            ? static_cast<unsigned char>(0xaf - (s & 0xf))
            : static_cast<unsigned char>(0xa0 + (s & 0xf));
    };

    unsigned char fillColor = 0;
    unsigned char edgeColor = 0;
    if (edgesOnly)
    {
        edgeColor = 250;   // PALETTE.PAL: (0,255,0) pure green, no cycle
    }
    else
    {
        unsigned gameTime = static_cast<unsigned>(ta->GameTime);
        unsigned baseStep = (gameTime * 0x21u / 0x1eu) & 0x1F;
        fillColor = rampColor(baseStep);
        edgeColor = rampColor(baseStep + 16);   // 180° offset
    }

    // Z-plane sweep: a thin bright line travels through the model along
    // its LOCAL +Z axis (the unit's depth axis, +Z = front per the 3DO
    // convention). zCoord byte 1 = back, 255 = front; sweep walks low→
    // high so the line moves back→front. Because zCoord is captured
    // BEFORE the cardinal rotation, the line orientation and direction
    // both rotate with the structure: top→bottom for S-facing, left→
    // right for E-facing, bottom→top for N, right→left for W.
    const unsigned      kSweepFrames    = 30;   // full cycle period (frequency)
    const unsigned      kSweepActive    = kSweepFrames / 2;  // sweep first half; rest second half
    const int           kSweepHalfWidth = 2;    // ±2 byte units in zCoord
    const unsigned char kSweepColor     = 250;  // pure green — same line colour in both modes
    const unsigned char kSweepColorWire = 250;
    bool        sweepActive = false;
    unsigned    kByte = 0;
    {
        // Restart the sweep on each user rotation.
        unsigned gt = static_cast<unsigned>(ta->GameTime) - rotationCycleGameTime;
        unsigned phase = gt % kSweepFrames;
        if (phase < kSweepActive)
        {
            kByte = 1u + phase * 254u / (kSweepActive - 1);
            sweepActive = true;
        }
    }

    static thread_local std::vector<unsigned char> fillScratch;
    static thread_local std::vector<unsigned char> edgeScratch;
    size_t pxCount = sprite->pixels.size();
    // In wireframe mode we still allocate the fill scratch so we can
    // paint just the sweep-line slice across the silhouette interior;
    // every other fill pixel stays as colorKey (transparent).
    if (fillScratch.size() < pxCount) fillScratch.resize(pxCount);
    if (edgeScratch.size() < pxCount) edgeScratch.resize(pxCount);
    const unsigned char colorKey = sprite->frame.ColorKey;
    for (size_t i = 0; i < pxCount; ++i)
    {
        // Sweep-test once per pixel; reused by both the fill and (in
        // wireframe mode) the edge path. zCoord is 0 outside the
        // silhouette so the `z > 0` check stops the sweep painting
        // pixels that aren't on the visible surface.
        bool sweepHit = false;
        if (sweepActive)
        {
            int z = sprite->zCoord[i];
            if (z > 0)
            {
                int diff = z - (int)kByte;
                if (diff < 0) diff = -diff;
                sweepHit = (diff <= kSweepHalfWidth);
            }
        }

        if (sprite->pixels[i] == colorKey)
        {
            fillScratch[i] = colorKey;
        }
        else if (edgesOnly)
        {
            // Wireframe mode: paint ONLY sweep-line pixels across the
            // silhouette interior; everything else stays transparent
            // so the underlying terrain shows through.
            fillScratch[i] = sweepHit ? kSweepColorWire : colorKey;
        }
        else
        {
            fillScratch[i] = sweepHit ? kSweepColor : fillColor;
        }

        // Edges always cycle / stay static. The sweep line on the fill
        // already carries the visual cue; we don't separately recolour
        // the edges.
        edgeScratch[i] = (sprite->edgePixels[i] == colorKey) ? colorKey : edgeColor;
    }

    OFFSCREEN off;
    memset(&off, 0, sizeof(off));
    if (!kFn_LockAttackSurface(&off)) return;
    off.ScreenRect = gs;

    // Always blit the fill scratch. In fill mode this paints the full
    // shimmering silhouette; in wireframe mode the scratch is mostly
    // colorKey (transparent) and only the sweep-line pixels are
    // non-transparent, so just the line shows through across facets.
    GhostGAFFrame fillFrame = sprite->frame;
    fillFrame.PtrFrameBits = fillScratch.data();
    kFn_CopyGafToContext(&off, &fillFrame, cx, cy);

    GhostGAFFrame edgeFrame = sprite->frame;
    edgeFrame.PtrFrameBits = edgeScratch.data();
    edgeFrame.Bits2_Ptr    = nullptr;
    kFn_CopyGafToContext(&off, &edgeFrame, cx, cy);

    // First-use tutorial tip centred just above the build rectangle.
    // Shown only at the cursor (not at each line-build slot — that would be
    // spammy) until the player first cycles rotation by EITHER input path
    // (TryCycleRotation flips m_rotateBuildKeyDiscovered regardless of which
    // path triggered it).
    if (showNag && !m_rotateBuildKeyDiscovered && allowedCount >= 2)
    {
        int rotateKey = VK_OEM_2;
        int snapOverrideKey = VK_MENU;
        if (LocalShare && LocalShare->Dialog)
        {
            Dialog* dialog = reinterpret_cast<Dialog*>(LocalShare->Dialog);
            rotateKey       = dialog->GetRotateBuildKey();
            snapOverrideKey = dialog->GetClickSnapOverrideKey();
        }

        char keyName[32] = {0};
        char snapName[32] = {0};
        vkToStr(rotateKey,       keyName,  sizeof(keyName));
        vkToStr(snapOverrideKey, snapName, sizeof(snapName));

        char tip[128];
        sprintf_s(tip, "Press %s, or %s+wheel, to rotate", keyName, snapName);

        TAProgramStruct* programPtr = *reinterpret_cast<TAProgramStruct**>(0x0051fbd0);
        programPtr->fontHandle      = reinterpret_cast<unsigned char*>(ta->COMIXFontHandle);
        programPtr->fontFrontColour = ta->desktopGUI.RadarObjecColor[15];
        programPtr->fontBackColour  = programPtr->fontAlpha;
        int fontHeight = programPtr->fontHandle[0];

        int tipW = GetTextExtent(programPtr->fontHandle, tip);
        int tipX = cx - tipW / 2;

        // Top of the build rectangle in screen coords (Pos1 is the far
        // corner that projects to the rect's top edge).
        int rectTopY = (p1y - ta->EyeBallMapYPos) + 32 - (elev / 2);
        int tipY = rectTopY - fontHeight - 4;
        if (tipY < gs.top) tipY = gs.top;

        DrawTextInScreen(&off, tip, tipX, tipY, -1);
    }

    kFn_UnlockAttackedSurface(&off);
}
#else
void CBuildGhost::RenderGhostAtCurrentBuildSpot(bool /*showNag*/) { /* no-op outside FULL3D */ }
#endif
