#include <algorithm>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <cctype>
#include <sstream>
#include <vector>
#include <map>

#include "TPacket.h"

using namespace tapacket;

void TPacket::decrypt(bytestring &data, std::size_t ofs, ::uint16_t &checkExtracted, std::uint16_t &checkCalculated)
{
    if (data.size() < 4+ofs)
    {
        data += std::uint8_t(0x06);
        return;
    }

    checkExtracted = *(std::uint16_t*)&data[ofs+1];
    checkCalculated = 0u;
    std::uint8_t xorKey = 3u;
    std::size_t i = ofs+3u;
    for (; i <= data.size() - 4; ++i)
    {
        checkCalculated += data[i];
        data[i] ^= xorKey;
        ++xorKey;
    }
}

void TPacket::encrypt(bytestring& data)
{
    if (data.size() < 4)
    {
        data += std::uint8_t(0x06);
        return;
    }

    std::uint16_t check = 0u;
    for (std::size_t i = 3u; i <= data.size() - 4; ++i)
    {
        data[i] ^= std::uint8_t(i);
        check += data[i];
    }
    data[1] = check & 0x00ff;
    data[2] = check >> 8;
}

bytestring TPacket::compress(const bytestring &data)
{
    unsigned index, cbf, count, a, matchl, cmatchl;
    std::uint16_t kommando, match;
    std::uint16_t *p;

    bytestring result;
    count = 7u;
    index = 4u;
    while (index < data.size() + 1u)
    {
        if (count == 7u)
        {
            count = 0u;
            result += std::uint8_t(0u);
            cbf = result.size();
        }
        else
        {
            ++count;
        }
        if (index < 6u || index>2000u)
        {
            result += data[index - 1u];
            ++index;
        }
        else
        {
            matchl = 2u;
            for (a = 4u; a < index - 1u; ++a)
            {
                cmatchl = 0;
                while (a + cmatchl < index && index + cmatchl < data.size() && data[a + cmatchl - 1u] == data[index + cmatchl - 1u])
                {
                    ++cmatchl;
                }
                if (cmatchl > matchl)
                {
                    matchl = cmatchl;
                    match = a;
                    if (matchl > 17u)
                    {
                        break;
                    }
                }
            }
            cmatchl = 0u;
            while (index + cmatchl < data.size() && data[index + cmatchl - 1u] == data[index - 2u])
            {
                ++cmatchl;
            }
            if (cmatchl > matchl)
            {
                matchl = cmatchl;
                match = index - 1u;
            }
            if (matchl>2)
            {
                result[cbf - 1u] |= (1u << count);
                matchl = (matchl - 2u) & 0x0f;
                kommando = ((match - 3u) << 4) | matchl;
                result += bytestring((const std::uint8_t*)"\0\0", 2);
                p = (std::uint16_t*)&result[result.size() - 2u];
                *p = kommando;
                index += matchl + 2u;
            }
            else
            {
                result += data[index - 1u];
                ++index;
            }
        }
    }
    if (count == 7u)
    {
        result += 0xff;
    }
    else
    {
        result[cbf - 1u] |= (0xff << (count + 1u));
    }
    result += bytestring((const std::uint8_t*)"\0\0", 2u);

    if (result.size() + 3u < data.size())
    {
        result = bytestring(1u, 0x04) + data[1u] + data[2u] + result;
    }
    else
    {
        result = data;
        result[0] = 0x03;
    }
    return result;
}

bytestring TPacket::decompress(const bytestring &data, const unsigned headerSize)
{
    return decompress(data.data(), data.size(), headerSize);
}

