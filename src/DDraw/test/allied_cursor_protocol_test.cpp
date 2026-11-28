#include "../AlliedCursorProtocol.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

using namespace AlliedCursorProtocol;
namespace
{
    const unsigned char Id = 0x61;
    Packet Data(std::uint32_t session, std::uint32_t serial, Kind kind = Position)
    {
        Packet p = Make(Id, kind, session); p.sequence = serial;
        if (kind == Position) { p.x = 321; p.y = 456; p.z = 12; }
        return p;
    }
    void Handshake(Peer& peer, std::uint32_t now = 0)
    {
        Packet hello = Make(Id, Hello, 22);
        assert(peer.Accept(hello, 11, now));
        assert(peer.pendingAck == 22 && !peer.capable);
        Packet ack = Make(Id, HelloAck, 22); ack.echoSession = 11;
        assert(peer.Accept(ack, 11, now));
        assert(peer.capable);
    }
    void TestEnvelope()
    {
        Packet p = Data(22, 1);
        assert(Envelope(p, Id));
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&p);
        assert(bytes[0] == 5 && bytes[1] == 0 && bytes[64] == 0);
        Packet bad = p; bad.msgId = 0x60; assert(!Envelope(bad, Id));
        bad = p; bad.size = 64; assert(!Envelope(bad, Id));
        bad = p; bad.version = 2; assert(!Envelope(bad, Id));
        bad = p; bad.nullText = 1; assert(!Envelope(bad, Id));
        bad = p; bad.reserved[8] = 1; assert(!Envelope(bad, Id));
        bad = p; bad.reserved[0] = 1; assert(!Envelope(bad, Id));
        bad = p; bad.session = 0; assert(!Envelope(bad, Id));
        bad = p; bad.sequence = 0; assert(!Envelope(bad, Id));
        bad = p; bad.kind = 5; assert(!Envelope(bad, Id));
        bad = p; bad.echoSession = 1; assert(!Envelope(bad, Id));
        bad = p; bad.x = -1; assert(!Envelope(bad, Id));
        bad = p; bad.y = 65536; assert(!Envelope(bad, Id));
        bad = p; bad.z = -1; assert(!Envelope(bad, Id));
        bad = p; bad.z = 65536; assert(!Envelope(bad, Id));
        bad = p; bad.arrow[0] = ' '; assert(!Envelope(bad, Id));
        bad = p; bad.x.digits[0] = 'a'; assert(!Envelope(bad, Id));
        bad = p; bad.y.digits[0] = '>'; assert(!Envelope(bad, Id));
        p = Make(Id, Hello, 22); assert(Envelope(p, Id));
        bad = p; bad.echoSession = 11; assert(!Envelope(bad, Id));
        bad = p; bad.x = 1; assert(!Envelope(bad, Id));
        p = Make(Id, HelloAck, 22); assert(!Envelope(p, Id));
        p.echoSession = 11; assert(Envelope(p, Id));
        p = Data(22, 2, Hidden); assert(Envelope(p, Id));
        p.x = 1; assert(!Envelope(p, Id));
    }
    void TestRecorderCompatibility()
    {
        // Mirror the Pascal recorder's complete binary-string search, including
        // bytes after nullText. No payload value may trigger enemy-chat copies.
        const auto check = [](const Packet& p)
        {
            const std::string wire(reinterpret_cast<const char*>(&p), sizeof(p));
            // Pascal Pos is one-based and returns zero when absent.
            const auto pos = [&wire](const char* pattern) -> std::size_t
            {
                const std::size_t index = wire.find(pattern);
                return index == std::string::npos ? 0 : index + 1;
            };
            const bool legacyEnemyForward = ((pos("->") > 0)
                && ((pos("->Allies") > 0) || (pos("->Enemies") > 0))
                && (pos("->") < pos("> "))) || (pos("->") == 0);
            assert(!legacyEnemyForward);
            // ChatMsgHandler takes PChar(Copy(s,Pos('>')+2,64)) and only
            // invokes a command if that substring begins with a period.
            assert(pos(">") > 0 && pos(">") + 1 < wire.size());
            assert(wire[pos(">") + 1] != '.');
            assert(wire[64] == 0 && wire.find("->") == 6);
            assert(wire.find("->", 7) == std::string::npos);
            assert(wire.find("->Allies") == std::string::npos);
            assert(wire.find("->Enemies") == std::string::npos);
            assert(wire.find("> ") == std::string::npos);
            assert(wire.find('.') == std::string::npos);
        };
        for (std::uint32_t value : {0u, 1u, 0x3e2du, 0xffffffffu, 0x416c6c69u, 0x202e3e2du})
        {
            Packet p = Make(Id, Position, value ? value : 1);
            p.echoSession = value; p.sequence = value; p.x = value; p.y = value; p.z = value;
            check(p);
            assert(static_cast<std::uint32_t>(p.x) == value);
        }
        std::uint32_t seed = 0x12345678u;
        const auto next = [&seed]() -> std::uint32_t
        {
            seed = seed * 1664525u + 1013904223u;
            return seed;
        };
        for (unsigned iteration = 0; iteration < 100000; ++iteration)
        {
            Packet p = Make(Id, static_cast<Kind>(iteration % 4 + 1), next());
            p.echoSession = next(); p.sequence = next(); p.x = next(); p.y = next(); p.z = next();
            check(p);
        }
    }
    void TestNegotiationAndSessions()
    {
        Peer peer;
        assert(!peer.Accept(Data(22, 1), 11, 0));
        Packet hello = Make(Id, Hello, 22);
        assert(peer.Accept(hello, 11, 0));
        assert(!peer.Accept(Data(22, 1), 11, 0));
        Packet ack = Make(Id, HelloAck, 22); ack.echoSession = 10;
        assert(!peer.Accept(ack, 11, 0));
        Handshake(peer);
        assert(peer.Accept(Data(22, 1), 11, 100));
        assert(peer.Visible(100));
        assert(!peer.Accept(Data(23, 2), 11, 200));
        assert(peer.session == 22 && peer.Visible(200));
        hello.session = 23;
        assert(peer.Accept(hello, 11, 200));
        assert(!peer.capable && !peer.Visible(200) && peer.pendingAck == 23);
        assert(!peer.Accept(Data(22, 3), 11, 200));
        ack.session = 23; ack.echoSession = 11;
        assert(peer.Accept(ack, 11, 200));
        assert(peer.Accept(Data(23, 1), 11, 200));
        assert(peer.Visible(200));
        // A previous match's acknowledgment cannot enable the new match.
        Peer nextMatch;
        assert(!nextMatch.Accept(ack, 12, 200));
        assert(!nextMatch.capable);
    }
    void TestOrderingAndExpiry()
    {
        Peer peer; Handshake(peer);
        assert(peer.Accept(Data(22, 0xfffffffeu), 11, 100));
        assert(peer.Accept(Data(22, 1), 11, 200));
        assert(!peer.Accept(Data(22, 0xffffffffu), 11, 300));
        assert(!peer.Accept(Data(22, 1), 11, 400));
        assert(peer.receivedAt == 200);
        assert(peer.Visible(2199) && !peer.Visible(2200));
        assert(peer.Accept(Data(22, 2), 11, 2300));
        assert(peer.Accept(Data(22, 3, Hidden), 11, 2400));
        assert(!peer.Visible(2400));
        assert(!peer.Accept(Data(22, 2), 11, 2500));
        peer.Expire(17399); assert(peer.capable);
        peer.Expire(17400); assert(!peer.capable && peer.session == 0);
        // Wall-clock wrap preserves the two-second cursor lifetime.
        Handshake(peer, 0xfffffff0u);
        assert(peer.Accept(Data(22, 1), 11, 0xfffffff0u));
        assert(peer.Visible(10));
        assert(!peer.Visible(2000));
    }
    void TestPacing()
    {
        Transmitter tx; Point p; p.x = 10; p.y = 20;
        assert(tx.Due(p, true, 0)); tx.Sent(p, true, 0);
        ++p.x;
        assert(!tx.Due(p, true, 99));
        assert(tx.Due(p, true, 100)); tx.Sent(p, true, 100);
        assert(!tx.Due(p, true, 1099));
        assert(tx.Due(p, true, 1100));
        assert(!tx.Due(p, false, 199));
        assert(tx.Due(p, false, 200)); tx.Sent(p, false, 200);
        assert(!tx.Due(p, false, 1199));
        assert(tx.Due(p, false, 1200));
        unsigned transmissions = 0;
        tx = Transmitter();
        for (unsigned ms = 0; ms < 1000; ++ms)
        {
            p.x = ms;
            if (tx.Due(p, true, ms)) { tx.Sent(p, true, ms); ++transmissions; }
        }
        assert(transmissions == 10);
        tx.Sent(p, true, 0xfffffff0u); ++p.x;
        assert(!tx.Due(p, true, 1));
        assert(tx.Due(p, true, 100));
    }
    void TestGlobalBudget()
    {
        Budget budget;
        for (unsigned i = 0; i < 9; ++i) assert(budget.Take(0));
        assert(!budget.Take(0));
        assert(!budget.Take(11));
        assert(budget.Take(12));
        assert(!budget.Take(12));
        unsigned packets = 0;
        budget = Budget();
        for (unsigned ms = 0; ms < 10000; ++ms)
            while (budget.Take(ms)) ++packets;
        assert(packets <= 909 && packets >= 908);
        // A pause earns at most the nine-packet burst, not a backlog.
        for (unsigned i = 0; i < 9; ++i) assert(budget.Take(999999));
        assert(!budget.Take(999999));
        budget = Budget();
        for (unsigned i = 0; i < 9; ++i) assert(budget.Take(0xfffffff0u));
        assert(budget.Take(0));
        assert(!budget.Take(0));
    }
}
int main()
{
    TestEnvelope(); TestRecorderCompatibility(); TestNegotiationAndSessions(); TestOrderingAndExpiry();
    TestPacing(); TestGlobalBudget();
    std::cout << "Allied cursor protocol tests passed\n";
}
