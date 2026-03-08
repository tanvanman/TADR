#include "NotToAir.h"
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

// -----------------------------------------------------------------------
// LoadWeaponTdf hook @ 0x42E4AB
//   Opcode: 8D 4D 20  51  8B CB  -- 6 bytes, all position-independent
//           lea ecx,[ebp+20h]  push ecx  mov ecx,ebx
//   ebx = TdfFile* (loaded @ 0x42E447, unchanged to this point)
//   ebp = WeaponTypedef* (set @ 0x42E489, unchanged to this point)
//
//   Called early in the function, before any WeaponTypeMask bits are set.
//   Every subsequent bit operation uses read-modify-write (and reg, not MASK; or)
//   which leaves all other bits, including WTM_NotToAir, untouched.
//
//   TdfFile::GetInt is __thiscall: this->ecx, args right-to-left on stack, callee cleans.
// -----------------------------------------------------------------------
static const DWORD kParseHookAddr   = 0x42E4ABu;
static const DWORD kParseHookLen    = 6u;
static const DWORD kGetIntAddr      = 0x4C46C0u;   // TdfFile::GetInt(char* key, int default)
static const DWORD kWeaponMaskOff   = 0x111u;       // WeaponTypedef::WeaponTypeMask

static const char kNotToAirKey[]    = "nottoair";

// Call TdfFile::GetInt("nottoair", 0) via raw address, thiscall convention.
static int callGetInt(DWORD tdfThis, const char* key)
{
    DWORD fnAddr = kGetIntAddr;     // local DWORD so inline asm can use indirect call
    int result;
    __asm {
        push 0          // default value (second param)
        push key        // key string    (first param)
        mov  ecx, tdfThis
        call fnAddr     // call dword ptr [fnAddr] -- indirect via local variable
        mov  result, eax
    }
    return result;
}

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

    m_tdfParseHook.reset(new InlineSingleHook(
        kParseHookAddr, kParseHookLen,
        INLINE_5BYTESLAGGERJMP,
        (InlineX86HookRouter)ParseRouter));
}

NotToAir::~NotToAir()
{
    m_weaponCheckHook.reset();
    m_tdfParseHook.reset();
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

// TDF parsing: reads "nottoair" from the weapon TDF and sets WTM_NotToAir.
int __stdcall NotToAir::ParseRouter(PInlineX86StackBuffer pBuf)
{
    if (callGetInt(pBuf->Ebx, kNotToAirKey) & 1)
    {
        DWORD* pMask = (DWORD*)((BYTE*)pBuf->Ebp + kWeaponMaskOff);
        *pMask |= WTM_NotToAir;
    }
    return 0;   // let the stub execute the hooked instructions normally
}
