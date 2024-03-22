#include "ChallengeResponse.h"
#include "BattleroomCommands.h"
#include "hook/hook.h"
#include "iddrawsurface.h"
#include "random_code_seg_keys.h"
#include "tafunctions.h"

#include <fstream>
#include <sstream>

#include <psapi.h>
#include <wincrypt.h>

#pragma comment(lib, "Version.lib")

#define CONCAT_HELPER(x, y) x ## y
#define CONCAT(x, y) CONCAT_HELPER(x, y)
#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)
#define RANDOM_CODE_SEG_NEXT CONCAT(RANDOM_CODE_SEG_, __COUNTER__)
//#define RANDOM_CODE_SEG_NEXT 0

std::unique_ptr<ChallengeResponse> ChallengeResponse::m_instance = NULL;

extern HINSTANCE HInstance;

static void InitChallengeResponseMessage(ChallengeResponseMessage &msg, ChallengeResponseCommand cmd)
{
	std::memset(&msg, 0, sizeof(msg));
	msg.chatByte = 0x05;
	msg.nullText = 0x00;
	msg.msgId = 0x2b;
	msg.size = sizeof(ChallengeResponseMessage);
	msg.command = cmd;
}

static bool IsChallengeResponseMessage(const ChallengeResponseMessage& msg)
{
	return msg.chatByte == 0x05 && msg.nullText == 0x00 && msg.msgId == 0x2b && msg.size == sizeof(ChallengeResponseMessage);
}

static std::string toLowerCase(const std::string& str) {
	std::string result;
	for (char c : str) {
		result += std::tolower(c);
	}
	return result;
}

static bool endsWith(const std::string& fullString, const std::string& ending) {
	if (fullString.length() >= ending.length()) {
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	}
	else {
		return false;
	}
}

static std::tuple<std::string, std::string, std::string> getFilenameComponents(const std::string& path) {
	std::string directory;
	std::string filename;
	std::string extension;

	std::size_t pos = path.find_last_of('\\');
	if (pos != std::string::npos) {
		directory = path.substr(0, pos);
		filename = path.substr(pos + 1);
	}
	else {
		filename = path;
	}

	pos = filename.find_last_of('.');
	if (pos != std::string::npos) {
		std::string temp = filename;
		filename = temp.substr(0, pos);
		extension = temp.substr(pos + 1);
	}

	return std::make_tuple(directory, filename, extension);
}

static void encodeInteger(unsigned x, char* s, int nDigits)
{
	// all unique TA-printable characters
	const char digits[] =
		"!\"#$%&'*+,-./0123456789:;<=>?"
		"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
		"`abcdefghijklmnopqrstuvwxyz{|}~"
		"\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F"
		"\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F"
		"\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF"
		"\xB0\xB1"
		"\xCD"
		"\xD3\xDA\xDF";

	const int base = std::strlen(digits);

	if (nDigits < 1) {
		nDigits = 1;
	}

	int i = 0;
	s[nDigits] = '\0';

	for (i = nDigits-1; i >= 0; i--) {
		s[i] = digits[x % base];
		x /= base;
	}
}

static void broadcastChatMessage(const std::string& text)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	char buffer[65] = { 0 };
	buffer[0] = 0x05; // chat
	std::memcpy(buffer + 1, text.c_str(), std::min(text.size(), sizeof(buffer)-2));
	unsigned fromDpid = taPtr->Players[taPtr->LocalHumanPlayer_PlayerID].DirectPlayID;
	HAPI_BroadcastMessage(fromDpid, buffer, sizeof(buffer));
}

// keep a record of which units, weapon and features come from which file/hpi/ccx/ufo/gp3
static std::map<std::string, std::string> gamePathToHpiLookup;
static std::string UnitsStr = toLowerCase((const char*)0x503920);
static std::string WeaponsStr = toLowerCase((const char*)0x50392c);
static std::string FeaturesStr = toLowerCase((const char*)0x502c7c);
static std::string ScriptsStr = toLowerCase((const char*)0x503740);

// requested game path is file on the native file system
unsigned int LogGamePathAddr1 = 0x4bb332;
#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
int __stdcall LogGamePathProc1(PInlineX86StackBuffer X86StrackBuffer)
{
	if (X86StrackBuffer->Eax) {
		std::string gameFile = toLowerCase((const char*)X86StrackBuffer->Edi);
		if (gameFile.find(UnitsStr) == 0 || gameFile.find(WeaponsStr) == 0 || gameFile.find(FeaturesStr) == 0 || gameFile.find(ScriptsStr) == 0) {
			gamePathToHpiLookup[gameFile] = gameFile;
		}
	}
	return 0;
}
#pragma code_seg(pop)

// request game path is in an hpi/ccx/ufo/gp3 archive
unsigned int LogGamePathAddr2 = 0x4bb3a7;
#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
int __stdcall LogGamePathProc2(PInlineX86StackBuffer X86StrackBuffer)
{
	std::string gameFile = toLowerCase(*(const char**)(X86StrackBuffer->Esp + 0x14));
	std::string hpiFile = (const char*)(X86StrackBuffer->Eax + 0x14);
	if (gameFile.find(UnitsStr) == 0 || gameFile.find(WeaponsStr) == 0 || gameFile.find(FeaturesStr) == 0 || gameFile.find(ScriptsStr) == 0) {
		gamePathToHpiLookup[gameFile] = hpiFile;
	}
	return 0;
}
#pragma code_seg(pop)

unsigned int ChallengeResponseUpdateAddr = 0x4954b7;
#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
int __stdcall ChallengeResponseUpdateProc(PInlineX86StackBuffer X86StrackBuffer)
{
	// send out challenge-response requests at the right time
	// and then later make a big song and dance about anyone who failed or who hasn't replied
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	PlayerStruct* me = &taPtr->Players[taPtr->LocalHumanPlayer_PlayerID];
	if (!me->PlayerActive || me->DirectPlayID == 0 || (me->PlayerInfo->PropertyMask & WATCH) || DataShare->PlayingDemo) {
		return 0;
	}

	auto *cr = ChallengeResponse::GetInstance();
	if (taPtr->GameTime == 0) {
		cr->SnapshotFeatureMap();
		cr->ClearPersistentMessages();
	}

	else if (taPtr->GameTime == 150) {
		for (int n = 0; n < 10; ++n) {
			PlayerStruct* p = &taPtr->Players[n];
			if (p->PlayerActive && p->DirectPlayID != 0 && GetInferredPlayerType(p) == Player_RemoteHuman && !(p->PlayerInfo->PropertyMask & WATCH)) {
				ChallengeResponseMessage msg;
				InitChallengeResponseMessage(msg, ChallengeResponseCommand::ChallengeRequest);
				cr->NewNonse(msg.data, sizeof(msg.data));
				cr->InitPlayerResponse(p->DirectPlayID, msg.data, sizeof(msg.data));
				HAPI_SendBuf(me->DirectPlayID, p->DirectPlayID, (const char*) & msg, sizeof(msg));
			}
		}
	}

	else if (taPtr->GameTime == 300) {
		ChallengeResponse::GetInstance()->VerifyResponses();
	}

	return 0;
}
#pragma code_seg(pop)

