#include "ChallengeResponse.h"
#include "hook/hook.h"
#include "iddrawsurface.h"
#include "nswfl_crc32.h"
#include "random_code_seg_keys.h"
#include "tafunctions.h"

#include <fstream>
#include <sstream>

#include <psapi.h>

#define CONCAT_HELPER(x, y) x ## y
#define CONCAT(x, y) CONCAT_HELPER(x, y)
#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)

std::unique_ptr<ChallengeResponse> ChallengeResponse::m_instance = NULL;

#pragma pack(1)
struct ChallengeResponseMessage {
	char code;		// 0x2b
	short size;		// sizeof(ChallengeResponseMessage)
	char command;	// 1: request, 2: reply
	unsigned data[2];// for requests, data[0] is the nonce; for replys data are the crcs
};
#pragma pack()

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

unsigned int ChallengeResponseUpdateAddr = 0x4954b7;
#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_1)))
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
		ChallengeResponseMessage msg = { 0 };
		msg.code = 0x2b;
		msg.size = sizeof(msg);
		msg.command = 1;

		for (int n = 0; n < 10; ++n) {
			PlayerStruct* p = &taPtr->Players[n];
			if (p->PlayerActive && p->DirectPlayID != 0 && p->My_PlayerType == Player_RemoteHuman && !(p->PlayerInfo->PropertyMask & WATCH)) {
				unsigned nonse = cr->NewNonse();
				msg.data[0] = nonse;
				cr->InitPlayerResponse(p->DirectPlayID, nonse);

				char buffer[65] = { 0 };
				buffer[0] = 0x05; // chat
				std::memcpy(buffer + 2, &msg, sizeof(msg));
				HAPI_SendBuf(
					me->DirectPlayID, p->DirectPlayID,
					buffer, sizeof(buffer));
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
#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_2)))
int __stdcall ReceiveChallengeOrResponseProc(PInlineX86StackBuffer X86StrackBuffer)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	PlayerStruct* me = &taPtr->Players[taPtr->LocalHumanPlayer_PlayerID];
	if (!me->PlayerActive || me->DirectPlayID == 0 || (me->PlayerInfo->PropertyMask & WATCH) || DataShare->PlayingDemo) {
		return 0;
	}

	char* buffer = (char*)taPtr->PacketBuffer_p;
	if (buffer[0] == 0x05 && buffer[1] == 0x00 && buffer[2] == 0x2b) {
		ChallengeResponseMessage msg;
		std::memcpy(&msg, (char*)(taPtr->PacketBuffer_p + 2), sizeof(msg));

		if (msg.command == 1) {
			msg.command = 2;
			ChallengeResponse::GetInstance()->ComputeChallengeResponse(msg.data[0], msg.data);

			char buffer[65] = { 0 };
			buffer[0] = 0x05; // chat
			std::memcpy(buffer + 2, &msg, sizeof(msg));

			unsigned fromDpid = taPtr->Players[taPtr->LocalHumanPlayer_PlayerID].DirectPlayID;
			unsigned replyDpid = taPtr->HAPI_net_data0;
			HAPI_SendBuf(fromDpid, replyDpid, buffer, sizeof(buffer));
		}

		else if (msg.command == 2) {
			ChallengeResponse::GetInstance()->SetPlayerResponse(taPtr->HAPI_net_data0, msg.data[0], msg.data[1]);
		}
	}
	return 0;
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_3)))
ChallengeResponse::ChallengeResponse():
	m_rng(std::random_device()())
{
#ifndef NDEBUG
	std::ofstream fs("ErrorLog.txt");	// clear the errorlog
#endif

	m_crc.Initialize();
	m_hooks.push_back(std::make_unique<InlineSingleHook>(ChallengeResponseUpdateAddr, 5, INLINE_5BYTESLAGGERJMP, ChallengeResponseUpdateProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(ReceiveChallengeOrResponseAddr, 5, INLINE_5BYTESLAGGERJMP, ReceiveChallengeOrResponseProc));

	SnapshotModules();
}
#pragma code_seg(pop)


#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_4)))
ChallengeResponse *ChallengeResponse::GetInstance()
{
	if (m_instance == NULL) {
		m_instance.reset(new ChallengeResponse());
	}
	return m_instance.get();
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_5)))
void ChallengeResponse::SnapshotFeatureMap()
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	const int N = taPtr->FeatureMapSizeX * taPtr->FeatureMapSizeY;
	std::shared_ptr<FeatureStruct[]> array(new FeatureStruct[N]);

	std::memcpy(array.get(), taPtr->FeatureMap, N * sizeof(FeatureStruct));

	m_featureMapSnapshot = array;
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_6)))
void ChallengeResponse::SnapshotModules()
{
	HMODULE hModule = NULL;

	// this dll
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		reinterpret_cast<LPCSTR>(&ChallengeResponseUpdateProc), &hModule);
	SnapshotModule(hModule);

	// this process
	hModule = GetModuleHandle(NULL);
	SnapshotModule(hModule);

	for (const std::string& path : GetModules()) {
		if (endsWith(toLowerCase(path), "playx.dll")) {
			SnapshotFile(path.c_str());
		}
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_7)))
void ChallengeResponse::SnapshotModule(HMODULE hModule)
{
	CHAR moduleFileName[MAX_PATH];
	GetModuleFileNameEx(GetCurrentProcess(), hModule, moduleFileName, MAX_PATH);
	SnapshotFile(moduleFileName);
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_8)))
unsigned ChallengeResponse::NewNonse()
{
	return m_rng();
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_9)))
void ChallengeResponse::ComputeChallengeResponse(unsigned nonse, unsigned crcs[2])
{
	crcs[0] = crcs[1] = RANDOM_CODE_SEG_0;
	m_crc.PartialCRC(&crcs[0], (unsigned char*)&nonse, sizeof(nonse));
	m_crc.PartialCRC(&crcs[1], (unsigned char*)&nonse, sizeof(nonse));

	CrcModules(&crcs[0]);
	CrcWeapons(&crcs[1]);
	CrcFeatures(&crcs[1]);
	CrcUnits(&crcs[1]);
	CrcGamingState(&crcs[1]);
	CrcMapSnapshot(&crcs[1]);

	for (int i = 0; i < 2; ++i) {
		crcs[i] = std::max(crcs[i], 1u);
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_10)))
void ChallengeResponse::InitPlayerResponse(unsigned dpid, unsigned nonse)
{
	m_responses[dpid] = std::make_tuple(nonse, 0u, 0u);
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_11)))
void ChallengeResponse::SetPlayerResponse(unsigned dpid, unsigned crc1, unsigned crc2)
{
	std::get<1>(m_responses[dpid]) = crc1;
	std::get<2>(m_responses[dpid]) = crc2;
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_12)))
void ChallengeResponse::VerifyResponses()
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	PlayerStruct* localPlayer = &taPtr->Players[taPtr->LocalHumanPlayer_PlayerID];

	for (auto it = m_responses.begin(); it != m_responses.end(); ++it) {
		unsigned dpid = it->first;
		unsigned nonse = std::get<0>(it->second);
		unsigned replyCrc[] = { std::get<1>(it->second), std::get<2>(it->second) };

		unsigned ourCrc[2];
		ComputeChallengeResponse(nonse, ourCrc);

		PlayerStruct* p = FindPlayerByDPID(dpid);
		std::string playerName = p ? p->Name : std::to_string(dpid);

		char msg[65] = { 0 };
		std::ostringstream ss;
		ss << "[VerCheck] ";

		if (replyCrc[0] == 0u) {
			ss << playerName << " did not reply to VerCheck query!";
			SendText(ss.str().c_str(), 0);
			msg[0] = 0x05;	// chat
			std::strncpy(msg + 1, ss.str().c_str(), 64);
			ss << " Their dll lacks version checking (or packet loss/crash)";
			m_persistentCheatWarnings.push_back(ss.str());
		}
		else if (replyCrc[0] == ourCrc[0] && replyCrc[1] == ourCrc[1]) {
		}
		else if (replyCrc[0] != ourCrc[0]) {
			ss << playerName << '/' << localPlayer->Name << " exe or dll mismatch!";
			SendText(ss.str().c_str(), 0);
			msg[0] = 0x05;	// chat
			std::strncpy(msg + 1, ss.str().c_str(), 64);
			ss << " Players use different exe/dll versions";
			m_persistentCheatWarnings.push_back(ss.str());
		}
		else if (replyCrc[1] != ourCrc[1]) {
			ss << playerName << '/' << localPlayer->Name << " game data mismatch!";
			SendText(ss.str().c_str(), 0);
			msg[0] = 0x05;	// chat
			std::strncpy(msg + 1, ss.str().c_str(), 64);
			ss << " Someone is cheating";
			m_persistentCheatWarnings.push_back(ss.str());
			LogAll("ErrorLog.txt");
		}

		if (msg[0] != 0) {
			HAPI_BroadcastMessage(localPlayer->DirectPlayID, msg, sizeof(msg));
		}
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_13)))
void ChallengeResponse::CrcModules(unsigned* crc)
{
	std::size_t M = m_moduleInitialDiskSnapshots.size();
	for (std::size_t m = 0u; m < M; ++m) {
		m_crc.PartialCRC(crc, m_moduleInitialDiskSnapshots[m]->data(), m_moduleInitialDiskSnapshots[m]->size());
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_14)))
void ChallengeResponse::CrcWeapons(unsigned* crc)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	for (int n = 0; n < 256; ++n) {
		WeaponStruct* w = &taPtr->Weapons[n];
		if (w->WeaponName[0] != '\0') {
			m_crc.PartialCRC(crc, (unsigned char*)&w->Damage, 16);
			m_crc.PartialCRC(crc, (unsigned char*)&w->ID, 1);
			m_crc.PartialCRC(crc, (unsigned char*)&w->WeaponTypeMask, 4);
		}
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_15)))
void ChallengeResponse::CrcFeatures(unsigned* crc)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	for (int n = 0; n < taPtr->NumFeatureDefs; ++n) {
		FeatureDefStruct* f = &taPtr->FeatureDef[n];
		m_crc.PartialCRC(crc, (unsigned char*)&f->FootprintX, 2);
		m_crc.PartialCRC(crc, (unsigned char*)&f->FootprintZ, 2);
		if (f->BurnWeapon) m_crc.PartialCRC(crc, &f->BurnWeapon->ID, 1);
		m_crc.PartialCRC(crc, (unsigned char*)&f->SparkTime, 2);
		m_crc.PartialCRC(crc, (unsigned char*)&f->Damage, 2);
		m_crc.PartialCRC(crc, (unsigned char*)&f->Energy, 4);
		m_crc.PartialCRC(crc, (unsigned char*)&f->Metal, 4);
		m_crc.PartialCRC(crc, (unsigned char*)&f->Height, 1);
		short fm = f->FeatureMask & 0x0fff;
		m_crc.PartialCRC(crc, (unsigned char*)&fm, 2);
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_16)))
void ChallengeResponse::CrcUnits(unsigned* crc)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	for (unsigned n = 0u; n < taPtr->UNITINFOCount; ++n) {
		UnitDefStruct* u = &taPtr->UnitDef[n];
		if (u->buildLimit != 0) {
			m_crc.PartialCRC(crc, (unsigned char*)&u->FootX, 4);
			if (u->YardMap) m_crc.PartialCRC(crc, (unsigned char*)u->YardMap, u->FootX * u->FootY);
			m_crc.PartialCRC(crc, (unsigned char*)&u->buildLimit, 4);
			m_crc.PartialCRC(crc, (unsigned char*)&u->__X_Width, (char*)&u->data_14 - (char*)&u->__X_Width);
			m_crc.PartialCRC(crc, (unsigned char*)&u->lRawSpeed_maxvelocity, (char*)&u->weapon1 - (char*)&u->lRawSpeed_maxvelocity);
			if (u->weapon1) m_crc.PartialCRC(crc, (unsigned char*)&u->weapon1->ID, 1);
			if (u->weapon2) m_crc.PartialCRC(crc, (unsigned char*)&u->weapon2->ID, 1);
			if (u->weapon3) m_crc.PartialCRC(crc, (unsigned char*)&u->weapon3->ID, 1);
			if (u->ExplodeAs) m_crc.PartialCRC(crc, (unsigned char*)&u->ExplodeAs->ID, 1);
			if (u->SelfeDestructAs) m_crc.PartialCRC(crc, (unsigned char*)&u->SelfeDestructAs->ID, 1);
			m_crc.PartialCRC(crc, (unsigned char*)&u->nMaxHP, (char*)&u->ExplodeAs - (char*)&u->nMaxHP);
			m_crc.PartialCRC(crc, (unsigned char*)&u->maxslope, (char*)&u->wpri_badTargetCategory_MaskAryPtr - (char*)&u->maxslope);
			m_crc.PartialCRC(crc, (unsigned char*)&u->UnitTypeMask_0, 2 * sizeof(long));
		}
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_17)))
void ChallengeResponse::CrcGamingState(unsigned* crc)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	GameingState* g = taPtr->GameingState_Ptr;

	m_crc.PartialCRC(crc, (unsigned char*)&g->surfaceMetal, (char*)&g->schemaInfo[0] - (char*)&g->surfaceMetal);
	m_crc.PartialCRC(crc, (unsigned char*)&taPtr->MinWindSpeed, (char*)&taPtr->data16 - (char*)&taPtr->MinWindSpeed);
	m_crc.PartialCRC(crc, (unsigned char*)&taPtr->SeaLevel, 1);
	m_crc.PartialCRC(crc, (unsigned char*)&taPtr->mapDebugMode, 1);
	m_crc.PartialCRC(crc, (unsigned char*)&taPtr->WindSpeedHardLimit, 4);
	unsigned short losType = taPtr->LosType & 7;
	m_crc.PartialCRC(crc, (unsigned char*)&losType, 2);
	m_crc.PartialCRC(crc, (unsigned char*)&taPtr->PlayerUnitsNumber_Skim, 2);
	m_crc.PartialCRC(crc, (unsigned char*)&taPtr->MaxUnitNumberPerPlayer, 2);
	m_crc.PartialCRC(crc, (unsigned char*)&taPtr->SoftwareDebugMode, 2);
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_18)))
void ChallengeResponse::CrcMapSnapshot(unsigned* crc)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	const int N = taPtr->FeatureMapSizeX * taPtr->FeatureMapSizeY;
	for (int n = 0; n < N; ++n) {
		FeatureStruct* f = &m_featureMapSnapshot.get()[n];
		m_crc.PartialCRC(crc, (unsigned char*)&f->height, (char*)&f->field_0c - (char*)&f->height);
	}
}
#pragma code_seg(pop)


