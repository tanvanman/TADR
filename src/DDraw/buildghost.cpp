#include "buildghost.h"
#include "unitrotate.h"

#include "dialog.h"
#include "iddrawsurface.h"
#include "tafunctions.h"
#include "tahook.h"
#include "tamem.h"
#include "hook/etc.h"

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

CBuildGhost::CBuildGhost()
{
    ReadRotateKeyDiscovered();
#if   TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_RECT
    IDDrawSurface::OutptTxt("[BuildGhost] mode=RECT (no model preview)");
#elif TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_WIREFRAME
    IDDrawSurface::OutptTxt("[BuildGhost] mode=WIREFRAME");
#elif TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_FULL3D
    IDDrawSurface::OutptTxt("[BuildGhost] mode=FULL3D");
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
// Shared 3DO helpers (WIREFRAME and FULL3D)
// =============================================================================
#if TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_WIREFRAME || \
    TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_FULL3D

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
// WIREFRAME mode
// =============================================================================
#if TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_WIREFRAME
namespace
{
    struct PieceFrame
    {
        Model3DONode* node;
        int origX_fxp, origY_fxp, origZ_fxp;
    };

    inline uint64_t EdgeKey(short x1, short y1, short x2, short y2)
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

    void BuildEdgeList(Model3DONode* rootNode, int rotation,
                       std::vector<CBuildGhost::Line2D>& out)
    {
        if (!rootNode) return;
        std::vector<short> sx, sy;
        std::vector<PieceFrame> stack;
        stack.reserve(32);
        stack.push_back({ rootNode, 0, 0, 0 });

        std::unordered_set<uint64_t> seenEdges;
        seenEdges.reserve(512);

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
                int pieceOrigX = f.origX_fxp + rOfsX;
                int pieceOrigY = f.origY_fxp + ofsY;
                int pieceOrigZ = f.origZ_fxp + rOfsZ;

                if (vertCount > 0 && vertArray)
                {
                    if (static_cast<int>(sx.size()) < vertCount)
                    {
                        sx.resize(vertCount);
                        sy.resize(vertCount);
                    }
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
                        sx[v] = static_cast<short>(psx);
                        sy[v] = static_cast<short>(psy);
                    }

                    // Silhouette-only emit.
                    if (faceCount > 0 && faceArray)
                    {
                        struct EdgeAdj { unsigned char frontCount, backCount; };
                        std::unordered_map<unsigned, EdgeAdj> pieceEdges;
                        pieceEdges.reserve(faceCount * 3);

                        for (int fi = 0; fi < faceCount; ++fi)
                        {
                            Model3DOFace& face = faceArray[fi];
                            int fVertCount = face.VertexCount;
                            unsigned short* idx = face.pVertexIndices;
                            if (fVertCount < 2 || !idx) continue;

                            bool isFront;
                            if (fVertCount < 3) isFront = true;
                            else
                            {
                                long long area2 = 0;
                                bool valid = true;
                                for (int i = 0; i < fVertCount; ++i)
                                {
                                    int a = idx[i];
                                    int b = idx[(i + 1) % fVertCount];
                                    if (a >= vertCount || b >= vertCount) { valid = false; break; }
                                    area2 += (long long)sx[a] * sy[b]
                                           - (long long)sx[b] * sy[a];
                                }
                                if (!valid) continue;
                                isFront = (area2 > 0);
                            }

                            for (int i = 0; i < fVertCount; ++i)
                            {
                                int a = idx[i];
                                int b = idx[(i + 1) % fVertCount];
                                if (a >= vertCount || b >= vertCount) continue;
                                unsigned lo = static_cast<unsigned>(a < b ? a : b);
                                unsigned hi = static_cast<unsigned>(a < b ? b : a);
                                unsigned key = (hi << 16) | lo;
                                EdgeAdj& adj = pieceEdges[key];
                                if (isFront) { if (adj.frontCount < 255) ++adj.frontCount; }
                                else         { if (adj.backCount  < 255) ++adj.backCount; }
                            }
                        }

                        for (const auto& kv : pieceEdges)
                        {
                            unsigned key = kv.first;
                            const EdgeAdj& adj = kv.second;
                            unsigned lo = key & 0xFFFFu;
                            unsigned hi = key >> 16;
                            if (lo >= (unsigned)vertCount || hi >= (unsigned)vertCount) continue;
                            bool silhouette = (adj.frontCount >= 1 && adj.backCount >= 1);
                            if (!silhouette) continue;
                            uint64_t ck = EdgeKey(sx[lo], sy[lo], sx[hi], sy[hi]);
                            if (!seenEdges.insert(ck).second) continue;
                            CBuildGhost::Line2D ln = { sx[lo], sy[lo], sx[hi], sy[hi] };
                            out.push_back(ln);
                        }
                    }
                }

                if (child)
                {
                    PieceFrame childFrame;
                    childFrame.node = child;
                    childFrame.origX_fxp = pieceOrigX;
                    childFrame.origY_fxp = pieceOrigY;
                    childFrame.origZ_fxp = pieceOrigZ;
                    stack.push_back(childFrame);
                }
                node = sibling;
            }
        }
    }

    // Bresenham line into 8bpp pixel buffer. No clipping (caller bounds).
    void RasterizeLine(unsigned char* pixels, int stride,
                       int x0, int y0, int x1, int y1,
                       unsigned char color)
    {
        int dx =  std::abs(x1 - x0);
        int dy = -std::abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        for (;;)
        {
            pixels[y0 * stride + x0] = color;
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }
}

