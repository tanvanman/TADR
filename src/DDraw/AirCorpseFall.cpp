#include "AirCorpseFall.h"
#include "tamem.h"
#include "tafunctions.h"

// -----------------------------------------------------------------------
// UNITS_CreateCorpse @ 0x486439
//   Bytes: MOV EAX,[ESP+0x1C]  TEST EAX,EAX  (8B 44 24 1C 85 C0) -- 6 bytes.
//   EAX is still FEATURES_PlaceFeatureOnMap's return value at hook entry: the new
//   feature-anim node, or NULL. All three incoming paths (land fall-through, and the
//   water branches at 0x486402 / 0x486424) land on 0x486439 itself, and nothing
//   branches into 0x48643B..0x48643E.
//
// Feature-anim nodes are a union. For features with FeatureDef.Flags bit0 == 0 these
// bytes are position and velocity, not the GAFSpriteStates Ghidra shows.
// -----------------------------------------------------------------------
static const DWORD kPlacedHookAddr = 0x486439u;
static const DWORD kPlacedHookLen  = 6u;

static const DWORD kNodePosOff = 0x08u;   // pos_x, pos_y, pos_z
static const DWORD kNodeVelOff = 0x14u;   // vel_x, vel_y, vel_z

static const DWORD kSeaLevelOff = 0x1427Fu;
static const DWORD kTADynMemPtr = 0x00511DE8u;

// Any non-zero value defeats the all-zero retirement test at 0x424214; from the next
// tick the integrator subtracts TAdynmemStruct::Gravity, so the wreck falls from rest.
static const int kSeedFallVelY = -1;

// -----------------------------------------------------------------------

AirCorpseFall* AirCorpseFall::m_instance = nullptr;

void AirCorpseFall::Install()
{
    if (!m_instance)
        m_instance = new AirCorpseFall();
}

AirCorpseFall::AirCorpseFall()
{
    m_placedHook.reset(new InlineSingleHook(
        kPlacedHookAddr, kPlacedHookLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)PlacedRouter));
}

AirCorpseFall::~AirCorpseFall()
{
    m_placedHook.reset();
}

int __stdcall AirCorpseFall::PlacedRouter(PInlineX86StackBuffer pBuf)
{
    BYTE* node = (BYTE*)pBuf->Eax;
    if (!node)
        return 0;

    int* vel = (int*)(node + kNodeVelOff);
    if (vel[0] || vel[1] || vel[2])
        return 0;   // the water branch already seeded a sink velocity

    // Read the node's own stored position rather than a register: EDI differs between
    // the land and water paths (ADD EDI,0x64 at 0x48642C).
    const int h = GetPosHeight((Position_Dword*)(node + kNodePosOff));
    if (h < 0)
        return 0;

    const int seaLevel = *(BYTE*)(*(DWORD*)kTADynMemPtr + kSeaLevelOff);
    if (h <= seaLevel)
        return 0;   // over water: vanilla handles it

    // Ground units' corpses are already at terrain height, so they bail here and
    // behave exactly as before. Only wrecks spawned above the terrain get moved.
    if (*(int*)(node + kNodePosOff + 4) <= (h << 16))
        return 0;

    vel[1] = kSeedFallVelY;
    return 0;
}