bytestring TPacket::decompress(const std::uint8_t *data, const unsigned len, const unsigned headerSize)
{
    bytestring result;
    if (data[0] != 0x04)
    {
        result.append(data, len);
        return result;
    }

    result.reserve(std::max(0x1000u, 2*len));
    result.append(data, headerSize);
    result[0] = 0x03;

    unsigned index = headerSize;
    while (index < len)
    {
        unsigned cbf = data[index];
        ++index;

        for (unsigned nump = 0; nump < 8; ++nump)
        {
            if (index >= len)
            {
                // error: ran out of bytes. if you get here theres a bug
                result[0] = 0x04;
                return result;
            }
            if (((cbf >> nump) & 1) == 0)
            {
                result += data[index];
                ++index;
            }
            else
            {
                unsigned uop = *(std::uint16_t*)(&data[index]);
                index += 2;
                unsigned a = uop >> 4;
                if (a == 0)
                {
                    return result;
                }
                uop = uop & 0x0f;
                a += headerSize;
                for (unsigned b = a; b < uop + a + 2; ++b)
                {
                    result += b <= result.size() ? result[b-1] : 0;
                }
            }
        }
    }
    return result;
}

std::string TPacket::toString(SubPacketCode spc)
{
    switch (spc)
    {
    case SubPacketCode::ZERO_00: return "ZERO_00";
    case SubPacketCode::PING_02: return "PING_02";
    case SubPacketCode::UNK_03: return "UNK_03";
    case SubPacketCode::CHAT_05: return "CHAT_05";
    case SubPacketCode::PAD_ENCRYPT_06: return "PAD_ENCRYPT";
    case SubPacketCode::UNK_07: return "UNK_07";
    case SubPacketCode::LOADING_STARTED_08:  return "LOADING_STARTED";
    case SubPacketCode::UNIT_BUILD_STARTED_09: return "UNIT_BUILD_STARTED_09";
    case SubPacketCode::UNK_0A: return "UNK_0A";
    case SubPacketCode::UNIT_TAKE_DAMAGE_0B:return "UNIT_TAKE_DAMAGE_0B";
    case SubPacketCode::UNIT_KILLED_0C: return "UNIT_KILLED_0C";
    case SubPacketCode::WEAPON_FIRED_0D: return "WEAPON_FIRED_0D";
    case SubPacketCode::AREA_OF_EFFECT_0E: return "AREA_OF_EFFECT_0E";
    case SubPacketCode::FEATURE_ACTION_0F: return "FEATURE_ACTION_0F";
    case SubPacketCode::UNIT_START_SCRIPT_10: return "UNIT_START_SCRIPT_10";
    case SubPacketCode::UNIT_STATE_11: return "UNIT_STATE_11";
    case SubPacketCode::UNIT_BUILD_FINISHED_12: return "UNIT_BUILD_FINISHED_12";
    case SubPacketCode::GIVE_UNIT_14: return "GIVE_UNIT_14";
    case SubPacketCode::START_15: return "START_15";
    case SubPacketCode::SHARE_RESOURCES_16: return "SHARE_RESOURCES_16";
    case SubPacketCode::UNK_17: return "UNK_17";
    case SubPacketCode::HOST_MIGRATION_18: return "HOST_MIGRATION_18";
    case SubPacketCode::SPEED_19: return "SPEED_19";
    case SubPacketCode::UNIT_DATA_1A: return "UNIT_DATA_1A";
    case SubPacketCode::REJECT_1B: return "REJECT_1B";
    case SubPacketCode::START_1E: return "START_1E";
    case SubPacketCode::UNK_1F: return "UNK_1F";
    case SubPacketCode::PLAYER_INFO_20: return "PLAYER_INFO_20";
    case SubPacketCode::UNK_21: return "UNK_21";
    case SubPacketCode::IDENT3_22:  return "IDENT3_22";
    case SubPacketCode::ALLY_23: return "ALLY_23";
    case SubPacketCode::TEAM_24: return "TEAM_24";
    case SubPacketCode::IDENT2_26:  return "IDENT2_26";
    case SubPacketCode::PLAYER_RESOURCE_INFO_28: return "PLAYER_RESOURCE_INFO_28";
    case SubPacketCode::UNK_29: return "UNK_29";
    case SubPacketCode::LOADING_PROGRESS_2A: return "LOADING_PROGRESS_2A";
    case SubPacketCode::UNIT_STAT_AND_MOVE_2C: return "UNIT_STAT_AND_MOVE_2C";
    case SubPacketCode::UNK_2E: return "UNK_2E";
    case SubPacketCode::THALDREN_EXTENDED_42: return "THALDREN_EXTENDED_42";
    case SubPacketCode::UNK_F6: return "UNK_F6";
    case SubPacketCode::ALLY_CHAT_F9: return "ALLY_CHAT_F9";
    case SubPacketCode::REPLAYER_SERVER_FA: return "REPLAYER_SERVER_FA";
    case SubPacketCode::RECORDER_DATA_CONNECT_FB: return "RECORDER_DATA_CONNECT_FB";
    case SubPacketCode::MAP_POSITION_FC: return "MAP_POSITION_FC";
    case SubPacketCode::SMARTPAK_TICK_OTHER_FD: return "SMARTPAK_TICK_OTHER_FD";
    case SubPacketCode::SMARTPAK_TICK_START_FE: return "SMARTPAK_TICK_START_FE";
    case SubPacketCode::SMARTPAK_TICK_FF: return "SMARTPAK_TICK_FF";
    default: {
        std::ostringstream ss;
        ss << "UNK_" << std::hex << std::setw(2) << std::setfill('0') << unsigned(spc);
        return ss.str();
    }
    };
}