#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_19)))
void ChallengeResponse::Blit(LPVOID lpSurfaceMem, int dwWidth, int dwHeight, int lPitch)
{
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

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_20)))
void ChallengeResponse::Blit(OFFSCREEN* offscreen)
{
	for (unsigned n = 0u; n < m_persistentCheatWarnings.size(); ++n) {
		// y-coordinate on which the +clock Game Time was drawn
		int yOff = offscreen->Height - 15 * n - 64;
		std::string msg = m_persistentCheatWarnings[n];
		DrawTextInScreen(offscreen, const_cast<char*>(msg.c_str()), 129, yOff, -1);
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_21)))
void ChallengeResponse::ClearPersistentMessages(void)
{
	m_persistentCheatWarnings.clear();
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_22)))
std::vector<std::string> ChallengeResponse::GetModules()
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

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_23)))
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
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_24)))
void ChallengeResponse::LogWeapons(const std::string& filename)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	std::ofstream fs(filename, std::ios::app);
	fs << "========== Weapons:\n";
	fs << "n,ID,name,damage,AEO,edgeeffectiveness,range,data5,coverage,weapontypemask\n";
	for (int n = 0; n < 256; ++n) {
		WeaponStruct* w = &taPtr->Weapons[n];
		if (w->WeaponName[0] != '\0') {
			fs << n << ',' << int(w->ID) << ',' << w->WeaponName << ',' << w->Damage << ',' << w->AOE << ',' << w->EdgeEffectivnes << ',' 
				<< w->Range << ',' << w->data5 << ',' << w->coverage << ',' << w->WeaponTypeMask << '\n';
		}
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_25)))
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
		fs << f->SparkTime << ',' << f->Damage << ',' << f->Energy << ',' << f->Metal << ',' << int(f->Height) << ',' << (f->FeatureMask&0x0fff) << '\n';
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_26)))
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
		"bmcode,mask0,mask1\n";
	for (unsigned n = 0u; n < taPtr->UNITINFOCount; ++n) {
		UnitDefStruct* u = &taPtr->UnitDef[n];
		fs << n << ',' << u->UnitTypeID << ',' << u->Name << ',' << u->buildLimit << ',' << u->FootX << ',' << u->FootY << ',' << u->__X_Width << ',' << u->X_Width << ','
			<< u->data_7 << ',' << u->Y_Width << ',' << u->__Y_Width << ',' << u->data8 << ',' << u->__Z_Width << ',' << u->Z_Width << ',' << u->data_9 << ',' << u->data_10 << ','
			<< u->data_11 << ',' << u->data_12 << ',' << u->data_13 << ',' << u->buildcostenergy << ',' << u->buildcostmetal << ',' << u->lRawSpeed_maxvelocity << ',' << u->data_15 << ',' << u->data_16 << ','
			<< u->cceleration << ',' << u->bankscale << ',' << u->pitchscale << ',' << u->damagemodifier << ',' << u->moverate1 << ',' << u->moverate2 << ',' << u->movementclass << ',' << u->turnrate << ','
			<< u->corpse << ',' << u->maxwaterdepth << ',' << u->minwaterdepth << ',' << u->energymake << ',' << u->energyuse << ',' << u->metalmake << ',' << u->extractsmetal << ',' << u->windgenerator << ','
			<< u->tidalgenerator << ',' << u->cloakcost << ',' << u->cloakcostmoving << ',' << u->energystorage << ',' << u->metalstorage << ',' << u->buildtime << ',';
		if (u->YardMap) {
			fs.write(u->YardMap, u->FootX * u->FootY);
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
			<< int(u->bmcode) << ',' << u->UnitTypeMask_0 << ',' << u->UnitTypeMask_1 << '\n';
	}
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_27)))
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
		<< taPtr->WindSpeedHardLimit << ',' << taPtr->LosType << ',' << taPtr->PlayerUnitsNumber_Skim << ',' << taPtr->MaxUnitNumberPerPlayer << ',' << taPtr->SoftwareDebugMode << '\n';
}
#pragma code_seg(pop)

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_28)))
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

#pragma code_seg(push, CONCAT(".text$", STRINGIFY(RANDOM_CODE_SEG_29)))
void ChallengeResponse::LogAll(const std::string& filename)
{
	LogWeapons(filename);
	LogFeatures(filename);
	LogUnits(filename);
	LogGamingState(filename);
	//LogMapSnapshot(filename);
}
#pragma code_seg(pop)