const CBuildGhost::NanoframeSprite* CBuildGhost::GetNanoframeSprite(
    unsigned unitInfoIdx, int rotation)
{
    rotation &= 3;
    unsigned key = (unitInfoIdx << 2) | rotation;

    auto it = m_nanoframeCache.find(key);
    if (it != m_nanoframeCache.end())
        return it->second.frame.Width == 0 ? nullptr : &it->second;

    TAdynmemStruct* ta = GetTA();
    if (!ta || !ta->MODEL_PTRS) return nullptr;
    Model3DONode* rootNode = ta->MODEL_PTRS[unitInfoIdx];
    if (!rootNode)
    {
        m_nanoframeCache[key] = NanoframeSprite{};
        return nullptr;
    }

    std::vector<Line2D> edges;
    edges.reserve(256);
    BuildEdgeList(rootNode, rotation, edges);
    if (edges.empty())
    {
        m_nanoframeCache[key] = NanoframeSprite{};
        return nullptr;
    }

    short minX = SHRT_MAX, maxX = SHRT_MIN;
    short minY = SHRT_MAX, maxY = SHRT_MIN;
    for (const Line2D& ln : edges)
    {
        if (ln.x1 < minX) minX = ln.x1; if (ln.x1 > maxX) maxX = ln.x1;
        if (ln.x2 < minX) minX = ln.x2; if (ln.x2 > maxX) maxX = ln.x2;
        if (ln.y1 < minY) minY = ln.y1; if (ln.y1 > maxY) maxY = ln.y1;
        if (ln.y2 < minY) minY = ln.y2; if (ln.y2 > maxY) maxY = ln.y2;
    }

    int w = maxX - minX + 1;
    int h = maxY - minY + 1;
    if (w <= 0 || h <= 0 || w > 1024 || h > 1024)
    {
        m_nanoframeCache[key] = NanoframeSprite{};
        return nullptr;
    }

    NanoframeSprite& out = m_nanoframeCache[key];
    out.pixels.assign(static_cast<size_t>(w) * static_cast<size_t>(h), 0u);

    const unsigned char kColorKey  = 0;
    const unsigned char kLineColor = 234;

    for (const Line2D& ln : edges)
    {
        int x0 = ln.x1 - minX;
        int y0 = ln.y1 - minY;
        int x1 = ln.x2 - minX;
        int y1 = ln.y2 - minY;
        RasterizeLine(out.pixels.data(), w, x0, y0, x1, y1, kLineColor);
    }

    out.frame.Width        = static_cast<unsigned short>(w);
    out.frame.Height       = static_cast<unsigned short>(h);
    out.frame.HotspotX     = static_cast<short>(-minX);
    out.frame.HotspotY     = static_cast<short>(-minY);
    out.frame.ColorKey     = kColorKey;
    out.frame.Compressed   = 0;
    out.frame.SubFrames    = 0;
    out.frame.AlphaBlend   = 0;
    out.frame.Unknown_0C   = 0;
    out.frame.PtrFrameBits = out.pixels.data();
    out.frame.Bits2_Ptr    = nullptr;

    IDDrawSurface::OutptFmtTxt(
        "[BuildGhost-WIRE] cached %dx%d (%u edges) typeIdx=%u rot=%d",
        w, h, (unsigned)edges.size(), unitInfoIdx, rotation);
    return &out;
}
#endif // WIREFRAME