unsigned TPacket::getExpectedSubPacketSize(const bytestring &bytes)
{
    return getExpectedSubPacketSize(bytes.data(), bytes.size());
}

unsigned TPacket::getExpectedSubPacketSize(const std::uint8_t *s, unsigned sz)
{
    if (sz == 0u)
    {
        return 0u;
    }

    unsigned len = 0u;
    SubPacketCode spc = SubPacketCode(s[0]);

    switch (spc)
    {
    case SubPacketCode:: ZERO_00:
        for (; len < sz && s[len] == 0u; ++len);
        break;
    case SubPacketCode::PING_02: len = 13;   break;
    case SubPacketCode::UNK_03: len = 7;     break;
    case SubPacketCode::CHAT_05:
        len = 65;
        if (s[len - 1] != 0)
        {
            // older recorder versions sometimes emit more text than they should
            // however, it is send as a single packet.
            len = sz;
            // And if map position is enabled, the last 5 bytes should be the map
            // pos data
            if (SubPacketCode(s[len - 5]) == SubPacketCode::MAP_POSITION_FC)
            {
                len -= 5;
            }
        }
        break;
    case SubPacketCode::PAD_ENCRYPT_06: len = 1;    break;
    case SubPacketCode::UNK_07: len = 1;    break;
    case SubPacketCode::LOADING_STARTED_08:  len = 1;    break;
    case SubPacketCode::UNIT_BUILD_STARTED_09: len = 23;    break;
    case SubPacketCode::UNK_0A: len = 7;     break;
    case SubPacketCode::UNIT_TAKE_DAMAGE_0B: len = 9;     break;
    case SubPacketCode::UNIT_KILLED_0C: len = 11;    break;
    case SubPacketCode::WEAPON_FIRED_0D: len = 36;    break;
    case SubPacketCode::AREA_OF_EFFECT_0E: len = 14;    break;
    case SubPacketCode::FEATURE_ACTION_0F: len = 6;     break;
    case SubPacketCode::UNIT_START_SCRIPT_10: len = 22;    break;
    case SubPacketCode::UNIT_STATE_11: len = 4;     break;
    case SubPacketCode::UNIT_BUILD_FINISHED_12: len = 5;     break;
    case SubPacketCode::GIVE_UNIT_14: len = 24;    break;
    case SubPacketCode::START_15: len = 1;    break;
    case SubPacketCode::SHARE_RESOURCES_16: len = 17;    break;
    case SubPacketCode::UNK_17: len = 2;    break;
    case SubPacketCode::HOST_MIGRATION_18: len = 2;    break;
    case SubPacketCode::SPEED_19: len = 3;     break;
    case SubPacketCode::UNIT_DATA_1A: len = 14;   break;
    case SubPacketCode::REJECT_1B: len = 6;     break;
    case SubPacketCode::START_1E: len = 2;    break;
    case SubPacketCode::UNK_1F: len = 5;     break;
    case SubPacketCode::PLAYER_INFO_20: len = 192;  break;
    case SubPacketCode::UNK_21: len = 10;    break;
    case SubPacketCode::IDENT3_22:  len = 6;    break;
    case SubPacketCode::ALLY_23: len = 14;    break;
    case SubPacketCode::TEAM_24: len = 6; break;
    case SubPacketCode::IDENT2_26:  len = 41;   break;
    case SubPacketCode::PLAYER_RESOURCE_INFO_28: len = 58;    break;
    case SubPacketCode::UNK_29: len = 3;     break;
    case SubPacketCode::LOADING_PROGRESS_2A:  len = 2;    break;
    case SubPacketCode::UNIT_STAT_AND_MOVE_2C:
        if (sz >= 3) len = *(std::uint16_t*)(&s[1]);
        break;
    case SubPacketCode::UNK_2E: len = 9; break;
    case SubPacketCode::THALDREN_EXTENDED_42:
        if (sz >= 3) len = *(std::uint16_t*)(&s[1]) + 3;
        break;
    case SubPacketCode::UNK_F6: len = 1;     break;
    case SubPacketCode::ALLY_CHAT_F9: len = 73;    break;
    case SubPacketCode::REPLAYER_SERVER_FA: len = 1;     break;
    case SubPacketCode::RECORDER_DATA_CONNECT_FB:
        if (sz >= 2) len = unsigned(s[1]) + 3;
        break;
    case SubPacketCode::MAP_POSITION_FC: len = 5;     break;
    case SubPacketCode::SMARTPAK_TICK_OTHER_FD:
        if (sz >= 3) len = *(std::uint16_t*)(&s[1]) - 4;
        break;
    case SubPacketCode::SMARTPAK_TICK_START_FE: len = 5;     break;
    case SubPacketCode::SMARTPAK_TICK_FF: len = 1;     break;
    default: len = 0;
    };

    return len;
}


