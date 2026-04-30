#pragma once

// Central registry of CHAT_05 hijack msgIds.
//
// TA's chat subpacket (opcode 0x05) carries 65 bytes: a 0x05 header byte,
// the chat text starting at +1, terminated by NUL. We hijack chat packets
// whose first text byte is NUL (i.e. empty chat) by treating the byte at
// +2 as a custom message-id; PacketChatRouter dispatches on it. The
// recorder/replayer treat such packets as opaque chat and round-trip them
// faithfully.
//
// Reserve a new id HERE before adding a hijacker, so collisions are caught
// at compile time rather than in production.
namespace ChatHijackId {

    // ---- 0x20..0x3F : integrity / extension --------------------------------
    constexpr unsigned char ChallengeResponse = 0x2b;  // ChallengeResponse.cpp
    constexpr unsigned char VoteReject        = 0x2c;  // VoteReject.cpp
    constexpr unsigned char WeaponFiredExt    = 0x2d;  // WeaponFiredExt.cpp (planned)

    // ---- 0x40..0x5F : reserved for gameplay extensions ---------------------
    // ---- 0x60..0x7F : reserved for UI / HUD notifications ------------------
    // ---- 0x80..0xFF : reserved for future use ------------------------------
}
