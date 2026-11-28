#pragma once

// Cursor sharing carries presentation data only. Keep the complete wire/state
// contract portable so malformed traffic, wraparound and pacing are testable.
#include <cstddef>
#include <cstdint>

namespace AlliedCursorProtocol
{
    const std::uint32_t MoveIntervalMs = 100;
    const std::uint32_t HeartbeatMs = 1000;
    const std::uint32_t ProbeMs = 5000;
    const std::uint32_t VisibleMs = 2000;
    const std::uint32_t CapabilityMs = 15000;
    const std::uint32_t AckIntervalMs = 1000;
    enum Kind { Hello = 1, HelloAck = 2, Position = 3, Hidden = 4 };

    struct Point
    {
        std::int32_t x = 0, y = 0, z = 0;
    };
    inline bool Same(const Point& a, const Point& b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    inline bool Newer(std::uint32_t a, std::uint32_t b)
    {
        return a != b && static_cast<std::uint32_t>(a - b) < 0x80000000u;
    }

#pragma pack(push, 1)
    struct Hex32
    {
        char digits[8];
        Hex32& operator=(std::uint32_t value)
        {
            const char hex[] = "0123456789ABCDEF";
            for (int index = 7; index >= 0; --index)
            {
                digits[index] = hex[value & 15]; value >>= 4;
            }
            return *this;
        }
        bool Valid() const
        {
            for (char digit : digits)
                if (!((digit >= '0' && digit <= '9') || (digit >= 'A' && digit <= 'F'))) return false;
            return true;
        }
        operator std::uint32_t() const
        {
            std::uint32_t value = 0;
            for (char digit : digits)
                value = (value << 4) | static_cast<unsigned>(digit <= '9' ? digit - '0' : digit - 'A' + 10);
            return value;
        }
    };
    struct Packet
    {
        std::uint8_t chatByte, nullText, msgId, size, version, kind;
        // Recorder idplay.pas forwards directed chats lacking "->" to enemy
        // recorders. This fixed prefix prevents that forwarding. Uppercase hex
        // throughout the payload cannot produce its "->Allies", "->Enemies"
        // or command-parser patterns, regardless of the numeric values.
        char arrow[2];
        Hex32 session, echoSession, sequence, x, y, z;
        // The recorder uses the final NUL to recognise the 65-byte chat size.
        // All unused bytes stay zero, including byte 64.
        std::uint8_t reserved[9];
    };
#pragma pack(pop)
    static_assert(sizeof(Packet) == 65, "TA CHAT05 envelope");
    static_assert(sizeof(Hex32) == 8, "Canonical uppercase wire numbers");
    static_assert(offsetof(Packet, reserved) + 8 == 64, "CHAT05 final NUL");

    inline Packet Make(unsigned char id, Kind kind, std::uint32_t session)
    {
        Packet p = {};
        p.chatByte = 5; p.msgId = id; p.size = sizeof(p); p.version = 1;
        p.kind = static_cast<std::uint8_t>(kind); p.arrow[0] = '-'; p.arrow[1] = '>';
        p.session = session; p.echoSession = 0; p.sequence = 0;
        p.x = 0; p.y = 0; p.z = 0;
        return p;
    }
    inline bool Envelope(const Packet& p, unsigned char id)
    {
        if (p.chatByte != 5 || p.nullText || p.msgId != id || p.size != sizeof(p)
            || p.version != 1 || p.kind < Hello || p.kind > Hidden
            || p.arrow[0] != '-' || p.arrow[1] != '>'
            || !p.session.Valid() || !p.echoSession.Valid() || !p.sequence.Valid()
            || !p.x.Valid() || !p.y.Valid() || !p.z.Valid() || !p.session) return false;
        for (unsigned char byte : p.reserved) if (byte) return false;
        if (p.kind == Hello || p.kind == HelloAck)
            return !p.sequence && !p.x && !p.y && !p.z
                && (p.kind == Hello ? p.echoSession == 0 : p.echoSession != 0);
        if (p.echoSession || !p.sequence) return false;
        if (p.kind == Hidden) return !p.x && !p.y && !p.z;
        return p.x <= 65535 && p.y <= 65535 && p.z <= 65535;
    }

    struct Peer
    {
        std::uint32_t session = 0, pendingAck = 0, sequence = 0;
        std::uint32_t heardAt = 0, receivedAt = 0;
        bool capable = false, haveSequence = false, visible = false;
        Point point;

        void StartSession(std::uint32_t next)
        {
            *this = Peer(); session = next;
        }
        // Envelope and current sender/alliance checks belong to the caller.
        bool Accept(const Packet& p, std::uint32_t localSession, std::uint32_t now)
        {
            if (p.kind == Hello)
            {
                if (session != p.session) StartSession(p.session);
                pendingAck = p.session; heardAt = now;
                return true;
            }
            if (p.kind == HelloAck)
            {
                if (p.echoSession != localSession) return false;
                if (session != p.session) StartSession(p.session);
                capable = true; heardAt = now;
                return true;
            }
            if (!capable || p.session != session
                || (haveSequence && !Newer(p.sequence, sequence))) return false;
            haveSequence = true; sequence = p.sequence; heardAt = receivedAt = now;
            visible = p.kind == Position;
            point.x = p.x; point.y = p.y; point.z = p.z;
            return true;
        }
        void Expire(std::uint32_t now)
        {
            if (capable && now - heardAt >= CapabilityMs) *this = Peer();
        }
        bool Visible(std::uint32_t now) const
        {
            return capable && haveSequence && visible && now - receivedAt < VisibleMs;
        }
    };

    struct Transmitter
    {
        std::uint32_t sentAt = 0;
        bool sent = false, visible = false;
        Point point;
        bool Due(const Point& next, bool valid, std::uint32_t now) const
        {
            if (!sent) return true;
            const std::uint32_t elapsed = now - sentAt;
            return elapsed >= HeartbeatMs || (elapsed >= MoveIntervalMs
                && (valid != visible || (valid && !Same(point, next))));
        }
        void Sent(const Point& next, bool valid, std::uint32_t now)
        {
            point = next; visible = valid; sentAt = now; sent = true;
        }
    };

    // One shared allowance for discovery, acknowledgments and positions. The
    // nine-packet burst accommodates a full team; sustained traffic is <=90/s.
    struct Budget
    {
        std::uint32_t last = 0, credit = 9000;
        bool started = false;
        bool Take(std::uint32_t now)
        {
            if (!started) { started = true; last = now; }
            const std::uint32_t elapsed = now - last;
            last = now;
            const std::uint32_t addition = (elapsed < 1000 ? elapsed : 1000) * 90;
            credit = credit + addition < 9000 ? credit + addition : 9000;
            if (credit < 1000) return false;
            credit -= 1000;
            return true;
        }
    };
}