// s modified in-place
bytestring TPacket::split2(bytestring &s, bool smartpak, bool &error)
{
    error = false;
    if (s.empty())
    {
        return s;
    }

    unsigned len = getExpectedSubPacketSize(s);

    if ((s[0] == 0xff || s[0] == 0xfe || s[0] == 0xfd) && !smartpak)
    {
        error = true;
    }

    if (s.size() < len)
    {
        len = 0;
        error = true;
    }

    bytestring next;
    if (len == 0)
    {
        next = s;
        s.clear();
        error = true;
    }
    else
    {
        next = s.substr(0, len);
        s = s.substr(len);
    }

    return next;
}

unsigned TPacket::bin2int(const bytestring &s, unsigned start, unsigned num)
{
    unsigned i = 0u;  // index into s
    while (start > 7)
    {
        // skip bytes
        ++i;
        start -= 8u;
    };

    int result = 0u;
    std::uint8_t mask = 1 << start;
    std::uint8_t byte = s[i];

    for (unsigned j = 0u; j < num; ++j)
    {
        // for the jth bit of result
        if (byte & mask)
        {
            result |= (1 << j);
        }

        ++start;
        mask <<= 1;
        if (start > 7)
        {
            // next byte
            ++i;
            byte = s[i];
            start = 0;
            mask = 1u;
        }
    }
    return result;
}

