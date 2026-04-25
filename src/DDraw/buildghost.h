#ifndef buildghostH
#define buildghostH

#include <Windows.h>
#include <unordered_map>
#include <vector>

// =============================================================================
// Build-cursor "ghost" rendering mode — chosen at compile time. Override by
// defining TDRAW_BUILDGHOST_MODE on the command line (or in config.h) BEFORE
// this header is included.
//
//   TDRAW_BUILDGHOST_RECT      = 0  - No model preview, only TA's native
//                                     build rectangle. Cheapest. Original
//                                     behaviour before the ghost feature.
//   TDRAW_BUILDGHOST_WIREFRAME = 1  - Cached 2D wireframe sprite (silhouette
//                                     edges of the rotated 3DO model),
//                                     drawn via TA's color-keyed GAF blit.
//                                     Cheap per-frame, hundreds of queue
//                                     items OK.
//   TDRAW_BUILDGHOST_FULL3D    = 2  - Render the full 3D model into our own
//                                     pixel + depth buffers, flat-shade in
//                                     a cycling green ramp, overlay visible
//                                     edges. Default — the authentic look.
// =============================================================================
#define TDRAW_BUILDGHOST_RECT       0
#define TDRAW_BUILDGHOST_WIREFRAME  1
#define TDRAW_BUILDGHOST_FULL3D     2

#ifndef TDRAW_BUILDGHOST_MODE
#define TDRAW_BUILDGHOST_MODE TDRAW_BUILDGHOST_FULL3D
#endif

#if TDRAW_BUILDGHOST_MODE != TDRAW_BUILDGHOST_RECT && \
    TDRAW_BUILDGHOST_MODE != TDRAW_BUILDGHOST_WIREFRAME && \
    TDRAW_BUILDGHOST_MODE != TDRAW_BUILDGHOST_FULL3D
#error TDRAW_BUILDGHOST_MODE must be one of TDRAW_BUILDGHOST_{RECT,WIREFRAME,FULL3D}
#endif

// CBuildGhost — owns the per-(unitType, rotation) sprite cache and renders
// the placement-preview ghost from the 3DO model. Pure presentation: depends
// on CUnitRotate for the current rotation index + IsRotationAllowed; never
// mutates engine state.
//
// Singleton — one instance for the whole DLL. Constructed during ddraw
// init (after CUnitRotate so it can query rotation state). RECT mode is a
// no-op singleton; WIREFRAME / FULL3D modes own caches and helper data.
class CBuildGhost
{
public:
    // Meyers singleton — lazy-constructed on first call, destroyed at DLL
    // teardown. Caller gets a pointer (never null) to match the old API so
    // existing null-check sites keep working unchanged.
    static CBuildGhost* GetInstance();

    CBuildGhost(const CBuildGhost&) = delete;
    CBuildGhost& operator=(const CBuildGhost&) = delete;

    // Per-frame entry from CTAHook::TABlit. Renders the cursor ghost at
    // TA's current build-rect snap (CircleSelect_Pos1/Pos2). Skips when
    // CTAHook is in line-build mode — VisualizeRow renders one ghost per
    // row position via RenderGhostAtCurrentBuildSpot.
    void RenderNanoframeGhost();

    // Render one ghost at TA's current CircleSelect_Pos1/Pos2 build-rect
    // snap. Called from CTAHook::VisualizeRow inside the line-build loop
    // after each TestBuildSpot call so every rect in a shift-row gets
    // its own preview. Also used internally by RenderNanoframeGhost.
    // No-op outside FULL3D mode.
    //
    // showNag: if true AND the rotate-build key has not yet been discovered
    //   AND the current build unit allows more than one facing, draw a
    //   "Press <key> to rotate" tip above the build rectangle. The line-build
    //   caller passes false (tip shown only at the cursor, not at each slot).
    void RenderGhostAtCurrentBuildSpot(bool showNag = false);

    // Flag the rotate-build key as "discovered" — called by CUnitRotate on the
    // first successful cycle. Suppresses the tutorial nag permanently (persists
    // to HKEY_CURRENT_USER\<CompanyName>\Eye\RotateBuildKeyDiscovered).
    void SetRotateKeyDiscovered();
    bool IsRotateKeyDiscovered() const { return m_rotateBuildKeyDiscovered; }

#if TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_WIREFRAME || \
    TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_FULL3D

    // GAFFrame layout (0x18 bytes total; matches engine GAFFrame struct).
    // Reused by both wireframe and full-3D modes; the only difference is
    // whether Bits2_Ptr is null (wireframe) or points at a depth buffer
    // (full-3D). Sprite_RemapColorsByDepthRange (0x458d30) reads exactly
    // this layout: pixel ptr at +0x10 and depth ptr at +0x14.
    struct GhostGAFFrame
    {
        unsigned short Width;
        unsigned short Height;
        short          HotspotX;
        short          HotspotY;
        unsigned char  ColorKey;
        unsigned char  Compressed;
        unsigned char  SubFrames;
        unsigned char  AlphaBlend;
        int            Unknown_0C;
        unsigned char* PtrFrameBits;
        unsigned char* Bits2_Ptr;       // null in wireframe; depth buf in full-3D
    };
#endif

#if TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_WIREFRAME
    // 2D screen-space edge, relative to the unit's projected centre.
    struct Line2D { short x1, y1, x2, y2; };

    struct NanoframeSprite
    {
        GhostGAFFrame frame;
        std::vector<unsigned char> pixels;   // Width × Height, ColorKey = transparent
    };
#endif

#if TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_FULL3D
    // Cached full-3D sprite: flat-shaded silhouette + per-pixel depth.
    // Shared layout with the engine's GAFFrame so we can blit through
    // CopyGafToContext (0x4b7f90) and apply Sprite_RemapColorsByDepthRange
    // (0x458d30) without any wrapping.
    //
    // Pixel buffer:  ColorKey = transparent; otherwise = current ghost colour.
    // Depth  buffer: 0       = far / not written;
    //                255     = nearest;
    //                Mapped from world-y (elevation) over the model's y-extent.
    struct NanoframeSprite3D
    {
        GhostGAFFrame frame;                    // for the filled silhouette
        std::vector<unsigned char> pixels;      // fill pixel buffer
        std::vector<unsigned char> depth;       // per-pixel depth (world-y → 1..255)
        std::vector<unsigned char> edgePixels;  // visible-edge overlay
    };
#endif

private:
    CBuildGhost();
    ~CBuildGhost() = default;

    void ReadRotateKeyDiscovered();
    void WriteRotateKeyDiscovered();

    bool m_rotateBuildKeyDiscovered = false;

#if TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_WIREFRAME
    const NanoframeSprite* GetNanoframeSprite(unsigned unitInfoIdx, int rotation);
    std::unordered_map<unsigned, NanoframeSprite> m_nanoframeCache;
#endif

#if TDRAW_BUILDGHOST_MODE == TDRAW_BUILDGHOST_FULL3D
    const NanoframeSprite3D* GetNanoframeSprite3D(unsigned unitInfoIdx, int rotation);
    std::unordered_map<unsigned, NanoframeSprite3D> m_nanoframe3DCache;
#endif
};

#endif
