#pragma once

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
	ChallengeRequest = 1,
	ChallengeReply = 2,
	TDrawVersionRequest = 3,
	Gp3VersionRequest = 4,
	TPlayVersionRequest = 5,
	ExeVersionRequest = 6,
	AllVersionRequest = 7
};

#pragma pack(1)
struct ChallengeResponseMessage {
	char chatByte;	// 0x05
	char nullText;  // 0x00
	char msgId;		// 0x2b
	short size;		// sizeof(ChallengeReponseMessage)
	ChallengeResponseCommand command;
	unsigned data[2];// for requests, data[0] is the nonce; for replys data are the crcs
	char pad[51];
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
	unsigned NewNonse();
	void ComputeChallengeResponse(unsigned nonse, unsigned crcs[2]);
	void InitPlayerResponse(unsigned dpid, unsigned nonse);
	void SetPlayerResponse(unsigned dpid, unsigned crc1, unsigned crc2);
	void VerifyResponses();
	void Blit(LPVOID lpSurfaceMem, int dwWidth, int dwHeight, int lPitch);
	void Blit(OFFSCREEN* offscreen);
	void SetBroadcastNoReplyWarnings(bool broadcastNoReplyWarnings);

	static std::string GetReportString(const std::pair<unsigned, std::string> &crcAndFilename, const std::string* optionalVersion);
	std::string GetAllReportString();

	std::string GetTDrawVersionString();
	std::pair<unsigned, std::string> GetTotalACrc();
	std::pair<unsigned, std::string> GetTDrawCrc();
	std::pair<unsigned, std::string> GetTPlayXCrc();
	std::pair<unsigned, std::string> GetGp3Crc();

private:
	static std::unique_ptr<ChallengeResponse> m_instance;

	std::shared_ptr<FeatureStruct[]> m_featureMapSnapshot;
	std::vector< std::shared_ptr<std::vector<unsigned char> > > m_moduleInitialDiskSnapshots;
	std::vector< std::string > m_moduleInitialDiskSnapshotFilenames;
	std::vector< std::unique_ptr<SingleHook> > m_hooks;
	std::map<unsigned, std::tuple<unsigned, unsigned, unsigned> > m_responses;	// nonse, crc1, crc2
	taflib::CRC32 m_crc;
	std::mt19937 m_rng;
	bool m_broadcastNoReplyWarnings;
	std::vector<std::string> m_persistentCheatWarnings;
	std::vector<bool> getUsedWeaponIds();

	std::vector<std::string> GetModules();
	void SnapshotModule(HMODULE hModule);
	void SnapshotFile(const char* filename);

	void CrcModules(unsigned* crc);
	void CrcWeapons(unsigned* crc);
	void CrcFeatures(unsigned* crc);
	void CrcUnits(unsigned* crc);
	void CrcGamingState(unsigned* crc);
	void CrcMapSnapshot(unsigned* crc);

	void LogAll(const std::string &filename);
	void LogWeapons(const std::string& filename);
	void LogFeatures(const std::string& filename);
	void LogUnits(const std::string& filename);
	void LogGamingState(const std::string& filename);
	void LogMapSnapshot(const std::string& filename);

	static void handleBattleroomReportCommand(const std::string& str, ChallengeResponseCommand cmd);
};
