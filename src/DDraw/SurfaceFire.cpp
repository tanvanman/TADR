#include "SurfaceFire.h"
#include "WeaponTdfHook.h"
#include "tamem.h"
#include <unordered_set>

// -----------------------------------------------------------------------
// UnitAutoAim_CheckUnitWeapon water-path REJECT 1 hook @ 0x49AC0F
//   Bytes: XOR EAX,EAX (33 C0)  POP EDI (5F)  POP ESI (5E)  POP EBP (5D) -- 5 bytes
//   EDI = pWeaponDef (not yet popped at hook entry)
//   Fires when: waterweapon set AND target has no sub flag (0x80000) AND target Y > SeaLevel.
//   This is the rejection for plain land/surface units.
//   For surfacefire: redirect to range check at 0x49AC47 instead.
// -----------------------------------------------------------------------
static const DWORD kRejectHookAddr = 0x49AC0Fu;
static const DWORD kRejectHookLen  = 5u;

// -----------------------------------------------------------------------
// UnitAutoAim_CheckUnitWeapon water-path REJECT 2 hook @ 0x49AC20
//   EDI = pWeaponDef
//   Fires when: waterweapon set AND target has UnitTypeMask_0 & 0x1000 (e.g. hovercraft)
//   AND target Y > SeaLevel. For surfacefire: redirect to range check at 0x49AC47.
// -----------------------------------------------------------------------
static const DWORD kCheckHookAddr  = 0x49ac20u;
static const DWORD kCheckHookLen   = 5u;

static const DWORD kRangeCheckAddr    = 0x49AC47u;
static const DWORD kWeaponMaskOff     = 0x111u;   // WeaponTypedef::WeaponTypeMask
static const DWORD kUnitStateMaskOff  = 0x110u;   // UnitStruct::UnitStateMask

// -----------------------------------------------------------------------
// UnitAutoAim_CheckUnitWeapon waterweapon range-check gate @ 0x49AC47 (== kRangeCheckAddr)
//   Bytes: MOV ECX,[EAX+0x72]  MOV EAX,[EAX+0x6a]  (8B 48 72 8B 40 6A) -- 6 bytes,
//          position-independent. EAX = target UnitStruct*, EDI = WeaponTypedef*.
//   This is the single convergence point every "allow" path of the waterweapon
//   branch funnels into before the range^2 check -- including the paths CheckRouter
//   redirects here to bypass REJECT 1 / REJECT 2. A submerged submarine reaches it
//   too, having sailed past both reject branches via its sub flag.
//
//   For a nottounderwater weapon, reject a fully-submerged target here, i.e.
//   topY = target.Pos.y_hi + UNITINFO.lFpBndTop_Y_hi <= SeaLevel. This is the exact
//   submerged test the non-waterweapon branch applies natively at 0x49ACF7 (so plain
//   non-waterweapons honour nottounderwater without a hook); the gate is only needed
//   for waterweapons, which otherwise reach here unconditionally.
//
//   The rejection redirects to the REJECT 2 teardown at 0x49AC3B, NOT the identical
//   REJECT 1 teardown at 0x49AC0F. CheckRouter hooks 0x49AC0F and bounces a
//   surfacefire weapon back here to 0x49AC47; rejecting to 0x49AC0F would therefore
//   ping-pong 0x49AC47 -> 0x49AC0F -> 0x49AC47 forever (hard game freeze) for a weapon
//   that is both surfacefire=1 and nottounderwater=1. 0x49AC3B is not on that path.
// -----------------------------------------------------------------------
static const DWORD kRangeCheckHookLen   = 6u;
static const DWORD kSubmergedRejectAddr = 0x49AC3Bu;  // REJECT 2 teardown (see note above)

static const DWORD kPosYHiOff      = 0x70u;    // UnitStruct: high word of Pos.y
static const DWORD kUnitInfoPtrOff = 0x92u;    // UnitStruct::UNITINFO_p
static const DWORD kBndTopYHiOff   = 0x170u;   // UNITINFO: high word of lFpBndTop_Y
static const DWORD kSeaLevelOff    = 0x1427Fu; // TAdynmemStruct: SeaLevel (byte)
static const DWORD kTADynMemPtr    = 0x00511DE8u;