std::vector<bytestring> TPacket::slowUnsmartpak(const bytestring &_c, bool hasTimestamp, bool hasChecksum)
{
    bytestring c;

    if (hasChecksum)
    {
        c = _c;
    }
    else
    {
        c = _c.substr(0, 1) + (const std::uint8_t *)"xx" + _c.substr(1);
    }

    if (c[0] == 0x04)
    {
        c = TPacket::decompress(c, 3);
    }

    if (hasTimestamp)
    {
        c = c.substr(7);
    }
    else
    {
        c = c.substr(3);
    }

    std::vector<bytestring> ut;
    std::uint32_t packnum = 0u;
    while (!c.empty())
    {
        bool error;
        bytestring s = TPacket::split2(c, true, error);

        switch (SubPacketCode(s[0]))
        {
        case SubPacketCode::SMARTPAK_TICK_START_FE:
        {
            packnum = *(std::uint32_t*)(&s[1]);
            break;
        }

        case SubPacketCode::SMARTPAK_TICK_FF:
        {
            bytestring tmp({ ',', 0x0b, 0, 'x', 'x', 'x', 'x', 0xff, 0xff, 1, 0 });
            *(std::uint32_t*)(&tmp[3]) = packnum;
            ++packnum;
            ut.push_back(tmp);
            break;
        }

        case SubPacketCode::SMARTPAK_TICK_OTHER_FD:
        {
            bytestring tmp = s.substr(0, 3) + (const std::uint8_t*)"zzzz" + s.substr(3);
            *(std::uint32_t*)(&tmp[3]) = packnum;
            ++packnum;
            tmp[0] = 0x02c;
            ut.push_back(tmp);
            break;
        }

        default:
            ut.push_back(s);
        };
    }
    return ut;
}

std::vector<bytestring> TPacket::unsmartpak(const bytestring &_c, bool hasTimestamp, bool hasChecksum)
{
    const std::uint8_t *ptr = _c.data();;
    const std::uint8_t *end = ptr + _c.size();

    bytestring buffer;
    if (_c[0] == 0x04)
    {
        buffer = decompress(_c, hasChecksum ? 3 : 1);
        ptr = buffer.data();
        end = ptr + buffer.size();
    }

    ++ptr;
    if (hasChecksum) ptr += 2;
    if (hasTimestamp) ptr += 4;

    std::vector<bytestring> ut;
    std::uint32_t packnum = 0u;
    while (ptr < end)
    {
        unsigned subpakLen = getExpectedSubPacketSize(ptr, end-ptr);
        if (subpakLen == 0 || ptr+subpakLen > end)
        {
            subpakLen = end - ptr;
        }

        switch (SubPacketCode(ptr[0]))
        {
        case SubPacketCode::SMARTPAK_TICK_START_FE:
        {
            packnum = *(std::uint32_t*)(&ptr[1]);
            break;
        }

        case SubPacketCode::SMARTPAK_TICK_FF:
        {
            bytestring tmp({ ',', 0x0b, 0, 'x', 'x', 'x', 'x', 0xff, 0xff, 1, 0 });
            *(std::uint32_t*)(&tmp[3]) = packnum;
            ++packnum;
            ut.push_back(tmp);
            break;
        }

        case SubPacketCode::SMARTPAK_TICK_OTHER_FD:
        {
            ut.push_back(bytestring());
            bytestring &tmp = ut.back();
            tmp.reserve(subpakLen + 4);
            tmp.append(ptr, 3);
            tmp.append((std::uint8_t*)&packnum, 4);
            tmp.append(ptr + 3, subpakLen - 3);
            ++packnum;
            tmp[0] = 0x2c;
            break;
        }

        default:
            ut.push_back(bytestring());
            ut.back().append(ptr, subpakLen);
        };
        ptr += subpakLen;
    }
    return ut;
}

bytestring TPacket::trivialSmartpak(const bytestring& subpacket, std::uint32_t tcpseq)
{
    bytestring result((std::uint8_t*)"\x03\x00\x00", 3);
    result += bytestring((std::uint8_t*) & tcpseq, 4);
    result += subpacket;
    return result;
}

