#pragma once

#include <cstddef>
#include <cstdint>

// Extended weapon-fired packet for IDs >= 256.
//
// TA's WEAPON_FIRED_0D packet carries the weapon ID as a single byte at
// payload offset +0x19, capping the protocol at 256 weapons. Rather than
// resize that packet (which would break the recorder/replayer round-trip),
// we hijack a CHAT_05 envelope and stuff the 36-byte 0x0D payload plus the
// full uint16 ID into it. See PacketChatRouter.h for the empty-chat
// dispatch convention and ChatHijackIds.h for the central msgId registry.
//
// Recorder/replayer behaviour: this looks like a 65-byte chat with empty
// text — they store and round-trip it byte-for-byte without parsing fields.
//
// Wire compatibility: clients without tadr-ddraw treat this as an empty
// chat packet (no visible side effect). Cross-version games where a host
// uses overflow weapons against an older client cannot work: the older
// client will silently drop the fire event. The integrity-handshake in
// ChallengeResponse already rejects mismatched-mod joins, so this is OK.

#pragma pack(1)
struct WeaponFiredExtMessage {
    char     chatByte;            // 0x05    — CHAT_05 envelope
    char     nullText;            // 0x00    — empty chat sentinel
    char     msgId;               // ChatHijackId::WeaponFiredExt (0x2d)
    int16_t  size;                // sizeof(WeaponFiredExtMessage) = 65
    uint16_t fullWeaponId;        // 16-bit weapon id (the value byte +0x19 truncated)
    uint8_t  packet0D[36];        // verbatim WEAPON_FIRED_0D payload
                                  //   (byte +0x19 ignored on receive — fullWeaponId wins)
    char     pad[22];             // pad to 65 bytes
};
#pragma pack()
static_assert(sizeof(WeaponFiredExtMessage) == 65,
              "WeaponFiredExtMessage must be exactly 65 bytes (matches CHAT_05 size)");
// Byte 64 (the last byte of the 65-byte chat envelope) must remain zero so
// gpgnet4ta's TPacket sizer (tapacket/TPacket.cpp:286) does not trigger its
// "older recorder emitted long chat" fallback and over-read into the next
// subpacket. The fallback is `if (s[len-1] != 0) len = sz`. Our pad covers
// offsets 43..64 and is zero-init via memset, so this assert mainly guards
// against a future contributor shrinking the pad and adding a payload field
// that ends up at byte 64.
static_assert(offsetof(WeaponFiredExtMessage, pad) + sizeof(WeaponFiredExtMessage::pad) == 65,
              "pad must occupy through byte 64; byte 64 must remain zero-init");

namespace WeaponFiredExt {
    // Install send-side and receive-side hooks. Idempotent.
    void Install();

    // Tear down hooks.
    void Shutdown();
}