// -----------------------------------------------------------------------
// WeaponCanAim hook @ 0x49AB18
//   Bytes: MOV EAX,[EBX+0x111] (8B 83 11 01 00 00) -- 6 bytes, position-independent
//   EBX = WeaponTypedef*
//   Fires at the start of the waterweapon/depth check sequence.
//   For waterweapon=0: vanilla code rejects if the firer is submerged.
//   For surfacefire: redirect to 0x49AB9E (the can-aim success path that
//   waterweapon=1 normally reaches via JNZ at 0x49AB26).
// -----------------------------------------------------------------------
static const DWORD kCanAimHookAddr    = 0x49AB18u;
static const DWORD kCanAimHookLen     = 6u;
static const DWORD kCanAimSuccessAddr = 0x49AB9Eu;

// -----------------------------------------------------------------------
// ScriptAction_Type2Index hook @ 0x43F24F
//   Bytes: TEST [ESI+0x111],EDI (85 BE 11 01 00 00) -- 6 bytes, position-independent
//   ESI = weapon0 WeaponTypedef*, EDI = 0x10000 (waterweapon bit)
//   Fires when: the firing unit is a submarine (UnitTypeMask_0 & 0x1000) AND
//   the target is above sea level. Vanilla code rejects if weapon0 is a waterweapon,
//   preventing submarines from issuing an ATTACK COB action at surface targets.
//   For surfacefire: redirect to 0x43F27E (the allow path) instead.
// -----------------------------------------------------------------------
static const DWORD kScriptHookAddr  = 0x43F24Fu;
static const DWORD kScriptHookLen   = 6u;
static const DWORD kScriptAllowAddr = 0x43F27Eu;

// -----------------------------------------------------------------------
// ProjectilesEngine guidance gate hook @ 0x49B9EB
//   Bytes: TEST EAX,0x10000 (A9 00 00 01 00) -- 5 bytes, position-independent
//   EAX = WeaponTypeMask, ESI = WeaponTypedef*
//   Fires every tick for self-propelled projectiles (bit 20 set) while alive.
//   When waterweapon=1 AND projectile Y >= SeaLevel: kills all guidance —
//   applies gravity only, zeros ElevationAngle, skips Projectile_Cruise/Turn.
//   This prevents two-phase/VLaunch missiles from homing during the second stage.
//   For surfacefire: redirect to 0x49BA16 (normal guidance path) so the missile
//   continues to steer toward its target above sea level.
// -----------------------------------------------------------------------
static const DWORD kGuidanceHookAddr    = 0x49B9EBu;
static const DWORD kGuidanceHookLen     = 5u;
static const DWORD kGuidanceNormalAddr  = 0x49BA16u;

// -----------------------------------------------------------------------

static const char kSurfaceFireKey[]     = "surfacefire";
static const char kNotToUnderwaterKey[] = "nottounderwater";

// Weapon defs (WeaponTypedef*) that have surfacefire=1 / nottounderwater=1 in their TDF.
// Populated at load time by the WeaponTdfHook handler; read at runtime by the routers.
static std::unordered_set<DWORD> s_surfaceFireWeapons;
static std::unordered_set<DWORD> s_notToUnderwaterWeapons;

// -----------------------------------------------------------------------

SurfaceFire* SurfaceFire::m_instance = nullptr;

void SurfaceFire::Install()
{
    if (!m_instance)
        m_instance = new SurfaceFire();
}

SurfaceFire::SurfaceFire()
{
    m_rejectHook.reset(new InlineSingleHook(
        kRejectHookAddr, kRejectHookLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)CheckRouter));

    m_weaponCheckHook.reset(new InlineSingleHook(
        kCheckHookAddr, kCheckHookLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)CheckRouter));

    m_canAimHook.reset(new InlineSingleHook(
        kCanAimHookAddr, kCanAimHookLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)CanAimRouter));

    m_scriptActionHook.reset(new InlineSingleHook(
        kScriptHookAddr, kScriptHookLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)ScriptActionRouter));

    m_guidanceHook.reset(new InlineSingleHook(
        kGuidanceHookAddr, kGuidanceHookLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)GuidanceRouter));

    m_rangeCheckHook.reset(new InlineSingleHook(
        kRangeCheckAddr, kRangeCheckHookLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)RangeCheckRouter));

    WeaponTdfHook::Register([](const WeaponTdfHook::Context& ctx) {
        if (ctx.getInt(kSurfaceFireKey) & 1)
            s_surfaceFireWeapons.insert((DWORD)ctx.pWeaponDef);
        if (ctx.getInt(kNotToUnderwaterKey) & 1)
            s_notToUnderwaterWeapons.insert((DWORD)ctx.pWeaponDef);
    });
}