SmartPaker::SmartPaker() :
    m_first(true)
{ }

bytestring SmartPaker::operator()(const bytestring& subpak)
{
    if (subpak.size() < 7 || SubPacketCode(subpak[0]) != SubPacketCode::UNIT_STAT_AND_MOVE_2C)
    {
        return subpak;
    }

    bytestring result;
    if (m_first)
    {
        m_first = false;
        result.push_back(std::uint8_t(SubPacketCode::SMARTPAK_TICK_START_FE));
        result += subpak.substr(3, 4);
    }

    if (subpak[1] == 0x0b && subpak[2] == 0x00)
    {
        result.push_back(std::uint8_t(SubPacketCode::SMARTPAK_TICK_FF));
    }
    else
    {
        result.push_back(std::uint8_t(SubPacketCode::SMARTPAK_TICK_OTHER_FD));
        result += subpak.substr(1, 2);
        result += subpak.substr(7);
    }
    return result;
}

bytestring TPacket::createChatSubpacket(const std::string& message)
{
    char chatMessage[65];
    chatMessage[0] = std::uint8_t(SubPacketCode::CHAT_05);
    std::strncpy(&chatMessage[1], message.c_str(), 64);
    chatMessage[64] = 0;
    return bytestring((std::uint8_t*)chatMessage, sizeof(chatMessage));
}

bytestring TPacket::createHostMigrationSubpacket(int playerNumber)
{
    bytestring bs;
    bs.push_back(std::uint8_t(SubPacketCode::HOST_MIGRATION_18));
    bs.push_back(playerNumber);
    return bs;
}

TPing::TPing(std::uint32_t from, std::uint32_t id, std::uint32_t value) :
    from(from),
    id(id),
    value(value)
{ }

static std::uint32_t toUint32(const std::uint8_t* bytes)
{
    std::uint32_t result = bytes[3];
    for (int n = 2; n >= 0; --n) {
        result <<= 8;
        result |= bytes[n];
    }
    return result;
}

static std::uint16_t toUint16(const std::uint8_t* bytes)
{
    std::uint16_t result = bytes[1];
    result <<= 8;
    result |= bytes[0];
    return result;
}

static bytestring& serialise(bytestring& dest, std::uint32_t x)
{
    dest.push_back(std::uint8_t(x));
    dest.push_back(x >> 8);
    dest.push_back(x >> 16);
    dest.push_back(x >> 24);
    return dest;
}

static bytestring& serialise(bytestring& dest, std::uint16_t x)
{
    dest.push_back(std::uint8_t(x));
    dest.push_back(x >> 8);
    return dest;
}

TPing::TPing(const bytestring& subPacket)
{
    id = toUint32(subPacket.data() + 1);
    value = toUint32(subPacket.data() + 5);
    from = toUint32(subPacket.data()+9);
}

bytestring TPing::asSubPacket() const
{
    bytestring result;
    result.push_back(std::uint8_t(SubPacketCode::PING_02));
    serialise(result, id);
    serialise(result, value);
    serialise(result, from);
    return result;
}

TPlayerInfo::TPlayerInfo(const bytestring& subPacket)
{
    serialisedSize = subPacket.size();
    const std::uint8_t* ptr = subPacket.data() + 1;
    std::memcpy(fill1, ptr, sizeof(fill1));
    width = toUint16(ptr + 139);
    height = toUint16(ptr + 141);
    fill3 = ptr[143];
    player1Id = toUint32(ptr + 144);
    std::memcpy(data2, ptr + 148, sizeof(data2));
    clicked = ptr[155];
    std::memcpy(fill2, ptr + 156, sizeof(fill2));
    maxUnits = toUint16(ptr + 165);
    versionMajor = ptr[167];
    versionMinor = ptr[168];
    if (subPacket.size() == 192)
    {
        std::memcpy(data3, ptr + 169, sizeof(data3));
        player2Id = toUint32(ptr + 186);
        data4 = ptr[190];
    }
    else
    {
        // mysteriously short-sized PlayerInfo packets
        std::memcpy(data3, ptr + 169, sizeof(data3)-1);
        player2Id = player1Id;
        data4 = 0;
    }
}

