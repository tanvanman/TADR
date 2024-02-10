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

class ChallengeResponse
{
public:
	ChallengeResponse();

	static ChallengeResponse* GetInstance();

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

private:
	static std::unique_ptr<ChallengeResponse> m_instance;

	std::shared_ptr<FeatureStruct[]> m_featureMapSnapshot;
	std::vector< std::shared_ptr<std::vector<unsigned char> > > m_moduleInitialDiskSnapshots;
	std::vector< std::unique_ptr<SingleHook> > m_hooks;
	std::map<unsigned, std::tuple<unsigned, unsigned, unsigned> > m_responses;	// nonse, crc1, crc2
	taflib::CRC32 m_crc;
	std::mt19937 m_rng;

	std::vector<std::string> m_persistentCheatWarnings;

	void SnapshotModule(HMODULE hModule);
	void SnapshotFile(const char* filename);
	void CrcModules(unsigned* crc);
	void CrcWeapons(unsigned* crc);
	void CrcFeatures(unsigned* crc);
	void CrcUnits(unsigned* crc);
	void CrcGamingState(unsigned* crc);
	void CrcMapSnapshot(unsigned* crc);
};
