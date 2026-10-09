#include "ShareUnitSettings.h"

#include "hook/hook.h"
#include "tamem.h"

#include <cstddef>
#include <cstring>
#include <memory>
#include <vector>

namespace
{
    // Voluntary sharing settings preservation contributed by TAG_Venom.
    // GiveUnit recreates the unit, clears both stances, and never transmits
    // them or the requested cloak flag. The stock 24-byte 0x14 packet writes
    // sign-extended health at +0x0B but only reads the low word on receipt.
    // Use its ignored high word for a tagged snapshot, leaving health and
    // every other stock field intact. Unpatched receivers ignore the tag.
    // Capture calls GiveUnit at 004046C5; selection sharing calls it at
    // 00493486. Identify the actual caller BEFORE the rotation return thunk
    // replaces it, so gifts to an unallied player are still recognized.
    constexpr unsigned kCaptureGiveReturn = 0x004046CAu;
    thread_local std::vector<unsigned> g_giveReturnStack;

    constexpr unsigned kStanceMask = 0x003C0000u;
    constexpr unsigned kCloakRequest = 0x00000800u;
    constexpr unsigned kSettingsMask = kStanceMask | kCloakRequest;
    constexpr unsigned short kSnapshotTag = 0xA000u;
    constexpr unsigned short kSnapshotTagMask = 0xE000u;
    constexpr unsigned kHealthHighWordOffset = 0x0Du;

    constexpr DWORD kPacketSendAddr = 0x00488682u;
    constexpr DWORD kStanceClearAddr = 0x00488719u;
    constexpr DWORD kStateSetAddr = 0x004887D5u;
    constexpr DWORD kStateClearAddr = 0x004887E5u;
    constexpr DWORD kHookLength = 6u;

    std::unique_ptr<InlineSingleHook> g_packetSendHook;
    std::unique_ptr<InlineSingleHook> g_stanceClearHook;
    std::unique_ptr<InlineSingleHook> g_stateSetHook;
    std::unique_ptr<InlineSingleHook> g_stateClearHook;

    static_assert(sizeof(UnitStruct) == 0x118, "Engine unit stride");
    static_assert(offsetof(UnitStruct, UnitSelected) == 0x110, "Engine stance/cloak request");
    static_assert(offsetof(UnitStruct, cIsCloaked) == 0x10E, "Engine operational byte");
    static_assert(offsetof(UnitStruct, Owner_PlayerPtr0) == 0x96, "Engine unit owner");
    static_assert(offsetof(PlayerStruct, PlayerAryIndex) == 0x146, "Engine player slot");
    static_assert(offsetof(PlayerStruct, AllyFlagAry) == 0x108, "Engine alliances");

    bool HasValidStances(unsigned settings)
    {
        return ((settings >> 18) & 3u) < 3u && ((settings >> 20) & 3u) < 3u;
    }

    bool IsVoluntaryTransfer(const UnitStruct* source, const PlayerStruct* target)
    {
        const PlayerStruct* owner = source ? source->Owner_PlayerPtr0 : nullptr;
        if (!owner || !target || owner == target || g_giveReturnStack.empty()
            || g_giveReturnStack.back() == kCaptureGiveReturn)
            return false;
        const unsigned giver = static_cast<unsigned char>(owner->PlayerAryIndex);
        const unsigned receiver = static_cast<unsigned char>(target->PlayerAryIndex);
        return giver < 10u && receiver < 10u;
    }

    bool ReadSnapshot(const BYTE* packet, unsigned& settings, BYTE& operational)
    {
        if (!packet || packet[0] != 0x14)
            return false;
        unsigned short snapshot;
        std::memcpy(&snapshot, packet + kHealthHighWordOffset, sizeof(snapshot));
        if ((snapshot & kSnapshotTagMask) != kSnapshotTag)
            return false;
        settings = (static_cast<unsigned>(snapshot & 0x0Fu) << 18)
            | ((snapshot & 0x10u) ? kCloakRequest : 0u);
        operational = static_cast<BYTE>((snapshot >> 5) & 0xFFu);
        return HasValidStances(settings);
    }