bytestring TPlayerInfo::asSubPacket() const
{
    bytestring result;
    result.push_back(std::uint8_t(SubPacketCode::PLAYER_INFO_20));
    result.append(fill1, fill1 + sizeof(fill1));
    serialise(result, width);
    serialise(result, height);
    result.push_back(fill3);
    serialise(result, player1Id);
    result.append(data2, data2 + sizeof(data2));
    result.push_back(clicked);
    result.append(fill2, fill2 + sizeof(fill2));
    serialise(result, maxUnits);
    result.push_back(versionMajor);
    result.push_back(versionMinor);
    result.append(data3, data3 + sizeof(data3));
    serialise(result, player2Id);
    result.push_back(data4);
    return result.substr(0, serialisedSize);
}

void TPlayerInfo::setDpId(std::uint32_t dpid)
{
    player1Id = player2Id = dpid;
}

void TPlayerInfo::setInternalVersion(std::uint8_t v)
{
    // offset 188 once smartpaked
    data3[188-177] = v;
}

void TPlayerInfo::setAllowWatch(bool allowWatch)
{
    // offset 163 once smartpaked
    if (allowWatch)
    {
        clicked |= 0x80;
    }
    else
    {
        clicked &= ~0x80;
    }
}

void TPlayerInfo::setCheat(bool isCheat)
{
    // offset 164 once smartpaked
    if (isCheat)
    {
        fill2[0] |= 0x20;
    }
    else
    {
        fill2[0] &= ~0x20;
    }
}

void TPlayerInfo::setPermLos(bool permLos)
{
    // offset 164 once smartpaked
    fill2[0] = 0x08;

    // crashville
    /*
    if (permLos)
    {
        fill2[0] |= 0x08;
    }
    else
    {
        fill2[0] &= ~0x08;
    }
    */
}

std::uint8_t TPlayerInfo::getPermLosByte()
{
    return fill2[0];
}

bool TPlayerInfo::isClickedIn()
{
    // offset 163 once smartpaked
    return (clicked & 0x20) != 0u;
}

bool TPlayerInfo::isWatcher()
{
    return (clicked & 0x40) != 0u;
}

bool TPlayerInfo::isCheatsEnabled()
{
    return (fill2[0] & 0x20) != 0u;
}

bool TPlayerInfo::isAI()
{
    return data2[0] == 2;
}

std::int8_t TPlayerInfo::getSide()
{
    return data2[1];
}

std::uint8_t TPlayerInfo::getSlotNumber()
{
    return data2[2];
}

std::string TPlayerInfo::getMapName()
{
    return (const char*)fill1;
}

std::uint32_t TPlayerInfo::getMapHash()
{
    return *(std::uint32_t*)&data3[0];
}

TAlliance::TAlliance()
{
    dpidFrom = dpidTo = alliedToWithFrom = alliedFromWithTo = 0;
}

TAlliance::TAlliance(const bytestring& subPacket)
{
    const std::uint8_t* ptr = subPacket.data() + 1;
    dpidFrom = toUint32(ptr);
    dpidTo = toUint32(ptr + 4);
    alliedFromWithTo = ptr[8];
    alliedToWithFrom = toUint32(ptr + 9);
}

bytestring TAlliance::asSubPacket() const
{
    bytestring result;
    result.push_back(std::uint8_t(SubPacketCode::ALLY_23));
    serialise(result, dpidFrom);
    serialise(result, dpidTo);
    result.push_back(alliedFromWithTo);
    serialise(result, alliedToWithFrom);
    return result;
}