// =============================================================================
// FULL3D mode
// =============================================================================
#if TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_FULL3D
namespace
{
    struct ProjVert { int sx, sy; int depth; };

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

    // Convex polygon scanline fill with z-test.
    void FillConvexPolyZ(unsigned char* colorBuf,
                         unsigned char* depthBuf,
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
            int leftZ = 0, rightZ = 0;
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
                int x  = verts[i].sx + (verts[j].sx - verts[i].sx) * t / dy;
                int z  = verts[i].depth + (verts[j].depth - verts[i].depth) * t / dy;
                if (x < leftX)  { leftX  = x; leftZ  = z; }
                if (x > rightX) { rightX = x; rightZ = z; }
            }
            if (leftX > rightX) continue;
            int spanLeft  = leftX  < 0 ? 0 : leftX;
            int spanRight = rightX >= bufW ? bufW - 1 : rightX;
            if (spanLeft > spanRight) continue;

            int spanW = rightX - leftX;
            int rowBase = y * bufW;
            for (int x = spanLeft; x <= spanRight; ++x)
            {
                int z = (spanW > 0)
                    ? leftZ + (rightZ - leftZ) * (x - leftX) / spanW
                    : leftZ;
                if (z < 0)   z = 0;
                if (z > 255) z = 255;
                int idx = rowBase + x;
                if ((unsigned char)z >= depthBuf[idx])
                {
                    depthBuf[idx] = (unsigned char)z;
                    colorBuf[idx] = polyColor;
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

    struct PieceFrame { Model3DONode* node; int origX, origY, origZ; };
    std::vector<PieceFrame> stack;
    stack.reserve(32);
    stack.push_back({ rootNode, 0, 0, 0 });

    std::vector<int> projVx, projVy, projVz_world_y;

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

            if (vertCount > 0 && vertArray && faceCount > 0 && faceArray)
            {
                projVx.resize(vertCount);
                projVy.resize(vertCount);
                projVz_world_y.resize(vertCount);
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
                    projVz_world_y[v] = wy_int;
                    if (psx < minSX) minSX = psx;
                    if (psx > maxSX) maxSX = psx;
                    if (psy < minSY) minSY = psy;
                    if (psy > maxSY) maxSY = psy;
                    if (wy_int < minWY_int) minWY_int = wy_int;
                    if (wy_int > maxWY_int) maxWY_int = wy_int;
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
                        pv.depth = projVz_world_y[a];
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
                        StashedEdge se;
                        se.a.sx = projVx[a]; se.a.sy = projVy[a];
                        se.a.depth = projVz_world_y[a];
                        se.b.sx = projVx[b]; se.b.sy = projVy[b];
                        se.b.depth = projVz_world_y[b];
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
    out.edgePixels.assign(static_cast<size_t>(w) * static_cast<size_t>(h), 0u);

    int wyRange = maxWY_int - minWY_int;
    if (wyRange <= 0) wyRange = 1;

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
            int d = ((pv.depth - minWY_int) * 254 + (wyRange / 2)) / wyRange + 1;
            if (d < 1)   d = 1;
            if (d > 255) d = 255;
            r.depth = d;
            remapped.push_back(r);
        }
        FillConvexPolyZ(out.pixels.data(), out.depth.data(),
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

#elif TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_WIREFRAME
    TAdynmemStruct* ta = GetTA();
    if (!ta) return;
    if (ta->PrepareOrder_Type != ordertype::BUILD) return;

    unsigned buildIdx = static_cast<unsigned>(ta->BuildUnitID);
    if (buildIdx == 0) return;

    UnitDefStruct* ui = GetUnitDef(buildIdx);
    if (!ui) return;
    if (ui->bmcode != 0) return;

    const RECT& gs = ta->GameSreen_Rect;
    int mapX = ta->MouseMapPos.X;
    int mapY = ta->MouseMapPos.Y;
    int mapZ = ta->MouseMapPos.Z;

    int rotation = 0;
    if (CUnitRotate* rot = CUnitRotate::GetInstance())
    {
        rotation = rot->GetRotation() & 3;
        if (!rot->IsRotationAllowed(buildIdx, rotation)) rotation = 0;
    }

    short footX = ui->FootX;
    short footY = ui->FootY;
    if ((rotation & 1) == 1) { short t = footX; footX = footY; footY = t; }

    int snappedX = (footX & 1) ? ((mapX / 16) * 16 + 8) : (((mapX + 8) / 16) * 16);
    int snappedY = (footY & 1) ? ((mapY / 16) * 16 + 8) : (((mapY + 8) / 16) * 16);
    int cx = (snappedX - ta->EyeBallMapXPos) + 128;
    int cy = (snappedY - ta->EyeBallMapYPos) + 32 - (mapZ / 2);
    if (cx < gs.left || cx >= gs.right || cy < gs.top || cy >= gs.bottom) return;

    const NanoframeSprite* sprite = GetNanoframeSprite(buildIdx, rotation);
    if (!sprite) return;

    OFFSCREEN off;
    memset(&off, 0, sizeof(off));
    if (!kFn_LockAttackSurface(&off)) return;
    off.ScreenRect = gs;
    kFn_CopyGafToContext(&off, &sprite->frame, cx, cy);
    kFn_UnlockAttackedSurface(&off);

#elif TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_FULL3D
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

    int  rotation     = 0;
    int  allowedCount = 1;
    if (CUnitRotate* rot = CUnitRotate::GetInstance())
    {
        rotation = rot->GetRotation() & 3;
        if (!rot->IsRotationAllowed(buildIdx, rotation)) rotation = 0;
        allowedCount = 0;
        for (int r = 0; r < 4; ++r)
            if (rot->IsRotationAllowed(buildIdx, r)) ++allowedCount;
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

    // Animated colours from TA's nanoframe ramp 0xa0..0xaf, tied to
    // GameTime via DrawUnitNanoFrame's formula. All ghosts in a frame
    // share these → pulse stays synchronised across line-build positions.
    auto rampColor = [](unsigned step) -> unsigned char {
        unsigned s = step & 0x1F;
        return (s & 0x10)
            ? static_cast<unsigned char>(0xaf - (s & 0xf))
            : static_cast<unsigned char>(0xa0 + (s & 0xf));
    };
    unsigned gameTime = static_cast<unsigned>(ta->GameTime);
    unsigned baseStep = (gameTime * 0x21u / 0x1eu) & 0x1F;
    unsigned char fillColor = rampColor(baseStep);
    unsigned char edgeColor = rampColor(baseStep + 16);   // 180° offset

    static thread_local std::vector<unsigned char> fillScratch;
    static thread_local std::vector<unsigned char> edgeScratch;
    size_t pxCount = sprite->pixels.size();
    if (fillScratch.size() < pxCount) fillScratch.resize(pxCount);
    if (edgeScratch.size() < pxCount) edgeScratch.resize(pxCount);
    const unsigned char colorKey = sprite->frame.ColorKey;
    for (size_t i = 0; i < pxCount; ++i)
    {
        fillScratch[i] = (sprite->pixels[i]     == colorKey) ? colorKey : fillColor;
        edgeScratch[i] = (sprite->edgePixels[i] == colorKey) ? colorKey : edgeColor;
    }

    OFFSCREEN off;
    memset(&off, 0, sizeof(off));
    if (!kFn_LockAttackSurface(&off)) return;
    off.ScreenRect = gs;

    GhostGAFFrame fillFrame = sprite->frame;
    fillFrame.PtrFrameBits = fillScratch.data();
    kFn_CopyGafToContext(&off, &fillFrame, cx, cy);

    GhostGAFFrame edgeFrame = sprite->frame;
    edgeFrame.PtrFrameBits = edgeScratch.data();
    edgeFrame.Bits2_Ptr    = nullptr;
    kFn_CopyGafToContext(&off, &edgeFrame, cx, cy);

    // First-use tutorial tip: "Press <key> to rotate" centred just above
    // the build rectangle. Shown only at the cursor (not at each line-build
    // slot — that would be spammy) until the player first cycles rotation.
    if (showNag && !m_rotateBuildKeyDiscovered && allowedCount >= 2)
    {
        int rotateKey = VK_OEM_2;
        if (LocalShare && LocalShare->Dialog)
            rotateKey = reinterpret_cast<Dialog*>(LocalShare->Dialog)->GetRotateBuildKey();

        char keyName[32] = {0};
        vkToStr(rotateKey, keyName, sizeof(keyName));

        char tip[96];
        sprintf_s(tip, "Press %s to rotate", keyName);

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