    int __stdcall PacketSendProc(PInlineX86StackBuffer buf)
    {
        const UnitStruct* source = reinterpret_cast<const UnitStruct*>(buf->Esi);
        const PlayerStruct* target = reinterpret_cast<const PlayerStruct*>(buf->Edi);
        if (!IsVoluntaryTransfer(source, target) || !HasValidStances(source->UnitSelected))
            return 0; // Capture transfers retain the stock packet.

        const unsigned short snapshot = static_cast<unsigned short>(kSnapshotTag
            | ((source->UnitSelected & kStanceMask) >> 18)
            | ((source->UnitSelected & kCloakRequest) ? 0x10u : 0u)
            | ((source->cIsCloaked & 0xFFu) << 5));
        // At 00488682 the completed packet starts at ESP+0x10, before the
        // broadcast arguments are pushed. Only the ignored word is written.
        BYTE* packet = reinterpret_cast<BYTE*>(buf->Esp + 0x10u);
        std::memcpy(packet + kHealthHighWordOffset, &snapshot, sizeof(snapshot));
        return 0; // Re-execute MOV EDX,[ESI+0x96].
    }

    int __stdcall StanceClearProc(PInlineX86StackBuffer buf)
    {
        unsigned settings;
        BYTE operational;
        const BYTE* packet = reinterpret_cast<const BYTE*>(buf->Eax);
        if (packet)
        {
            // Never guess remote settings from the old unit's stale mirror.
            if (!ReadSnapshot(packet, settings, operational))
                return 0;
        }
        else
        {
            const UnitStruct* source = reinterpret_cast<const UnitStruct*>(buf->Esi);
            const DWORD* frame = reinterpret_cast<const DWORD*>(buf->Esp);
            const PlayerStruct* target = reinterpret_cast<const PlayerStruct*>(frame[0x30 / 4]);
            if (!IsVoluntaryTransfer(source, target) || !HasValidStances(source->UnitSelected))
                return 0;
            settings = source->UnitSelected;
        }
        // EDI is the successfully allocated replacement; EDX holds its state.
        // Replace only player settings, keeping its identity/lifecycle flags.
        // Skip the displaced AND that would clear the stances again. The next
        // TEST EAX,EAX produces the branch flags, so no EFLAGS emulation is needed.
        buf->Edx = (buf->Edx & ~kSettingsMask) | (settings & kSettingsMask);
        buf->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(kStanceClearAddr + kHookLength);
        return X86STRACKBUFFERCHANGE;
    }

    int __stdcall StateSetProc(PInlineX86StackBuffer buf)
    {
        const DWORD* frame = reinterpret_cast<const DWORD*>(buf->Esp);
        const BYTE* packet = reinterpret_cast<const BYTE*>(frame[0x34 / 4]);
        unsigned settings;
        BYTE operational;
        if (!ReadSnapshot(packet, settings, operational))
            return 0;
        // Replace MOV AL,[ESI+0x10E] with the giver's exact operational byte.
        // Keep TA's state-edge calls (Activate/Deactivate, cloak events, network
        // notification), rather than assigning the replacement byte directly.
        buf->Eax = (buf->Eax & 0xFFFFFF00u) | operational;
        buf->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(kStateSetAddr + kHookLength);
        return X86STRACKBUFFERCHANGE;
    }

    int __stdcall StateClearProc(PInlineX86StackBuffer buf)
    {
        const DWORD* frame = reinterpret_cast<const DWORD*>(buf->Esp);
        const BYTE* packet = reinterpret_cast<const BYTE*>(frame[0x34 / 4]);
        unsigned settings;
        BYTE operational;
        if (!ReadSnapshot(packet, settings, operational))
            return 0;
        // The second state-edge call clears the complement of the SAME byte.
        buf->Ecx = (buf->Ecx & 0xFFFFFF00u) | operational;
        buf->rtnAddr_Pvoid = reinterpret_cast<LPVOID>(kStateClearAddr + kHookLength);
        return X86STRACKBUFFERCHANGE;
    }