TTeam::TTeam()
{
    dpidFrom = 0;
    teamNumber = 5; // no team
}

TTeam::TTeam(const bytestring& subPacket)
{
    const std::uint8_t* ptr = subPacket.data() + 1;
    dpidFrom = toUint32(ptr);
    teamNumber = ptr[4];
}

bytestring TTeam::asSubPacket() const
{
    bytestring result;
    result.push_back(std::uint8_t(SubPacketCode::ALLY_23));
    serialise(result, dpidFrom);
    result.push_back(teamNumber);
    return result;
}

TIdent2::TIdent2()
{ 
    std::memset(dpids, 0, sizeof(dpids));
}

TIdent2::TIdent2(const bytestring& subPacket)
{
    const std::uint8_t* ptr = subPacket.data() + 1;
    for (std::size_t n = 0u; n < 10u; ++n, ptr += 4)
    {
        dpids[n] = toUint32(ptr);
    }
}

bytestring TIdent2::asSubPacket() const
{
    bytestring result;
    result.push_back(std::uint8_t(SubPacketCode::IDENT2_26));
    for (std::size_t n = 0u; n < 10u; ++n)
    {
        serialise(result, dpids[n]);
    }
    return result;
}

TIdent3::TIdent3(std::uint32_t playerDpId, std::uint8_t playerNumber):
    dpid(playerDpId),
    number(playerNumber)
{ }

TIdent3::TIdent3(const bytestring& subPacket)
{
    dpid = toUint32(subPacket.data() + 1);
    number = subPacket[5];
}

bytestring TIdent3::asSubPacket() const
{
    bytestring result;
    result.push_back(std::uint8_t(SubPacketCode::IDENT3_22));
    serialise(result, dpid);
    result.push_back(number);
    return result;
}

TUnitData::TUnitData()
{
    std::memset(this, 0, sizeof(this));
    pktid = SubPacketCode::UNIT_DATA_1A;
}

TUnitData::TUnitData(std::uint32_t id, std::uint16_t limit, bool inUse):
    pktid(SubPacketCode::UNIT_DATA_1A),
    sub(0x03),
    fill(0u),
    id(id)
{
    if (inUse)
    {
        u.statusAndLimit[0] = 0x0101;
        u.statusAndLimit[1] = limit;
    }
    else
    {
        u.statusAndLimit[0] = 0x0001;
        u.statusAndLimit[1] = 0xffff;
    }
}

TUnitData::TUnitData(const bytestring& subPacket)
{
    const std::uint8_t* ptr = subPacket.data();
    pktid = SubPacketCode(ptr[0]);
    sub = ptr[1];
    fill = toUint32(ptr + 2);
    id = toUint32(ptr + 6);
    if (sub == 0x03)
    {
        u.statusAndLimit[0] = toUint16(ptr + 10);
        u.statusAndLimit[1] = toUint16(ptr + 12);
    }
    else
    {
        u.crc = toUint32(ptr + 10);
    }
}

bytestring TUnitData::asSubPacket() const
{
    bytestring bs;
    bs.push_back(std::uint8_t(pktid));
    bs.push_back(sub);
    serialise(bs, fill);
    serialise(bs, id);
    if (sub == 0x03)
    {
        serialise(bs, u.statusAndLimit[0]);
        serialise(bs, u.statusAndLimit[1]);
    }
    else
    {
        serialise(bs, u.crc);
    }
    return bs;
}

TProgress::TProgress(std::uint8_t percent):
    percent(percent),
    data(0x06)
{ }

TProgress::TProgress(const bytestring& subPacket):
    percent(subPacket[1]),
    data(subPacket[2])
{ }

bytestring TProgress::asSubPacket() const
{
    bytestring result;
    result.push_back(std::uint8_t(SubPacketCode::LOADING_PROGRESS_2A));
    result.push_back(percent);
    result.push_back(data);
    return result;
}
