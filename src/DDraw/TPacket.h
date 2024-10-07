#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace tapacket
{

    typedef std::basic_string<std::uint8_t> bytestring;

    enum class PacketCode
    {
        UNCOMPRESSED = 0x03,
        COMPRESSED = 0x04
    };

    enum class SubPacketCode
    {
        ZERO_00 = 0x00,
        PING_02 = 0x02,
        UNK_03 = 0x03,
        CHAT_05 = 0x05,
        PAD_ENCRYPT_06 = 0x06,
        UNK_07 = 0x07,
        LOADING_STARTED_08 = 0x08,
        UNIT_BUILD_STARTED_09 = 0x09,
        UNK_0A = 0x0a,
        UNIT_TAKE_DAMAGE_0B = 0x0b,
        UNIT_KILLED_0C = 0x0c,
        WEAPON_FIRED_0D = 0x0d,
        AREA_OF_EFFECT_0E = 0x0e,
        FEATURE_ACTION_0F = 0x0f,
        UNIT_START_SCRIPT_10 = 0x10,
        UNIT_STATE_11 = 0x11,
        UNIT_BUILD_FINISHED_12 = 0x12,
        PLAY_SOUND_13 = 0x13,
        GIVE_UNIT_14 = 0x14,
        START_15 = 0x15,
        SHARE_RESOURCES_16 = 0x16,
        UNK_17 = 0x17,
        HOST_MIGRATION_18 = 0x18,
        SPEED_19 = 0x19,
        UNIT_DATA_1A = 0x1a,
        REJECT_1B = 0x1b,
        START_1E = 0x1e,
        UNK_1F = 0x1f,
        PLAYER_INFO_20 = 0x20,
        UNK_21 = 0x21,
        IDENT3_22 = 0x22,
        ALLY_23 = 0x23,
        TEAM_24 = 0x24,
        IDENT2_26  = 0x26,
        PLAYER_RESOURCE_INFO_28 = 0x28,
        UNK_29  = 0x29,
        LOADING_PROGRESS_2A  = 0x2a,
        UNIT_STAT_AND_MOVE_2C = 0x2c,
        UNK_2E = 0x2e,
        THALDREN_EXTENDED_42 = 0x42,
        UNK_F6  = 0xf6,
        ALLY_CHAT_F9 = 0xf9,
        REPLAYER_SERVER_FA = 0xfa,
        RECORDER_DATA_CONNECT_FB   = 0xfb,
        MAP_POSITION_FC    = 0xfc,
        SMARTPAK_TICK_OTHER_FD = 0xfd,
        SMARTPAK_TICK_START_FE= 0xfe,
        SMARTPAK_TICK_FF  = 0xff
    };

    class TPing
    {
    public:
        TPing(std::uint32_t from, std::uint32_t id, std::uint32_t value);
        TPing(const bytestring& subPacket);
        bytestring asSubPacket() const;

        std::uint32_t from;
        std::uint32_t id;
        std::uint32_t value;
    };

    class TPlayerInfo
    {
    public:
        TPlayerInfo(const bytestring& subPacket);
        bytestring asSubPacket() const;
        unsigned serialisedSize;

        void setDpId(std::uint32_t dpid);
        void setInternalVersion(std::uint8_t v);
        void setAllowWatch(bool allowWatch);
        void setCheat(bool allowCheats);
        void setPermLos(bool permLos);
        bool isClickedIn();
        bool isWatcher();
        bool isCheatsEnabled();
        std::uint8_t getPermLosByte();
        bool isAI();
        std::int8_t getSide();
        std::uint8_t getSlotNumber();   // actually this is color
        std::string getMapName();
        std::uint32_t getMapHash();
                                        //record        +typecode   +smartpak
        std::uint8_t fill1[139];        //0             //1         //8  
        std::uint16_t width;            //139           //140       //147
        std::uint16_t height;           //141           //142       //149
        std::uint8_t fill3;             //143           //144       //151
        std::uint32_t player1Id;        //144           //145       //152
        std::uint8_t data2[7];          //148           //149       //156
        std::uint8_t clicked;           //155           //156       //163
        std::uint8_t fill2[9];          //156           //157       //164
        std::uint16_t maxUnits;         //165           //166       //173
        std::uint8_t versionMajor;      //167           //168       //175
        std::uint8_t versionMinor;      //168           //169       //176
        std::uint8_t data3[17];         //169  7=182    //170       //177
        std::uint32_t player2Id;        //186           //187       //194
        std::uint8_t data4;             //190           //191       //198
    };                                  //191           //192       //199

    class TAlliance
    {
    public:
        TAlliance();
        TAlliance(const bytestring& subPacket);
        bytestring asSubPacket() const;

                                        //record        +typecode
        std::uint32_t dpidFrom;         //0             //1
        std::uint32_t dpidTo;           //4             //5
        std::uint8_t alliedFromWithTo;  //8             //9
        std::uint32_t alliedToWithFrom; //9             //10
                                        //13            //14
    };

    class TTeam
    {
    public:
        TTeam();
        TTeam(const bytestring& subPacket);
        bytestring asSubPacket() const;

                                        //record        +typecode
        std::uint32_t dpidFrom;         //0             //1
        std::uint8_t teamNumber;        //4             //5
                                        //5             //6
    };

    class TIdent2
    {
    public:
        TIdent2();
        TIdent2(const bytestring& subPacket);
        bytestring asSubPacket() const;

        std::uint32_t dpids[10];
    };

    class TIdent3
    {
    public:
        TIdent3(std::uint32_t playerDpId, std::uint8_t playerNumber);
        TIdent3(const bytestring& subPacket);
        bytestring asSubPacket() const;

        std::uint32_t dpid;
        std::uint8_t number;
    };

    class TUnitData
    {
    public:
        TUnitData();
        TUnitData(std::uint32_t id, std::uint16_t limit, bool inUse);
        TUnitData(const bytestring& subPacket);
        bytestring asSubPacket() const;
                                                    // +typecode    +smartpak
        tapacket::SubPacketCode pktid;                // 0            // 7
        std::uint8_t sub;                           // 1            // 8
        std::uint32_t fill;                         // 2            // 9
        std::uint32_t id;                           // 6            // 13
        union
        {
            std::uint16_t statusAndLimit[2];        // 10           // 17
            std::uint32_t crc;                      // 10           // 17
        } u;                                        // 14           // 21
    };

    class TProgress
    {
    public:
        TProgress(std::uint8_t percent);
        TProgress(const bytestring& subPacket);
        bytestring asSubPacket() const;

        // NB size of this packet doesn't gel with getExpectedSubPacketSize
        // but the extra "data" byte appears to be always 0x06
        // which getExpectedSubPacketSize() eats up as a one byte opcode
        std::uint8_t percent;
        std::uint8_t data;
    };

    class TPacket
    {
    public:
        static std::string toString(SubPacketCode code);
        static unsigned getExpectedSubPacketSize(const bytestring &bytes);
        static unsigned getExpectedSubPacketSize(const std::uint8_t *s, unsigned sz);
        static void decrypt(bytestring& data, std::size_t ofs, std::uint16_t &checkExtracted, std::uint16_t &checkCalculated);
        static void encrypt(bytestring &data);
        static bytestring compress(const bytestring &data);
        static bytestring decompress(const bytestring &data, const unsigned headerSize);
        static bytestring decompress(const std::uint8_t *data, const unsigned len, const unsigned headerSize);
        static bytestring split2(bytestring &s, bool smartpak, bool &error);

        static std::vector<bytestring> slowUnsmartpak(const bytestring &c, bool hasTimestamp, bool hasChecksum);
        static std::vector<bytestring> unsmartpak(const bytestring &c, bool hasTimestamp, bool hasChecksum);
        static bytestring trivialSmartpak(const bytestring& subpacket, std::uint32_t tcpseq);
        static bytestring createChatSubpacket(const std::string& message);
        static bytestring createHostMigrationSubpacket(int playerNumber);

        // @param s string who's characters contain bits of an integer <char1>,<char2>,<char3>,<char4>,etc
        // @param start number of bits into the string from where the number starts.  eg 0 starts at LSB of char1.  8 starts at LSB of char2.
        // @param num number of bits to extract
        static unsigned bin2int(const bytestring &s, unsigned start, unsigned num);

        static void test();
        static int testDecode(const bytestring &data, bool isEncrypted, bool hasTimestamp, bool hasChecksum, std::uint8_t filter);
        static void testCompression(unsigned dictionarySize);
        static bool testUnpakability(bytestring s, int recurseDepth);
    };

    /// @brief apply smartpak compression to type 0x2c (UNIT_STAT_AND_MOVE_2C) subpackets
    class SmartPaker
    {
        bool m_first;
    public:
        SmartPaker();
        bytestring operator() (const bytestring& subpak); 
    };

    struct DPAddress;
    class TaPacketHandler
    {
    public:
        virtual void onDplaySuperEnumPlayerReply(std::uint32_t dplayId, const std::string &playerName, DPAddress *tcp, DPAddress *udp) = 0;
        virtual void onDplayCreateOrForwardPlayer(std::uint16_t command, std::uint32_t dplayId, const std::string &name, DPAddress *tcp, DPAddress *udp) = 0;
        virtual void onDplayDeletePlayer(std::uint32_t dplayId) = 0;

        virtual void onTaPacket(std::uint32_t sourceDplayId, std::uint32_t otherDplayId, bool isLocalSource, const char* encrypted, int sizeEncrypted, const std::vector<bytestring>& subpaks) = 0;
    };

}