    bool BytesMatch(DWORD address, const BYTE* bytes, size_t length)
    {
        return std::memcmp(reinterpret_cast<const void*>(address), bytes, length) == 0;
    }
}

namespace ShareUnitSettings
{
    void __cdecl BeginGive(unsigned originalReturnAddress)
    {
        g_giveReturnStack.push_back(originalReturnAddress);
    }

    unsigned __cdecl EndGive()
    {
        const unsigned originalReturnAddress = g_giveReturnStack.back();
        g_giveReturnStack.pop_back();
        return originalReturnAddress;
    }

    void Install()
    {
        if (g_packetSendHook)
            return;
        const BYTE captureCall[] = { 0xE8, 0xA6, 0x3E, 0x08, 0x00 };
        const BYTE send[] = { 0x8B, 0x96, 0x96, 0x00, 0x00, 0x00 };
        const BYTE stance[] = { 0x81, 0xE2, 0xFF, 0xFF, 0xC3, 0xFF };
        const BYTE stateSet[] = { 0x8A, 0x86, 0x0E, 0x01, 0x00, 0x00 };
        const BYTE stateClear[] = { 0x8A, 0x8E, 0x0E, 0x01, 0x00, 0x00 };
        const BYTE packetArg[] = { 0x8B, 0x44, 0x24, 0x34 };
        const BYTE stanceResume[] = { 0x85, 0xC0, 0x89, 0x97, 0x10, 0x01, 0x00, 0x00 };
        const BYTE healthRead[] = { 0x66, 0x8B, 0x50, 0x0B };
        const BYTE setResume[] = { 0x6A, 0x01, 0x50, 0x8B, 0xCF, 0xE8, 0xAB, 0x28, 0x00, 0x00 };
        const BYTE clearResume[] = { 0x6A, 0x00, 0xF6, 0xD1, 0x51, 0x8B, 0xCF, 0xE8, 0x99, 0x28, 0x00, 0x00 };
        // Install all four or none. Also verify packet/continuation contracts.
        // These interior sites do not overlap the rotation or share-delay hooks.
        if (!BytesMatch(0x004046C5u, captureCall, sizeof(captureCall))
            || !BytesMatch(kPacketSendAddr, send, sizeof(send))
            || !BytesMatch(kStanceClearAddr, stance, sizeof(stance))
            || !BytesMatch(kStateSetAddr, stateSet, sizeof(stateSet))
            || !BytesMatch(kStateClearAddr, stateClear, sizeof(stateClear))
            || !BytesMatch(0x00488715u, packetArg, sizeof(packetArg))
            || !BytesMatch(0x0048871Fu, stanceResume, sizeof(stanceResume))
            || !BytesMatch(0x00488729u, healthRead, sizeof(healthRead))
            || !BytesMatch(0x004887DBu, setResume, sizeof(setResume))
            || !BytesMatch(0x004887EBu, clearResume, sizeof(clearResume)))
            return;

        g_packetSendHook.reset(new InlineSingleHook(kPacketSendAddr, kHookLength,
            INLINE_5BYTESLAGGERJMP, PacketSendProc));
        g_stanceClearHook.reset(new InlineSingleHook(kStanceClearAddr, kHookLength,
            INLINE_5BYTESLAGGERJMP, StanceClearProc));
        g_stateSetHook.reset(new InlineSingleHook(kStateSetAddr, kHookLength,
            INLINE_5BYTESLAGGERJMP, StateSetProc));
        g_stateClearHook.reset(new InlineSingleHook(kStateClearAddr, kHookLength,
            INLINE_5BYTESLAGGERJMP, StateClearProc));
    }

    void Shutdown()
    {
        g_stateClearHook.reset();
        g_stateSetHook.reset();
        g_stanceClearHook.reset();
        g_packetSendHook.reset();
    }
}
