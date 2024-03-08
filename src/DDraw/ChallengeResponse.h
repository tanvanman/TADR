#pragma once

#include "HmacSha256.h"
#include "nswfl_crc32.h"
#include "tamem.h"

#include <memory>
#include <vector>
#include <map>
#include <random>

#include "windows.h"

#undef min
#undef max

class SingleHook;
struct FeatureStruct;

enum class ChallengeResponseCommand : char {
	ChallengeRequest = 1,				// Request remote players to calculate and reply verification hashes
	LegacyCrc32Reply = 2,
	TDrawVersionRequest = 3,			// Request remote players to reply advertise their TDraw crc in chat
	Gp3VersionRequest = 4,				// Request remote players to reply advertise their gp3 crc in chat
	TPlayVersionRequest = 5,			// Request remote players to reply advertise their D/TPlayX crc in chat
	ExeVersionRequest = 6,				// Request remote players to reply advertise their TotalA.exe crc in chat
	AllVersionRequest = 7,				// Request remote players to reply advertise all crcs in chat
	ChallengeHashReplyModules = 32,		// Challenge Reply with hash of exe/dll modules
	ChallengeHashReplyGameData = 33,	// Challenge Reply with hash of game data
};

#pragma pack(1)
struct ChallengeResponseMessage {
	char chatByte;	// 0x05
	char nullText;  // 0x00
	char msgId;		// 0x2b
	short size;		// sizeof(ChallengeReponseMessage)
	ChallengeResponseCommand command;
	char data[32];  // for requests, data is the nonce; for replys data is the hash
	char pad[27];
};	// 65 bytes

#pragma pack()

class ChallengeResponse
{
public:
	ChallengeResponse();

	static ChallengeResponse* GetInstance();

	static void OnBattleroomCommandExeReport(const std::vector<std::string>&);
	static void OnBattleroomCommandTdReport(const std::vector<std::string>&);
	static void OnBattleroomCommandTpReport(const std::vector<std::string>&);
	static void OnBattleroomCommandGp3Report(const std::vector<std::string>&);
	static void OnBattleroomCommandCrcReport(const std::vector<std::string>&);

	void SnapshotModules();
	void SnapshotFeatureMap();
	void ClearPersistentMessages();
	void NewNonse(char *nonse, int len);
	void ComputeChallengeResponse(const char *nonse, char *modulesHash, char *gameDataHash);
	void InitPlayerResponse(unsigned dpid, char *nonse, int len);
	void SetPlayerModulesResponse(unsigned dpid, const char *hash, int len);
	void SetPlayerGameDataResponse(unsigned dpid, const char *hash, int len);
	void VerifyResponses();
	void Blit(LPVOID lpSurfaceMem, int dwWidth, int dwHeight, int lPitch);
	void Blit(OFFSCREEN* offscreen);

	static std::string GetReportString(const std::pair<unsigned, std::string> &crcAndFilename, const std::string* optionalVersion);
	std::string GetAllReportString();

	std::string GetTDrawVersionString();
	std::pair<unsigned, std::string> GetTotalACrc();
	std::pair<unsigned, std::string> GetTDrawCrc();
	std::pair<unsigned, std::string> GetTPlayXCrc();
	std::pair<unsigned, std::string> GetGp3Crc();

private:

	struct ResponseContext
	{
		bool modulesResponseReceived;
		bool gameDataResponseReceived;
		char nonse[SHA256_DIGEST_LENGTH];
		char modulesResponse[SHA256_DIGEST_LENGTH];
		char gameDataResponse[SHA256_DIGEST_LENGTH];
	};

	static std::unique_ptr<ChallengeResponse> m_instance;

	static const unsigned FEATURE_MASK = 0x0fff;
	static const unsigned LOS_TYPE_MASK = 7u;
	static const unsigned SOFTWARE_DEBUG_MODE_MASK =
		softwaredebugmode::CheatsEnabled |
		softwaredebugmode::InvulnerableFeatures |
		softwaredebugmode::Doubleshot |
		softwaredebugmode::Halfshot |
		softwaredebugmode::Radar;

	std::shared_ptr<FeatureStruct[]> m_featureMapSnapshot;
	std::vector< std::shared_ptr<std::vector<unsigned char> > > m_moduleInitialDiskSnapshots;
	std::vector< std::string > m_moduleInitialDiskSnapshotFilenames;
	std::vector< std::unique_ptr<SingleHook> > m_hooks;
	std::map<unsigned, ResponseContext> m_responses;	// keyed by dpid

	taflib::CRC32 m_crc;
	std::mt19937 m_rng;
	std::vector<std::string> m_persistentCheatWarnings;
	std::vector<bool> getUsedWeaponIds();

	std::vector<std::string> GetModulePaths();
	void SnapshotModule(HMODULE hModule);
	void SnapshotFile(const char* filename);

	void HashModules(HmacSha256Calculator &hmac);
	void HashWeapons(HmacSha256Calculator& hmac);
	void HashFeatures(HmacSha256Calculator& hmac);
	void HashUnits(HmacSha256Calculator& hmac);
	void HashGamingState(HmacSha256Calculator& hmac);
	void HashMapSnapshot(HmacSha256Calculator& hmac);

	void LogAll(const std::string &filename);
	void LogWeapons(const std::string& filename);
	void LogFeatures(const std::string& filename);
	void LogUnits(const std::string& filename);
	void LogGamingState(const std::string& filename);
	void LogMapSnapshot(const std::string& filename);
	void LogGameFileLookup(const std::string& filename);

	static void handleBattleroomReportCommand(const std::string& str, ChallengeResponseCommand cmd);
};
