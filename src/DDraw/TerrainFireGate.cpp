#include "TerrainFireGate.h"
#include "WeaponTags.h"
#include "tamem.h"
#include "tafunctions.h"

// -----------------------------------------------------------------------
// UNITS_AutoAim per-slot loop hook @ 0x49E1D6
//   Bytes: MOV EAX,[ESP+0x10]  LEA ECX,[ESP+0x28]
//          (8B 44 24 10 8D 4C 24 28) -- 8 bytes. ESP-relative, which the
//          LAGGERJMP trampoline restores faithfully (SurfaceFire's hook at
//          0x49AC0F steals three POPs and relies on the same thing).
//   EBX = WeaponTypedef*, EDI = firing UnitStruct*, ESI = &UnitWeapon.cStateMask
//   Nothing branches into 0x49E1D7..0x49E1DD; the JBE at 0x49E1CF lands on
//   0x49E1D6 itself.
// -----------------------------------------------------------------------
static const DWORD kAutoAimHookAddr = 0x49E1D6u;
static const DWORD kAutoAimHookLen  = 8u;
static const DWORD kNextSlotAddr    = 0x49E541u;  // loop increment

static const DWORD kUnitPosOff  = 0x6Au;      // UnitStruct::Pos
static const DWORD kSeaLevelOff = 0x1427Fu;   // TAdynmemStruct::SeaLevel (byte)
static const DWORD kTADynMemPtr = 0x00511DE8u;

static const char kNotOverWaterKey[] = "notoverwater";
static const char kNotOverLandKey[]  = "notoverland";

static const uint32_t kTerrainGateTags =
    (uint32_t)WeaponTags::NotOverWater | (uint32_t)WeaponTags::NotOverLand;

// -----------------------------------------------------------------------

TerrainFireGate* TerrainFireGate::m_instance = nullptr;

void TerrainFireGate::Install()
{
    if (!m_instance)
        m_instance = new TerrainFireGate();
}

TerrainFireGate::TerrainFireGate()
{
    WeaponTags::RegisterKey(kNotOverWaterKey, WeaponTags::NotOverWater);
    WeaponTags::RegisterKey(kNotOverLandKey,  WeaponTags::NotOverLand);

    // One hook serves both keys, so either one is enough to install it.
    WeaponTags::OnFirstUse(WeaponTags::NotOverWater, &InstallHooks);
    WeaponTags::OnFirstUse(WeaponTags::NotOverLand,  &InstallHooks);
}

void TerrainFireGate::InstallHooks()
{
    if (!m_instance || m_instance->m_autoAimHook)
        return;

    m_instance->m_autoAimHook.reset(new InlineSingleHook(
        kAutoAimHookAddr, kAutoAimHookLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)FireGateRouter));
}

TerrainFireGate::~TerrainFireGate()
{
    m_autoAimHook.reset();
}

// GetPosHeight reads only Pos.x and Pos.z, so for an airborne unit it returns the
// terrain height underneath it. This is the engine's own over-water test -- see
// UNITS_CreateCorpse @ 0x4863D0 and AirOrder_SetCruiseAltitude @ 0x44E6EA.
int __stdcall TerrainFireGate::FireGateRouter(PInlineX86StackBuffer pBuf)
{
    const uint32_t tags = WeaponTags::Get((const void*)pBuf->Ebx);
    if (!(tags & kTerrainGateTags))
        return 0;

    const int h = GetPosHeight((Position_Dword*)(pBuf->Edi + kUnitPosOff));
    if (h < 0)
        return 0;   // off-map: GetPosHeight returns -1, which would read as water

    const int seaLevel = *(BYTE*)(*(DWORD*)kTADynMemPtr + kSeaLevelOff);
    const uint32_t blocking = (h <= seaLevel) ? (uint32_t)WeaponTags::NotOverWater
                                              : (uint32_t)WeaponTags::NotOverLand;
    if (!(tags & blocking))
        return 0;

    pBuf->rtnAddr_Pvoid = (LPVOID)kNextSlotAddr;
    return X86STRACKBUFFERCHANGE;
}
