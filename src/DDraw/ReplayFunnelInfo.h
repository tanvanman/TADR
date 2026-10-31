#pragma once

#include <cstddef>
#include <cstdint>

// The existing 65-byte funnel chat marker follows the 7-byte smartpak header.
// Its first six bytes stay unchanged for legacy DLLs. The optional LOS v1 tag
// carries the starting team already saved in the recording's player status.
inline bool ReplayFunnelStartingTeam(const std::uint8_t* packet, std::size_t size,
    unsigned char& team)
{
    if (!packet || size < 72 || packet[0] != 0x03 ||
        (packet[13] ^ 13) != 'L' || (packet[14] ^ 14) != 'O' ||
        (packet[15] ^ 15) != 'S' || (packet[16] ^ 16) != 1)
        return false;
    const unsigned char value = packet[17] ^ 17;
    if (value > 5) return false;
    team = value;
    return true;
}

// Optional camera sample in the unused marker tail. Earlier senders leave it zero.
inline bool ReplayFunnelMapPosition(const std::uint8_t* packet, std::size_t size,
    unsigned short& x, unsigned short& y)
{
    unsigned char team;
    if (!ReplayFunnelStartingTeam(packet, size, team) ||
        (packet[18] ^ 18) != 'C' || (packet[19] ^ 19) != 1)
        return false;
    x = (packet[20] ^ 20) | ((packet[21] ^ 21) << 8);
    y = (packet[22] ^ 22) | ((packet[23] ^ 23) << 8);
    return true;
}
