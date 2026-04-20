#include "NotToAir.h"
#include "WeaponTdfHook.h"
#include "tamem.h"

// -----------------------------------------------------------------------
// UnitAutoAim_CheckUnitWeapon hook @ 0x49AD07
//   Opcode: A9 00 00 02 00  (test eax, 20000h)  -- 5 bytes, position-independent
//   eax = WeaponTypeMask, ebx = TargetUnitPtr
//
//   If weapon has WTM_NotToAir (bit 31) AND target UnitStateMask & 3 == 2 (flying),
//   redirect to the existing reject path at 0x49AD1C which returns 0 to the caller.
// -----------------------------------------------------------------------
static const DWORD kCheckHookAddr   = 0x49AD07u;
static const DWORD kRejectAddr      = 0x49AD1Cu;   // xor eax,eax + frame teardown + retn 0Ch
static const DWORD kUnitSelectedOff = 0x110u;       // UnitStruct::UnitSelected (IDA: UnitStateMask)
static const DWORD kWeaponMaskOff   = 0x111u;       // WeaponTypedef::WeaponTypeMask

static const char kNotToAirKey[]    = "nottoair";

// -----------------------------------------------------------------------

NotToAir* NotToAir::m_instance = nullptr;

void NotToAir::Install()
{
    if (!m_instance)
        m_instance = new NotToAir();
}

NotToAir::NotToAir()
{
    m_weaponCheckHook.reset(new InlineSingleHook(
        kCheckHookAddr, 5,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)CheckRouter));

    WeaponTdfHook::Register([](const WeaponTdfHook::Context& ctx) {
        if (ctx.getInt(kNotToAirKey) & 1)
        {
            DWORD* pMask = (DWORD*)((BYTE*)ctx.pWeaponDef + kWeaponMaskOff);
            *pMask |= WTM_NotToAir;
        }
    });
}

NotToAir::~NotToAir()
{
    m_weaponCheckHook.reset();
}

// Targeting gate: rejects flying targets for nottoair weapons.
int __stdcall NotToAir::CheckRouter(PInlineX86StackBuffer pBuf)
{
    if (pBuf->Eax & WTM_NotToAir)
    {
        DWORD unitSelected = *(DWORD*)((BYTE*)pBuf->Ebx + kUnitSelectedOff);
        if ((unitSelected & 3u) == 2u)
        {
            // nottoair weapon + flying target: early return 0
            pBuf->rtnAddr_Pvoid = (LPVOID)kRejectAddr;
            return X86STRACKBUFFERCHANGE;
        }
    }
    return 0;
}
