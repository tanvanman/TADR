#ifndef buildghostH
#define buildghostH

#include "config.h"

#include <Windows.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct Model3DONode;

class SingleHook;

// =============================================================================
// Build-cursor "ghost" rendering style — chosen at RUN time from the
// totala.ini [Preferences] "NanoframePreview" option.
//
// All three styles share one rasteriser and one sprite cache; the split is
// purely a render-stage choice, so there is nothing to configure at compile
// time beyond which style a fresh install starts out with — see
// TDRAW_BUILDGHOST_STYLE_DEFAULT below.
// =============================================================================
enum class BuildGhostPreviewStyle
{
    // No model preview at all — only TA's native build rectangle. The
    // renderer is never entered and no FBI preview keys are registered.
    Disabled = 0,
    // Shimmering edge frame plus the z-plane scanline sweeping through it,
    // with no fill inside the silhouette.
    Wireframe = 1,
    // Wireframe plus the flat-shaded shimmering nanoframe fill, so the
    // preview looks like the nanoframe the building starts out as.
    Fill = 2,
};

// Default used when totala.ini names no style. Override in the per-mod
// config_*.h (pulled in by config.h above), e.g.
//   #define TDRAW_BUILDGHOST_STYLE_DEFAULT BuildGhostPreviewStyle::Disabled
// The macro is only expanded inside buildghost.cpp, so naming the enum before
// it is declared here is fine.
#ifndef TDRAW_BUILDGHOST_STYLE_DEFAULT
#define TDRAW_BUILDGHOST_STYLE_DEFAULT BuildGhostPreviewStyle::Wireframe
#endif

// State of the totala.ini [Preferences] "NanoframePreview" option. Accepts
// FULL / WIREFRAME / DISABLED; the superseded boolean key
// "NanoframePreviewFill" is still honoured when "NanoframePreview" is absent.
// Read once and cached — this sits in the per-frame render path — so a change
// takes effect at the next TA start.
BuildGhostPreviewStyle GetBuildGhostPreviewStyle();

// CBuildGhost — owns the per-(unitType, rotation) sprite cache and renders
// the placement-preview ghost from the 3DO model. Pure presentation: depends
// on CUnitRotate for the current rotation index + IsRotationAllowed; never
// mutates engine state.
//
// Singleton — one instance for the whole DLL. Constructed during ddraw
// init (after CUnitRotate so it can query rotation state), which is also
// where the ini style is latched: a Disabled instance registers nothing and
// never renders.
class CBuildGhost
{
public:
    // Meyers singleton — lazy-constructed on first call, destroyed at DLL
    // teardown. Caller gets a pointer (never null) to match the old API so
    // existing null-check sites keep working unchanged.
    static CBuildGhost* GetInstance();

    // Register FBI keys (PreviewPieces, PreviewPiecesS/E/N/W,
    // PreviewFaceOpponent, PreviewObject3D). See tdraw.txt for semantics.
    // Must run before TA loads unit defs.
    static void RegisterUnitDefKeys();

    // Drop caches that hold pointers into TA's per-game pools.
    void OnGameTeardown();

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
    // No-op when NanoframePreview=DISABLED.
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

    // GAFFrame layout (0x18 bytes total; matches engine GAFFrame struct).
    // Sprite_RemapColorsByDepthRange (0x458d30) reads exactly this layout:
    // pixel ptr at +0x10 and depth ptr at +0x14.
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
        unsigned char* Bits2_Ptr;       // depth buffer
    };

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
        std::vector<unsigned char> zCoord;      // per-pixel local-z (front/back) → 1..255
        std::vector<unsigned char> edgePixels;  // visible-edge overlay
    };

private:
    CBuildGhost();
    ~CBuildGhost() = default;

    void ReadRotateKeyDiscovered();
    void WriteRotateKeyDiscovered();

    bool m_rotateBuildKeyDiscovered = false;

    std::vector<std::shared_ptr<SingleHook>> m_hooks;

    const NanoframeSprite3D* GetNanoframeSprite3D(unsigned unitInfoIdx, int rotation);
    std::unordered_map<unsigned, NanoframeSprite3D> m_nanoframe3DCache;

    // Returns PreviewObject3D= override (loaded via HPI on first use)
    // when set, otherwise ta->MODEL_PTRS[unitInfoIdx].
    Model3DONode* GetPreviewModelRoot(unsigned unitInfoIdx);

    // Lowercase 3DO base name → root node, dropped on game teardown
    // (entries hold TA-owned pointers).
    std::unordered_map<std::string, Model3DONode*> m_overrideModelRoots;
};

#endif