SurfaceFire::~SurfaceFire()
{
    m_rejectHook.reset();
    m_weaponCheckHook.reset();
    m_canAimHook.reset();
    m_scriptActionHook.reset();
    m_guidanceHook.reset();
    m_rangeCheckHook.reset();
}

// UnitAutoAim_CheckUnitWeapon: bypass water-path surface rejection → range check.
// Also respects nottoair: if the weapon has WTM_NotToAir and the target is flying,
// let the rejection proceed (don't allow surfacefire to override nottoair).
// At both hook sites: EAX = pTargetUnit, EDI = pWeaponDef.
int __stdcall SurfaceFire::CheckRouter(PInlineX86StackBuffer pBuf)
{
    if (s_surfaceFireWeapons.count(pBuf->Edi))
    {
        DWORD weaponMask = *(DWORD*)((BYTE*)pBuf->Edi + kWeaponMaskOff);
        if (weaponMask & WTM_NotToAir)
        {
            DWORD stateMask = *(DWORD*)((BYTE*)pBuf->Eax + kUnitStateMaskOff);
            if ((stateMask & 3u) == 2u)
                return 0;   // nottoair + flying target: let rejection proceed
        }

        pBuf->rtnAddr_Pvoid = (LPVOID)kRangeCheckAddr;
        return X86STRACKBUFFERCHANGE;
    }
    return 0;
}

// WeaponCanAim: allow surfacefire weapons to aim regardless of firer depth.
int __stdcall SurfaceFire::CanAimRouter(PInlineX86StackBuffer pBuf)
{
    if (s_surfaceFireWeapons.count(pBuf->Ebx))
    {
        pBuf->rtnAddr_Pvoid = (LPVOID)kCanAimSuccessAddr;
        return X86STRACKBUFFERCHANGE;
    }
    return 0;
}

// ScriptAction_Type2Index: allow submarine to issue ATTACK at surface target
// when weapon0 has surfacefire=1.
int __stdcall SurfaceFire::ScriptActionRouter(PInlineX86StackBuffer pBuf)
{
    if (s_surfaceFireWeapons.count(pBuf->Esi))
    {
        pBuf->rtnAddr_Pvoid = (LPVOID)kScriptAllowAddr;
        return X86STRACKBUFFERCHANGE;
    }
    return 0;
}

// ProjectilesEngine: allow surfacefire missile guidance above sea level.
// ESI = WeaponTypedef* for the current projectile.
int __stdcall SurfaceFire::GuidanceRouter(PInlineX86StackBuffer pBuf)
{
    if (s_surfaceFireWeapons.count(pBuf->Esi))
    {
        pBuf->rtnAddr_Pvoid = (LPVOID)kGuidanceNormalAddr;
        return X86STRACKBUFFERCHANGE;
    }
    return 0;
}

// UnitAutoAim_CheckUnitWeapon: waterweapon range-check convergence @ 0x49AC47.
// For a nottounderwater weapon, reject a fully-submerged target (topY <= SeaLevel).
// EAX = target UnitStruct*, EDI = WeaponTypedef*. See the kRangeCheckAddr block above
// for why the reject redirects to 0x49AC3B and not 0x49AC0F.
int __stdcall SurfaceFire::RangeCheckRouter(PInlineX86StackBuffer pBuf)
{
    if (s_notToUnderwaterWeapons.count(pBuf->Edi))
    {
        BYTE* pTarget  = (BYTE*)pBuf->Eax;
        DWORD unitInfo = *(DWORD*)(pTarget + kUnitInfoPtrOff);
        int   topY     = (int)*(short*)(pTarget + kPosYHiOff)
                       + (int)*(short*)((BYTE*)unitInfo + kBndTopYHiOff);
        int   seaLevel = *(BYTE*)(*(DWORD*)kTADynMemPtr + kSeaLevelOff);

        if (topY <= seaLevel)
        {
            // nottounderwater weapon + fully-submerged target: reject (return 0).
            pBuf->rtnAddr_Pvoid = (LPVOID)kSubmergedRejectAddr;
            return X86STRACKBUFFERCHANGE;
        }
    }
    return 0;
}