unsigned int ReceiveChallengeOrResponseAddr = 0x45522e;
#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
int __stdcall ReceiveChallengeOrResponseProc(PInlineX86StackBuffer X86StrackBuffer)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	PlayerStruct* me = &taPtr->Players[taPtr->LocalHumanPlayer_PlayerID];
	if (!me->PlayerActive || me->DirectPlayID == 0 || (me->PlayerInfo->PropertyMask & WATCH) || DataShare->PlayingDemo) {
		return 0;
	}

	const ChallengeResponseMessage *msg = (ChallengeResponseMessage*)taPtr->PacketBuffer_p;
	if (!msg || !IsChallengeResponseMessage(*msg)) {
		return 0;
	}

	switch (msg->command) {
	case ChallengeResponseCommand::ChallengeRequest: {
		ChallengeResponseMessage replies[2];
		InitChallengeResponseMessage(replies[0], ChallengeResponseCommand::ChallengeHashReplyModules);
		InitChallengeResponseMessage(replies[1], ChallengeResponseCommand::ChallengeHashReplyGameData);
		ChallengeResponse::GetInstance()->ComputeChallengeResponse(msg->data, replies[0].data, replies[1].data);

		unsigned fromDpid = taPtr->Players[taPtr->LocalHumanPlayer_PlayerID].DirectPlayID;
		unsigned replyDpid = taPtr->HAPI_net_data0;
		HAPI_SendBuf(fromDpid, replyDpid, (char*)&replies[0], sizeof(replies));
		break;
	}

	case ChallengeResponseCommand::LegacyCrc32Reply: {
		ChallengeResponse::GetInstance()->SetPlayerModulesResponse(taPtr->HAPI_net_data0, msg->data, 4);
		ChallengeResponse::GetInstance()->SetPlayerGameDataResponse(taPtr->HAPI_net_data0, msg->data+4, 4);
		break;
	}

	case ChallengeResponseCommand::ChallengeHashReplyModules: {
		ChallengeResponse::GetInstance()->SetPlayerModulesResponse(taPtr->HAPI_net_data0, msg->data, SHA256_DIGEST_LENGTH);
		break;
	}

	case ChallengeResponseCommand::ChallengeHashReplyGameData: {
		ChallengeResponse::GetInstance()->SetPlayerGameDataResponse(taPtr->HAPI_net_data0, msg->data, SHA256_DIGEST_LENGTH);
		break;
	}

	case ChallengeResponseCommand::TDrawVersionRequest: {
		std::string versionString = ChallengeResponse::GetInstance()->GetTDrawVersionString();
		auto crcAndFilename = ChallengeResponse::GetInstance()->GetTDrawCrc();
		std::string str = ChallengeResponse::GetReportString(crcAndFilename, &versionString);
		ChatText(str.c_str());
		broadcastChatMessage(str);
		break;
	}

	case ChallengeResponseCommand::Gp3VersionRequest: {
		auto crcAndFilename = ChallengeResponse::GetInstance()->GetGp3Crc();
		std::string str = ChallengeResponse::GetReportString(crcAndFilename, NULL);
		ChatText(str.c_str());
		broadcastChatMessage(str);
		break;
	}

	case ChallengeResponseCommand::TPlayVersionRequest: {
		auto crcAndFilename = ChallengeResponse::GetInstance()->GetTPlayXCrc();
		std::string str = ChallengeResponse::GetReportString(crcAndFilename, NULL);
		ChatText(str.c_str());
		broadcastChatMessage(str);
		break;
	}

	case ChallengeResponseCommand::ExeVersionRequest: {
		auto crcAndFilename = ChallengeResponse::GetInstance()->GetTotalACrc();
		std::string str = ChallengeResponse::GetReportString(crcAndFilename, NULL);
		ChatText(str.c_str());
		broadcastChatMessage(str);
		break;
	}

	case ChallengeResponseCommand::AllVersionRequest: {
		std::string str = ChallengeResponse::GetInstance()->GetAllReportString();
		ChatText(str.c_str());
		broadcastChatMessage(str);
		break;
	}

	}
	return 0;
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
ChallengeResponse::ChallengeResponse():
	m_rng(std::random_device()())
{
#ifndef NDEBUG
	std::ofstream fs("ErrorLog.txt");	// clear the errorlog
#endif

	m_crc.Initialize();
	m_hooks.push_back(std::make_unique<InlineSingleHook>(ChallengeResponseUpdateAddr, 5, INLINE_5BYTESLAGGERJMP, ChallengeResponseUpdateProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(ReceiveChallengeOrResponseAddr, 5, INLINE_5BYTESLAGGERJMP, ReceiveChallengeOrResponseProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(LogGamePathAddr1, 5, INLINE_5BYTESLAGGERJMP, LogGamePathProc1));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(LogGamePathAddr2, 5, INLINE_5BYTESLAGGERJMP, LogGamePathProc2));

	SnapshotModules();

	BattleroomCommands::GetInstance()->RegisterCommand(".exereport", &ChallengeResponse::OnBattleroomCommandExeReport);
	BattleroomCommands::GetInstance()->RegisterCommand(".tdreport", &ChallengeResponse::OnBattleroomCommandTdReport);
	BattleroomCommands::GetInstance()->RegisterCommand(".tpreport", &ChallengeResponse::OnBattleroomCommandTpReport);
	BattleroomCommands::GetInstance()->RegisterCommand(".gp3report", &ChallengeResponse::OnBattleroomCommandGp3Report);
	BattleroomCommands::GetInstance()->RegisterCommand(".crcreport", &ChallengeResponse::OnBattleroomCommandCrcReport);
}
#pragma code_seg(pop)


#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
ChallengeResponse *ChallengeResponse::GetInstance()
{
	if (m_instance == NULL) {
		m_instance.reset(new ChallengeResponse());
	}
	return m_instance.get();
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::SnapshotFeatureMap()
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	const int N = taPtr->FeatureMapSizeX * taPtr->FeatureMapSizeY;
	std::shared_ptr<FeatureStruct[]> array(new FeatureStruct[N]);

	std::memcpy(array.get(), taPtr->FeatureMap, N * sizeof(FeatureStruct));

	m_featureMapSnapshot = array;
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::SnapshotModules()
{
	HMODULE hModule = NULL;

	// this process (ie TotalA.exe)
	hModule = GetModuleHandle(NULL);
	SnapshotModule(hModule);

	// this dll
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		reinterpret_cast<LPCSTR>(&ChallengeResponseUpdateProc), &hModule);
	SnapshotModule(hModule);

	// all dlls loaded from current working directory
	std::vector<std::string> modules = GetModulePaths();

	// The outermost wrapper of DPlayX.dll
	// Will be the patchloader if the patchloader is being used,
	// otherwise it'll be the recorder.
	std::string DPlayXOutermostWrapper((const char*)0x4ff9e4);
	for (auto it = modules.begin(); it != modules.end(); ++it) {
		std::string path = *it;
		if (endsWith(toLowerCase(path), toLowerCase("\\" + DPlayXOutermostWrapper))) {
			SnapshotFile(path.c_str());
			modules.erase(it);
			break;
		}
	}

	for (auto it = modules.begin(); it != modules.end(); ++it) {
		std::string path = *it;
		if (endsWith(toLowerCase(path), "playx.dll")) {		// inner wrappers (ie the recorder if a patchloader is being used)
			SnapshotFile(path.c_str());
		}
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::SnapshotModule(HMODULE hModule)
{
	CHAR moduleFileName[MAX_PATH];
	GetModuleFileNameEx(GetCurrentProcess(), hModule, moduleFileName, MAX_PATH);
	SnapshotFile(moduleFileName);
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::NewNonse(char* nonse, int len)
{
	for (int n = 0; n < len; n += 4) {
		unsigned x = m_rng();
		std::memcpy(&nonse[n], &x, std::min(4, len - n));
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::ComputeChallengeResponse(const char* _nonse, char* modulesHash, char* gameDataHash)
{
	const unsigned* nonse = (const unsigned*)_nonse;
	unsigned hmacKey[] = {
		RANDOM_CODE_SEG_0, nonse[0],
		RANDOM_CODE_SEG_1, nonse[1],
		RANDOM_CODE_SEG_2, nonse[2],
		RANDOM_CODE_SEG_3, nonse[3],
		RANDOM_CODE_SEG_4, nonse[4],
		RANDOM_CODE_SEG_5, nonse[5],
		RANDOM_CODE_SEG_6, nonse[6],
		RANDOM_CODE_SEG_7, nonse[7]
	};

	try {
		HmacSha256Calculator hmacModules((unsigned char*)hmacKey, sizeof(hmacKey));
		HashModules(hmacModules);
		if (!hmacModules.finalize((unsigned char*)modulesHash, SHA256_DIGEST_LENGTH)) {
			std::memset(modulesHash, 0, SHA256_DIGEST_LENGTH);
		}

		HmacSha256Calculator hmacGameData((unsigned char*)hmacKey, sizeof(hmacKey));
		HashWeapons(hmacGameData);
		HashFeatures(hmacGameData);
		HashUnits(hmacGameData);
		HashGamingState(hmacGameData);
		HashMapSnapshot(hmacGameData);
		if (!hmacGameData.finalize((unsigned char*)gameDataHash, SHA256_DIGEST_LENGTH)) {
			std::memset(gameDataHash, 0, SHA256_DIGEST_LENGTH);
		}
	}
	catch (std::exception& e) {
		std::string errorMessage;
		DWORD errorMessageID = GetLastError();
		if (errorMessageID != 0) {
			LPSTR messageBuffer = nullptr;
			size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
			errorMessage = std::string(messageBuffer, size);
			LocalFree(messageBuffer);
		}

		IDDrawSurface::OutptTxt("Error: %s: %s", e.what(), errorMessage.c_str());
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::InitPlayerResponse(unsigned dpid, char *nonse, int len)
{
	auto& r = m_responses[dpid];
	r.modulesResponseReceived = r.gameDataResponseReceived = false;
	std::memset(r.nonse, 0, sizeof(r.nonse));
	std::memset(r.modulesResponse, 0, sizeof(r.modulesResponse));
	std::memset(r.gameDataResponse, 0, sizeof(r.gameDataResponse));
	std::memcpy(r.nonse, nonse, std::min(sizeof(r.nonse), unsigned(len)));
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::SetPlayerModulesResponse(unsigned dpid, const char* hash, int len)
{
	auto& r = m_responses[dpid];
	r.modulesResponseReceived = true;
	std::memcpy(r.modulesResponse, hash, std::min(sizeof(r.modulesResponse), unsigned(len)));
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::SetPlayerGameDataResponse(unsigned dpid, const char* hash, int len)
{
	auto& r = m_responses[dpid];
	r.gameDataResponseReceived = true;
	std::memcpy(r.gameDataResponse, hash, std::min(sizeof(r.gameDataResponse), unsigned(len)));
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::VerifyResponses()
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	PlayerStruct* localPlayer = &taPtr->Players[taPtr->LocalHumanPlayer_PlayerID];

	for (auto it = m_responses.begin(); it != m_responses.end(); ++it) {
		unsigned dpid = it->first;
		ResponseContext& r = it->second;

		char ourModulesHash[SHA256_DIGEST_LENGTH];
		char ourGameDataHash[SHA256_DIGEST_LENGTH];
		ComputeChallengeResponse(r.nonse, ourModulesHash, ourGameDataHash);

		bool noReply = !r.modulesResponseReceived || !r.gameDataResponseReceived;
		bool modulesMismatch = memcmp(r.modulesResponse, ourModulesHash, SHA256_DIGEST_LENGTH);
		bool gameDataMismatch = memcmp(r.gameDataResponse, ourGameDataHash, SHA256_DIGEST_LENGTH);

		PlayerStruct* p = FindPlayerByDPID(dpid);
		std::string playerName = p ? p->Name : std::to_string(dpid);

		std::string broadcastMessage;
		std::ostringstream ss;
		ss << "*** ";

		if (noReply) {
			ss << playerName << " did not reply to VerCheck query!";
			broadcastMessage = ss.str();
			ss << " Their dll lacks version checking (or packet loss/crash)";
			m_persistentCheatWarnings.push_back(ss.str());
		}
		else if (!modulesMismatch && !gameDataMismatch) {
		}
		else if (modulesMismatch) {
			ss << playerName << '/' << localPlayer->Name << " exe or dll mismatch!";
			broadcastMessage = ss.str();
			ss << " Players use different exe/dll versions";
			m_persistentCheatWarnings.push_back(ss.str());

		}
		else if (gameDataMismatch) {
			ss << playerName << '/' << localPlayer->Name << " game data mismatch!";
			broadcastMessage = ss.str();
			ss << " Compare ErrorLog.txt to diagnose";
			m_persistentCheatWarnings.push_back(ss.str());
			LogAll("ErrorLog.txt");
		}

		if (!broadcastMessage.empty()) {
			SendText(broadcastMessage.c_str(), 0);
			broadcastChatMessage(broadcastMessage);
		}
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::HashModules(HmacSha256Calculator& hmac)
{
	std::size_t M = m_moduleInitialDiskSnapshots.size();
	for (std::size_t m = 0u; m < M; ++m) {
		hmac.processChunk(m_moduleInitialDiskSnapshots[m]->data(), m_moduleInitialDiskSnapshots[m]->size());
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::HashWeapons(HmacSha256Calculator& hmac)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	std::vector<bool> usedWeapons = getUsedWeaponIds();
	for (int n = 0; n < 256; ++n) {
		WeaponStruct* w = &taPtr->Weapons[n];
		if (w->WeaponName[0] != '\0' && usedWeapons[n]) {
			hmac.processChunk((unsigned char*)&w->Damage, 16);
			hmac.processChunk((unsigned char*)&w->ID, 1);
			hmac.processChunk((unsigned char*)&w->WeaponTypeMask, 4);
		}
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::HashFeatures(HmacSha256Calculator& hmac)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	for (int n = 0; n < taPtr->NumFeatureDefs; ++n) {
		FeatureDefStruct* f = &taPtr->FeatureDef[n];
		hmac.processChunk((unsigned char*)&f->FootprintX, 2);
		hmac.processChunk((unsigned char*)&f->FootprintZ, 2);
		if (f->BurnWeapon) hmac.processChunk(&f->BurnWeapon->ID, 1);
		hmac.processChunk((unsigned char*)&f->SparkTime, 2);
		hmac.processChunk((unsigned char*)&f->Damage, 2);
		hmac.processChunk((unsigned char*)&f->Energy, 4);
		hmac.processChunk((unsigned char*)&f->Metal, 4);
		hmac.processChunk((unsigned char*)&f->Height, 1);
		short fm = f->FeatureMask & FEATURE_MASK;
		hmac.processChunk((unsigned char*)&fm, 2);
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::HashUnits(HmacSha256Calculator& hmac)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	for (unsigned n = 0u; n < taPtr->UNITINFOCount; ++n) {
		UnitDefStruct* u = &taPtr->UnitDef[n];
		if (u->buildLimit != 0) {
			hmac.processChunk((unsigned char*)&u->FootX, 4);
			if (u->YardMap) hmac.processChunk((unsigned char*)u->YardMap, u->FootX * u->FootY);
			int buildLimit = u->buildLimit == -1 ? taPtr->PlayerUnitsNumber_Skim : u->buildLimit;
			hmac.processChunk((unsigned char*)&buildLimit, 4);
			hmac.processChunk((unsigned char*)&u->__X_Width, (char*)&u->cobDataPtr - (char*)&u->__X_Width);
			hmac.processChunk((unsigned char*)&u->lRawSpeed_maxvelocity, (char*)&u->weapon1 - (char*)&u->lRawSpeed_maxvelocity);
			if (u->weapon1) hmac.processChunk((unsigned char*)&u->weapon1->ID, 1);
			if (u->weapon2) hmac.processChunk((unsigned char*)&u->weapon2->ID, 1);
			if (u->weapon3) hmac.processChunk((unsigned char*)&u->weapon3->ID, 1);
			if (u->ExplodeAs) hmac.processChunk((unsigned char*)&u->ExplodeAs->ID, 1);
			if (u->SelfeDestructAs) hmac.processChunk((unsigned char*)&u->SelfeDestructAs->ID, 1);
			hmac.processChunk((unsigned char*)&u->nMaxHP, (char*)&u->ExplodeAs - (char*)&u->nMaxHP);
			hmac.processChunk((unsigned char*)&u->maxslope, (char*)&u->wpri_badTargetCategory_MaskAryPtr - (char*)&u->maxslope);
			hmac.processChunk((unsigned char*)&u->UnitTypeMask_0, 2 * sizeof(long));
			if (u->cobDataPtr) {
				CobHeader* c = u->cobDataPtr;
				hmac.processChunk((unsigned char*)&c->Version, sizeof(c->Version));
				hmac.processChunk((unsigned char*)&c->StaticVariablesCount, sizeof(c->StaticVariablesCount));
				hmac.processChunk(c->ByteCodeStart, c->BytecodeLength);
				for (int i = 0; i < c->MethodCount; ++i) {
					hmac.processChunk((unsigned char*)&c->MethodEntryPoints[i], 4);
					hmac.processChunk((unsigned char*)c->MethodNameOffsets[i], std::strlen(c->MethodNameOffsets[i]));
				}
				for (int i = 0; i < c->PieceCount; ++i) {
					hmac.processChunk((unsigned char*)c->PieceNameOffsets[i], std::strlen(c->PieceNameOffsets[i]));
				}
			}
		}
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::HashGamingState(HmacSha256Calculator& hmac)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	GameingState* g = taPtr->GameingState_Ptr;

	hmac.processChunk((unsigned char*)&g->surfaceMetal, (char*)&g->schemaInfo[0] - (char*)&g->surfaceMetal);
	hmac.processChunk((unsigned char*)&taPtr->MinWindSpeed, (char*)&taPtr->data16 - (char*)&taPtr->MinWindSpeed);
	hmac.processChunk((unsigned char*)&taPtr->SeaLevel, 1);
	hmac.processChunk((unsigned char*)&taPtr->mapDebugMode, 1);
	hmac.processChunk((unsigned char*)&taPtr->WindSpeedHardLimit, 4);
	unsigned short losType = taPtr->LosType & LOS_TYPE_MASK;
	hmac.processChunk((unsigned char*)&losType, 2);
	hmac.processChunk((unsigned char*)&taPtr->PlayerUnitsNumber_Skim, 2);
	hmac.processChunk((unsigned char*)&taPtr->MaxUnitNumberPerPlayer, 2);
	unsigned short softwareDebugMode = taPtr->SoftwareDebugMode & SOFTWARE_DEBUG_MODE_MASK;
	hmac.processChunk((unsigned char*)&softwareDebugMode, 2);
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::HashMapSnapshot(HmacSha256Calculator& hmac)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	const int N = taPtr->FeatureMapSizeX * taPtr->FeatureMapSizeY;
	for (int n = 0; n < N; ++n) {
		FeatureStruct* f = &m_featureMapSnapshot.get()[n];
		hmac.processChunk((unsigned char*)&f->height, (char*)&f->field_0c - (char*)&f->height);
	}
}
#pragma code_seg(pop)


#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::Blit(LPVOID lpSurfaceMem, int dwWidth, int dwHeight, int lPitch)
{
	if (lpSurfaceMem == NULL || DataShare->TAProgress != TAInGame) {
		return;
	}

	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;

	OFFSCREEN OffScreen;
	memset(&OffScreen, 0, sizeof(OFFSCREEN));
	OffScreen.Height = dwHeight;
	OffScreen.Width = lPitch;
	OffScreen.lPitch = lPitch;
	OffScreen.lpSurface = lpSurfaceMem;

	OffScreen.ScreenRect.left = 0;
	OffScreen.ScreenRect.right = dwWidth;

	OffScreen.ScreenRect.top = 0;
	OffScreen.ScreenRect.bottom = dwHeight;

	Blit(&OffScreen);
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::Blit(OFFSCREEN* offscreen)
{
	TAProgramStruct* programPtr = *(TAProgramStruct**)0x0051fbd0;
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;

	if (taPtr->GameTime > 2 * 60 * 30) {	// 2 mins
		return;
	}

	programPtr->fontHandle = (unsigned char*)taPtr->COMIXFontHandle;
	programPtr->fontFrontColour = taPtr->desktopGUI.RadarObjecColor[15];
	programPtr->fontBackColour = programPtr->fontAlpha;
	int fontHeight = programPtr->fontHandle[0];

	for (int n = 0u; n < int(m_persistentCheatWarnings.size()); ++n) {
		// bottom of map
		int yOff = offscreen->Height - fontHeight * (n - 1) - 64;
		if (taPtr->SoftwareDebugMode & softwaredebugmode::Clock) {
			yOff -= fontHeight;
		}

		std::string msg = m_persistentCheatWarnings[n];
		DrawTextInScreen(offscreen, const_cast<char*>(msg.c_str()), 129, yOff, -1);
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::ClearPersistentMessages(void)
{
	m_persistentCheatWarnings.clear();
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
std::vector<std::string> ChallengeResponse::GetModulePaths()
{
	std::vector<std::string> results;

	DWORD processId = GetCurrentProcessId();
	HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
	if (processHandle != NULL) {
		HMODULE modules[1024];
		DWORD needed;
		if (EnumProcessModules(processHandle, modules, sizeof(modules), &needed)) {
			TCHAR processPath[MAX_PATH];
			if (GetModuleFileNameEx(processHandle, nullptr, processPath, MAX_PATH)) {
				std::string currentProcessPath(processPath);
				std::string currentProcessDirectory = currentProcessPath.substr(0, currentProcessPath.find_last_of("\\"));
				for (DWORD i = 0; i < (needed / sizeof(HMODULE)); i++) {
					TCHAR modulePath[MAX_PATH];
					if (GetModuleFileNameEx(processHandle, modules[i], modulePath, MAX_PATH)) {
						std::string moduleDirectory = std::string(modulePath).substr(0, std::string(modulePath).find_last_of("\\"));
						if (toLowerCase(currentProcessDirectory) == toLowerCase(moduleDirectory)) {
							results.push_back(modulePath);
						}
					}
				}
			}
		}
		CloseHandle(processHandle);
	}

	return results;
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::SnapshotFile(const char* moduleFileName)
{
	std::ifstream file(moduleFileName, std::ios::binary);
	if (!file.is_open()) {
		return;
	}

	file.seekg(0, std::ios::end);
	std::streamsize fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	std::shared_ptr<std::vector<unsigned char> > moduleOnDisk(new std::vector<unsigned char>(fileSize));
	if (!file.read(reinterpret_cast<char*>(moduleOnDisk->data()), fileSize)) {
		return;
	}

	m_moduleInitialDiskSnapshots.push_back(moduleOnDisk);
	m_moduleInitialDiskSnapshotFilenames.push_back(moduleFileName);
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::LogWeapons(const std::string& filename)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	std::ofstream fs(filename, std::ios::app);
	fs << "========== Weapons:\n";
	fs << "n,ID,name,damage,AEO,edgeeffectiveness,range,data5,coverage,weapontypemask\n";
	std::vector<bool> usedWeapons = getUsedWeaponIds();
	for (int n = 0; n < 256; ++n) {
		WeaponStruct* w = &taPtr->Weapons[n];
		if (w->WeaponName[0] != '\0' && usedWeapons[n]) {
			fs << n << ',' << int(w->ID) << ',' << w->WeaponName << ',' << w->Damage << ',' << w->AOE << ',' << w->EdgeEffectivnes << ',' 
				<< w->Range << ',' << w->data5 << ',' << w->coverage << ',' << w->WeaponTypeMask << '\n';
		}
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::LogFeatures(const std::string& filename)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	std::ofstream fs(filename, std::ios::app);
	fs << "========== Features:\n";
	fs << "n,name,footx,footz,burnweapon,sparktime,damage,energy,metal,height,featuremask\n";
	for (int n = 0; n < taPtr->NumFeatureDefs; ++n) {
		FeatureDefStruct* f = &taPtr->FeatureDef[n];
		fs << n << ',' << f->Name << ',' << f->FootprintX << ',' << f->FootprintZ << ',';
		if (f->BurnWeapon)
			fs << int(f->BurnWeapon->ID) << ',';
		else
			fs << "NULL,";
		fs << f->SparkTime << ',' << f->Damage << ',' << f->Energy << ',' << f->Metal << ',' << int(f->Height) << ',' << (f->FeatureMask & FEATURE_MASK) << '\n';
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::LogUnits(const std::string &filename)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	std::ofstream fs(filename, std::ios::app);
	fs << "========== Units:\n";
	fs << "n,ID,name,buildlimit,footx,footy,_xwidth,xwidth,"
		"data7,ywidith,_ywidth,data8,_zwidth,zwidth,data9,data10,"
		"data11,data12,data13,buildcostenergy,buildcostmetal,maxvelocity,data15,data16,"
		"acceleration,bankscale,pitchscale,damagemodifier,moverate1,moverate2,movementclass,turnrate,"
		"corpse,maxwaterdepth,minwaterdepth,energymake,energyuse,metalmake,extractsmetal,windgenerator,"
		"tidalgenerator,cloakcost,cloakcostmoving,energystorage,metalstorage,buildtime,"
		"yardmap,"
		"weapon1name,weapon1id,"
		"weapon2name,weapon2id,"
		"weapon3name,weapon3id,"
		"maxhp,data8,workertime,healtime,sightdistance,radardistance,sonardistance,mincloakdistance,"
		"radardistancejam,sonardistancejam,nbuilddistance,builddistance,nmaneuverleashlength,attackrunlength,kamikazedistance,sortbias,"
		"cruisealt,data4,maxslope,badslope,transportsize,transportcapacity,waterline,makesmetal,"
		"bmcode,mask0,mask1,"
		"cobcrc32\n";

	for (unsigned n = 0u; n < taPtr->UNITINFOCount; ++n) {
		UnitDefStruct* u = &taPtr->UnitDef[n];
		int buildLimit = u->buildLimit == -1 ? taPtr->PlayerUnitsNumber_Skim : u->buildLimit;
		fs << n << ',' << u->UnitTypeID << ',' << u->Name << ',' << buildLimit << ',' << u->FootX << ',' << u->FootY << ',' << u->__X_Width << ',' << u->X_Width << ','
			<< u->data_7 << ',' << u->Y_Width << ',' << u->__Y_Width << ',' << u->data8 << ',' << u->__Z_Width << ',' << u->Z_Width << ',' << u->data_9 << ',' << u->data_10 << ','
			<< u->data_11 << ',' << u->data_12 << ',' << u->data_13 << ',' << u->buildcostenergy << ',' << u->buildcostmetal << ',' << u->lRawSpeed_maxvelocity << ',' << u->data_15 << ',' << u->data_16 << ','
			<< u->cceleration << ',' << u->bankscale << ',' << u->pitchscale << ',' << u->damagemodifier << ',' << u->moverate1 << ',' << u->moverate2 << ',' << u->movementclass << ',' << u->turnrate << ','
			<< u->corpse << ',' << u->maxwaterdepth << ',' << u->minwaterdepth << ',' << u->energymake << ',' << u->energyuse << ',' << u->metalmake << ',' << u->extractsmetal << ',' << u->windgenerator << ','
			<< u->tidalgenerator << ',' << u->cloakcost << ',' << u->cloakcostmoving << ',' << u->energystorage << ',' << u->metalstorage << ',' << u->buildtime << ',';

		if (u->YardMap) {
			for (int i = 0; i < u->FootX * u->FootY; ++i) {
				fs << int(u->YardMap[i]);
				if (i + 1 < u->FootX * u->FootY) {
					fs << ' ';
				}
			}
			fs << ',';
		}
		else {
			fs << "NULL,";
		}

		if (u->weapon1 && u->weapon1->WeaponName)
			fs << u->weapon1->WeaponName << ',' << int(u->weapon1->ID) << ',';
		else
			fs << "NULL,NULL,";

		if (u->weapon2 && u->weapon2->WeaponName)
			fs << u->weapon2->WeaponName << ',' << int(u->weapon2->ID) << ',';
		else
			fs << "NULL,NULL,";

		if (u->weapon3 && u->weapon3->WeaponName)
			fs << u->weapon3->WeaponName << ',' << int(u->weapon3->ID) << ',';
		else
			fs << "NULL,NULL,";
		fs
			<< u->nMaxHP << ',' << u->data8 << ',' << u->nWorkerTime << ',' << u->nHealTime << ',' << u->nSightDistance << ',' << u->nRadarDistance << ',' << u->nSonarDistance << ',' << u->mincloakdistance << ','
			<< u->radardistancejam << ',' << u->sonardistancejam << ',' << u->nBuildDistance << ',' << u->builddistance << ',' << u->nManeuverLeashLength << ',' << u->attackrunlength << ',' << u->kamikazedistance << ',' << u->sortbias << ','
			<< int(u->cruisealt) << ',' << int(u->data4) << ',' << int(u->maxslope) << ',' << int(u->badslope) << ',' << int(u->transportsize) << ',' << int(u->transportcapacity) << ',' << int(u->waterline) << ',' << u->makesmetal << ','
			<< int(u->bmcode) << ',' << u->UnitTypeMask_0 << ',' << u->UnitTypeMask_1 << ',';

		if (u->cobDataPtr) {
			CobHeader* c = u->cobDataPtr;
			unsigned crc = -1;
			m_crc.PartialCRC(&crc, (unsigned char*)&c->Version, sizeof(c->Version));
			m_crc.PartialCRC(&crc, (unsigned char*)&c->Version, sizeof(c->Version));
			m_crc.PartialCRC(&crc, (unsigned char*)&c->StaticVariablesCount, sizeof(c->StaticVariablesCount));
			m_crc.PartialCRC(&crc, c->ByteCodeStart, c->BytecodeLength);
			for (int i = 0; i < c->MethodCount; ++i) {
				m_crc.PartialCRC(&crc, (unsigned char*)&c->MethodEntryPoints[i], 4);
				m_crc.PartialCRC(&crc, (unsigned char*)c->MethodNameOffsets[i], std::strlen(c->MethodNameOffsets[i]));
			}
			for (int i = 0; i < c->PieceCount; ++i) {
				m_crc.PartialCRC(&crc, (unsigned char*)c->PieceNameOffsets[i], std::strlen(c->PieceNameOffsets[i]));
			}
			crc ^= -1;
			fs << crc;
		}
		fs << '\n';
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::LogGamingState(const std::string& filename)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	GameingState* g = taPtr->GameingState_Ptr;

	std::ofstream fs(filename, std::ios::app);
	fs << "========== GamingState:\n";
	fs << "surfacemetal,minwind_gs,maxwind_gs,gravity_gs,tidal_gs,islava,"
		<< "minwind,maxwind,gravity,tidal,"
		<< "sealevel,mapdebugmode,"
		<< "windspeedlimit,lostype,playerunitsnumber_skim,maxunitnumberperplayer,softwaredebugmode\n";

	fs << g->surfaceMetal << ',' << g->minWindSpeed << ',' << g->maxWindSpeed << ',' << g->gravity << ',' << g->tidalStrength << ',' << g->isLavaMap << ','
		<< taPtr->MinWindSpeed << ',' << taPtr->MaxWindSpeed << ',' << taPtr->Gravity << ',' << taPtr->TidalStrength << ','
		<< int(taPtr->SeaLevel) << ',' << int(taPtr->mapDebugMode) << ','
		<< taPtr->WindSpeedHardLimit << ',' << (taPtr->LosType & LOS_TYPE_MASK) << ',' << taPtr->PlayerUnitsNumber_Skim << ',' << taPtr->MaxUnitNumberPerPlayer << ',' << (taPtr->SoftwareDebugMode & SOFTWARE_DEBUG_MODE_MASK) << '\n';
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::LogMapSnapshot(const std::string& filename)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	std::ofstream fs(filename, std::ios::app);
	fs << "========== Map:\n";
	fs << "n,x,y,height,metal,feature,fddy,fddx\n";
	const int N = taPtr->FeatureMapSizeX * taPtr->FeatureMapSizeY;
	for (int n = 0; n < N; ++n) {
		FeatureStruct* f = &m_featureMapSnapshot.get()[n];
		int x = n % taPtr->FeatureMapSizeX;
		int y = n / taPtr->FeatureMapSizeX;
		fs << n << ',' << x << ',' << y << ',' <<
			int(f->height) << ',' << int(f->MetalValue) << ',' << f->FeatureDefIndex << ',' << int(f->FeatureDefDx) << ',' << int(f->FeatureDefDy) << '\n';
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::LogAll(const std::string& filename)
{
	LogWeapons(filename);
	LogFeatures(filename);
	LogUnits(filename);
	LogGamingState(filename);
	LogGameFileLookup(filename);
	//LogMapSnapshot(filename);
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
std::vector<bool> ChallengeResponse::getUsedWeaponIds()
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	std::vector<bool> isUsed(256, false);

	for (unsigned n = 0u; n < taPtr->UNITINFOCount; ++n) {
		const UnitDefStruct* u = &taPtr->UnitDef[n];
		if (u->weapon1) isUsed[u->weapon1->ID] = true;
		if (u->weapon2) isUsed[u->weapon2->ID] = true;
		if (u->weapon3) isUsed[u->weapon2->ID] = true;
	}
	for (int n = 0; n < taPtr->NumFeatureDefs; ++n) {
		const FeatureDefStruct* f = &taPtr->FeatureDef[n];
		if (f->BurnWeapon) isUsed[f->BurnWeapon->ID] = true;
	}

	return isUsed;
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::handleBattleroomReportCommand(const std::string& str, ChallengeResponseCommand cmd)
{
	ChatText(str.c_str());

	char buffer[2][65] = { 0 };	// two 0x05 "chat" messages back-to-back

	// 1st: "chat" message
	buffer[0][0] = 0x05;
	std::memcpy(&buffer[0][1], str.c_str(), str.size());

	// 2nd: remote request message
	ChallengeResponseMessage* msg = (ChallengeResponseMessage*)&buffer[1][0];
	InitChallengeResponseMessage(*msg, cmd);

	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	unsigned fromDpid = taPtr->Players[taPtr->LocalHumanPlayer_PlayerID].DirectPlayID;
	HAPI_BroadcastMessage(fromDpid, &buffer[0][0], sizeof(buffer));
}

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::OnBattleroomCommandExeReport(const std::vector<std::string>&)
{
	auto crcAndFilename = ChallengeResponse::GetInstance()->GetTotalACrc();
	std::string str = ChallengeResponse::GetReportString(crcAndFilename, NULL);
	handleBattleroomReportCommand(str, ChallengeResponseCommand::ExeVersionRequest);
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::OnBattleroomCommandTdReport(const std::vector<std::string>&)
{
	std::string versionString = ChallengeResponse::GetInstance()->GetTDrawVersionString();
	auto crcAndFilename = ChallengeResponse::GetInstance()->GetTDrawCrc();
	std::string str = ChallengeResponse::GetReportString(crcAndFilename, &versionString);
	handleBattleroomReportCommand(str, ChallengeResponseCommand::TDrawVersionRequest);
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::OnBattleroomCommandTpReport(const std::vector<std::string>&)
{
	auto crcAndFilename = ChallengeResponse::GetInstance()->GetTPlayXCrc();
	std::string str = ChallengeResponse::GetReportString(crcAndFilename, NULL);
	handleBattleroomReportCommand(str, ChallengeResponseCommand::TPlayVersionRequest);
}
#pragma code_seg(pop)


#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::OnBattleroomCommandGp3Report(const std::vector<std::string>&)
{
	auto crcAndFilename = ChallengeResponse::GetInstance()->GetGp3Crc();
	std::string str = ChallengeResponse::GetReportString(crcAndFilename, NULL);
	handleBattleroomReportCommand(str, ChallengeResponseCommand::Gp3VersionRequest);
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::OnBattleroomCommandCrcReport(const std::vector<std::string>&)
{
	std::string str = ChallengeResponse::GetInstance()->GetAllReportString();
	handleBattleroomReportCommand(str, ChallengeResponseCommand::AllVersionRequest);
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
std::string ChallengeResponse::GetTDrawVersionString()
{
	std::string tDrawVersion("<unknown>");
	{
		HRSRC hResInfo = FindResource(HInstance, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
		if (hResInfo == NULL)
			IDDrawSurface::OutptTxt("hResInfo NULL");

		HGLOBAL hResData = LoadResource(HInstance, hResInfo);
		if (hResData == NULL)
			IDDrawSurface::OutptTxt("hResData NULL");

		LPVOID lpResData = LockResource(hResData);
		if (lpResData == NULL)
			IDDrawSurface::OutptTxt("lpResData NULL");

		VS_FIXEDFILEINFO* pFileInfo = nullptr;
		UINT dwFileInfoSize = 0;
		if (VerQueryValue(lpResData, "\\", reinterpret_cast<LPVOID*>(&pFileInfo), &dwFileInfoSize) &&
			pFileInfo != nullptr &&
			pFileInfo->dwSignature == 0xfeef04bd)
		{
			tDrawVersion =
				std::to_string((pFileInfo->dwFileVersionMS >> 16) % 2000) +
				"." + std::to_string(pFileInfo->dwFileVersionMS & 0xffff) +
				"." + std::to_string(pFileInfo->dwFileVersionLS >> 16);
		}

		FreeResource(hResData);
	}

	return tDrawVersion;
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
std::pair<unsigned, std::string> ChallengeResponse::GetTotalACrc()
{
	unsigned crc = 0;
	std::string filename;
	for (int n = 0; n < m_moduleInitialDiskSnapshotFilenames.size(); ++n) {
		auto components = getFilenameComponents(m_moduleInitialDiskSnapshotFilenames[n]);
		if (toLowerCase(std::get<2>(components)) == "exe") {
			filename = std::get<1>(components) + "." + std::get<2>(components);
			crc = m_crc.FullCRC(m_moduleInitialDiskSnapshots[n]->data(), m_moduleInitialDiskSnapshots[n]->size());
			break;
		}
	}

	return std::make_pair(crc, filename);
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
std::pair<unsigned, std::string> ChallengeResponse::GetTDrawCrc()
{
	const std::string DDrawOutermostWrapper((const char*)0x4ff618);
	unsigned crc = 0;
	std::string filename;
	//for (int n = 0; n < m_moduleInitialDiskSnapshotFilenames.size(); ++n) {
	//	auto components = getFilenameComponents(m_moduleInitialDiskSnapshotFilenames[n]);
	//	if (toLowerCase(std::get<1>(components) + "." + std::get<2>(components)) == toLowerCase(DDrawOutermostWrapper)) {
	//		filename = std::get<1>(components) + "." + std::get<2>(components);
	//		crc = m_crc.FullCRC(m_moduleInitialDiskSnapshots[n]->data(), m_moduleInitialDiskSnapshots[n]->size());
	//		break;
	//	}
	//}
	if (m_moduleInitialDiskSnapshotFilenames.size() >= 1) {
		auto components = getFilenameComponents(m_moduleInitialDiskSnapshotFilenames[1]);
		filename = std::get<1>(components) + "." + std::get<2>(components);
		crc = m_crc.FullCRC(m_moduleInitialDiskSnapshots[1]->data(), m_moduleInitialDiskSnapshots[1]->size());
	}
	return std::make_pair(crc, filename);
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
std::pair<unsigned, std::string> ChallengeResponse::GetTPlayXCrc()
{
	const std::string DPlayXOutermostWrapper((const char*)0x4ff9e4);
	unsigned crc = 0;
	std::string filename;
	for (int n = 0; n < m_moduleInitialDiskSnapshotFilenames.size(); ++n) {
		auto components = getFilenameComponents(m_moduleInitialDiskSnapshotFilenames[n]);
		if (toLowerCase(std::get<1>(components) + "." + std::get<2>(components)) == toLowerCase(DPlayXOutermostWrapper)) {
			filename = std::get<1>(components) + "." + std::get<2>(components);
			crc = m_crc.FullCRC(m_moduleInitialDiskSnapshots[n]->data(), m_moduleInitialDiskSnapshots[n]->size());
			break;
		}
	}
	return std::make_pair(crc, filename);
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
std::pair<unsigned, std::string> ChallengeResponse::GetGp3Crc()
{
	unsigned crc = 0;
	std::string filename;
	{
		char buf[MAX_PATH];
		const char* format = (char*)0x5028cc;
		const char* version = (char*)0x5028d8;
		std::sprintf(buf, format, version);
		filename = buf;

		std::ifstream file(buf, std::ios::binary);
		if (file.is_open()) {
			file.seekg(0, std::ios::end);
			std::streamsize fileSize = file.tellg();
			file.seekg(0, std::ios::beg);

			std::vector<unsigned char> bytes(fileSize);
			if (file.read(reinterpret_cast<char*>(bytes.data()), fileSize)) {
				crc = m_crc.FullCRC(bytes.data(), fileSize);
			}
		}
	}
	return std::make_pair(crc, filename);
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
std::string ChallengeResponse::GetReportString(const std::pair<unsigned, std::string>& crcAndFilename, const std::string* optionalVersion)
{
	const unsigned crc = std::get<0>(crcAndFilename);
	const std::string& filename = std::get<1>(crcAndFilename);

	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	char buffer[100];
	if (optionalVersion) {
		std::sprintf(buffer, "*** %s uses %s %s CRC=%X",
			taPtr->Players[taPtr->LocalHumanPlayer_PlayerID].Name, filename.c_str(), optionalVersion->c_str(), crc);
	}
	else {
		std::sprintf(buffer, "*** %s uses %s CRC=%X",
			taPtr->Players[taPtr->LocalHumanPlayer_PlayerID].Name, filename.c_str(), crc);
	}

	std::string result(buffer);
	if (result.size() > 63) {
		result = result.substr(0, 63);
	}
	return result;
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
std::string ChallengeResponse::GetAllReportString()
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	const char* playerName = taPtr->Players[taPtr->LocalHumanPlayer_PlayerID].Name;

	std::string tdrawVersionString = GetTDrawVersionString();
	auto totalACrcAndFn = GetTotalACrc();
	auto tdrawCrcAndFn = GetTDrawCrc();
	auto tplayxCrcAndFn = GetTPlayXCrc();
	auto gp3CrcAndFn = GetGp3Crc();

	const int nDigits = 4;
	char totalACrcStr[nDigits + 1], tdrawCrcStr[nDigits + 1], tplayxCrcStr[nDigits + 1], gp3CrcStr[nDigits + 1];
	encodeInteger(std::get<0>(totalACrcAndFn), totalACrcStr, nDigits);
	encodeInteger(std::get<0>(tdrawCrcAndFn), tdrawCrcStr, nDigits);
	encodeInteger(std::get<0>(tplayxCrcAndFn), tplayxCrcStr, nDigits);
	encodeInteger(std::get<0>(gp3CrcAndFn), gp3CrcStr, nDigits);

	char buffer[100];
	std::sprintf(buffer, "%6.6s %s %5.5s %s %s %6.6s %s gp3 %s %s",
		std::get<1>(totalACrcAndFn).c_str(), totalACrcStr,
		std::get<1>(tdrawCrcAndFn).c_str(), tdrawVersionString.c_str(), tdrawCrcStr,
		std::get<1>(tplayxCrcAndFn).c_str(), tplayxCrcStr,
		gp3CrcStr,
		playerName);

	std::string result(buffer);
	if (result.size() > 63) {
		result = result.substr(0, 63);
	}
	return result;
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_NEXT)))
void ChallengeResponse::LogGameFileLookup(const std::string & filename)
{
	std::ofstream fs(filename, std::ios::app);
	fs << "========== GameFilesLookup:\n";

	for (auto p : gamePathToHpiLookup) {
		if (endsWith(p.second, ".gp3") || endsWith(p.second, ".ccx") || endsWith(p.second, ".hpi") || endsWith(p.second, ".ufo")) {
			auto components = getFilenameComponents(p.second);
			fs << p.first << " -> " << std::get<1>(components) << '.' << std::get<2>(components) << '\n';
		}
		else {
			fs << p.first << " -> " << p.second << '\n';
		}
	}
}
#pragma code_seg(pop)
