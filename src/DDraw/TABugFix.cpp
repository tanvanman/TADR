
#include "iddraw.h"
#include "iddrawsurface.h"
#include "HexDump.h"
#include "TPacket.h"

#include "hook/etc.h"
#include "hook/hook.h"

#include "tamem.h"
#include "tafunctions.h"
#include "TAbugfix.h"
#include "TAConfig.h"

#include "ddraw.h"

#include <chrono>
#include <random>
#include <sstream>

TABugFixing * FixTABug;
///////---------------------
/*
.text:004866E8 078 75 04                                                           jnz     short loc_4866EE
	.text:004866EA 078 33 F6                                                           xor     esi, esi
	.text:004866EC 078 EB 18                                                           jmp     short loc_486706
	-> if it's null, straight jmp to across the routine that used esi as unit ptr
	*/

unsigned int NullUnitDeathVictimAddr= 0x04866E8;
BYTE NullUnitDeathVictimBits[]={0x0F, 0x84, 0x6B, 0x07, 0x00, 0x00};

//.text:00438EDE 03C 0F 8C 69 01 00 00                                               jl      loc_43904D
//->    jle      loc_43904D  Radius most bigger than 0
unsigned int CircleRadiusAddr= 0x00438EDE;
BYTE CircleRadiusBits[]= {0x0F, 0x8E, 0x69, 0x01, 0x00, 0x00};

unsigned int CrackCdAddr= 0x0041D4CD;

BYTE CrackCDBits[]= {0x90, 0x90 , 0x90 , 0x90 , 0x90 , 0x90};
unsigned int CrackCd2Addr= 0x0050289C;
BYTE CrackCD2Bits[]= {0};

unsigned int CrackCd3Addr= 0x41D6B0;
BYTE CrackCD3Bits[]= {0x90, 0x90, 0xB0, 0x2E};

unsigned int LosTypeShouldBeACheatCodeAddr = 0x501df4;  // command level for +lostype handler
BYTE LosTypeShouldBeACheatCodeBits[] = { 2 };           // 1=normal; 2=cheat; 7=debug

unsigned int GUIErrorLengthAry[GUIERRORCOUNT]=
{
	0x04AEBBE,
	0x04AEBCA,
	0x04AEC2C,
	0x04AEC87
};
BYTE GUIErrorLengthBits[]= {0x80};

unsigned int CDMusic_TABAddr= 0x00460E0D;
BYTE CDMusic_TABBits[]= {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};

unsigned int CDMusic_Menu_PauseAddr= 0x00490B30;
unsigned int CDMusic_Victory_PauseAddr= 0x4996DF;


unsigned int CDMusic_StopAddr= 0x4CED4b;
BYTE CDMusic_StopBits[]= {0x44, 0xb6, 0x50};


unsigned int UnitVolumeYequZero_Addr= 0x049CE65;

unsigned int UnitIDOutRangeAddr= 0x48A26B;
unsigned int UnitIDOutRangeRtn= 0x048A270;
unsigned int UnitDeath_BeforeUpdateUIAddr= 0x04995EF;


unsigned int EnterDrawPlayer_MAPPEDMEMAddr= 0x0467440;
unsigned int LeaveDrawPlayer_MAPPEDMEMAddr= 0x0465572;

unsigned int EnterUnitLoopAddr= 0x0464F80;
unsigned int LeaveUnitLoopAddr= 0x046563B;
unsigned int LeaveUnitLoop2Addr= 0x04655F4;
 
unsigned int SavePlayerColorHookAddr = 0x454927;
unsigned int RestorePlayerColorHookAddr = 0x45493c;

unsigned int DisplayModeMinHeight768EnumAddr = 0x45E589;
BYTE DisplayModeMinHeight768EnumBits[] = { 0x00, 0x03 };
unsigned int DisplayModeMinHeight768DefAddr = 0x42FA97;
BYTE DisplayModeMinHeight768DefBits[] = { 0x00, 0x03 };
unsigned int DisplayModeMinHeight768RegAddr = 0x42FA83;

unsigned int DisplayModeMinWidth1024DefAddr = 0x42FA42;
BYTE DisplayModeMinWidth1024DefBits[] = { 0x00, 0x04 };
unsigned int DisplayModeMinWidth1024RegAddr = 0x42FA2E;

unsigned int SinglePlayerStartButtonAddr = 0x456780;
BYTE SinglePlayerStartButtonBits[] = { 0x02, 0x7d };

// dead space at the end of "Copyright 0000 Humongous Entertainment"
// Is 0 unless exe is patched to make it something else
unsigned int EnableClickSnapAddr = 0x50390a;
BYTE EnableClickSnapBits[] = { 3, 5 };	// default radius, maximum radius

// Patch map features to be owned by player_idx = 11 (instead of player_idx = 10 as per original behaviour)
// This enables the DrawPlayer11DT patch to draw the features
unsigned int DrawPlayer11DTEnableAddrs[] = { 0x483a75, 0x483ad3, 0x483b31 };
BYTE DrawPlayer11DTEnableBits[] = { 11 };

// Draw map features (player_idx = 11 if DrawPlayer11DTEnable patch is activated) regardless of LOS
unsigned int DrawPlayer11DTAddr = 0x469a7b;
int __stdcall DrawPlayer11DTProc(PInlineX86StackBuffer X86StrackBuffer)
{
	const UnitStruct* unit = (const UnitStruct*)X86StrackBuffer->Esi;
	if ((X86StrackBuffer->Ecx & 0x0f) == 11)  // change to 10 if you want to draw map features without also requiring DrawPlayer11DTEnable patch
	{
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x469aaf;	// draw feature regardless of LOS
		return X86STRACKBUFFERCHANGE;
	}
	return 0;
}

// the resource strip draws more line that it should
unsigned int ResourceStripHeightFixAddr = 0x469078;
int __stdcall ResourceStripHeightFixProc(PInlineX86StackBuffer X86StrackBuffer)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	short* gaf = (short*)X86StrackBuffer->Eax;
	if (gaf[1] >= taPtr->GameSreen_Rect.top) {
		gaf[1] = taPtr->GameSreen_Rect.top - 1;
	}
	return 0;
}

unsigned int PatrolDisableBuildRepairAddr = 0x4059e4;
int __stdcall PatrolDisableBuildRepairProc(PInlineX86StackBuffer X86StrackBuffer)
{
	const UnitStruct* unit = (const UnitStruct*)X86StrackBuffer->Esi;
	if (((unit->UnitSelected & 0x000c0000) >> 18) == 0 /* holdpos*/)
	{
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x405b18;	// skip check for build/repair, return directly to find-reclaim
		return X86STRACKBUFFERCHANGE;
	}
	return 0;
}

unsigned int PatrolDisableReclaimAddr = 0x405b18;
int __stdcall PatrolDisableReclaimProc(PInlineX86StackBuffer X86StrackBuffer)
{
	const UnitStruct* unit = (const UnitStruct*)X86StrackBuffer->Esi;
	if (((unit->UnitSelected & 0x000c0000) >> 18) == 2 /* roam */)
	{
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x405d4a;	// skip check for reclaim, return early
		return X86STRACKBUFFERCHANGE;
	}
	return 0;
}

unsigned int VTOLPatrolDisableBuildRepairAddr = 0x41547d;
int __stdcall VTOLPatrolDisableBuildRepairProc(PInlineX86StackBuffer X86StrackBuffer)
{
	const UnitStruct* unit = (const UnitStruct*)X86StrackBuffer->Edi;
	if (((unit->UnitSelected & 0x000c0000) >> 18) == 0 /* holdpos */)
	{
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x415621;	// skip check for build/repair, return directly to find-reclaim
		return X86STRACKBUFFERCHANGE;
	}
	return 0;
}

unsigned int VTOLPatrolDisableReclaimAddr = 0x415621;
int __stdcall VTOLPatrolDisableReclaimProc(PInlineX86StackBuffer X86StrackBuffer)
{
	const UnitStruct* unit = (const UnitStruct*)X86StrackBuffer->Edi;
	if (((unit->UnitSelected & 0x000c0000) >> 18) == 2 /* roam */)
	{
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x4157bd;	// skip check for reclaim, return early
		return X86STRACKBUFFERCHANGE;
	}
	return 0;
}

unsigned int JammingOwnRadarAddr = 0x467608;
int __stdcall JammingOwnRadarProc(PInlineX86StackBuffer X86StrackBuffer)
{
	const TAdynmemStruct* ptr = *(TAdynmemStruct**)0x00511de8;
	const UnitStruct* unit = (UnitStruct*)(X86StrackBuffer->Esi - 0x92);
	if (IsPlayerAllyUnit(unit->UnitInGameIndex, ptr->LOS_Sight_PlayerID)) {
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x46765a;
		return X86STRACKBUFFERCHANGE;
	}
	return 0;
}

unsigned int KeepOnReclaimPreparedOrderAddr = 0x4a7132;	// toggle the reclaim order
int __stdcall KeepOnReclaimPreparedOrderProc(PInlineX86StackBuffer X86StrackBuffer)
{
	TAProgramStruct* programPtr = *(TAProgramStruct**)0x0051fbd0;
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	unsigned char* isReclaimOrderActivated = (unsigned char*) X86StrackBuffer->Ebp + 0x138;
	if (taPtr->PrepareOrder_Type == ordertype::BUILD && *isReclaimOrderActivated) {
		// bypass toggle of reclaim order
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x4a715c;
		return X86STRACKBUFFERCHANGE;
	}
	return 0;
}

unsigned int GhostComFixAddr = 0x4553f2;	// start of 0x2c packet handler
int __stdcall GhostComFixProc(PInlineX86StackBuffer X86StrackBuffer)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	unsigned char* data = (unsigned char*)X86StrackBuffer->Edx;
	PlayerStruct* player = (PlayerStruct*)X86StrackBuffer->Edi;

	if (taPtr->GameTime >= taPtr->PlayerUnitsNumber_Skim ||
		!player->PlayerActive ||
		(player->PlayerInfo->PropertyMask & WATCH) ||
		InferredPlayerTypeIsLocal(player))
	{
		return 0;
	}

	// assert the commander's position whenever we receive a movement order for him.
	SerialBitArrayStruct sba;
	sba.data = data;
	sba.dword_ofs = 0u;
	sba.bit_ofs = 0u;
	if (0x2c == SerialBitArrayRead(&sba, 0, 8)) {
		int size = SerialBitArrayRead(&sba, 0, 16);
		int gameTicks = SerialBitArrayRead(&sba, 0, 32);
		int unitIndex = SerialBitArrayRead(&sba, 0, 16);
		if (unitIndex == 0) {	// commander
			int unitDefIndex = SerialBitArrayRead(&sba, 0, taPtr->UNITINFOCount_SignificantBitsCount);
			int bits = SerialBitArrayRead(&sba, 0, 3);
			if (bits & 4) {
				int fromX = SerialBitArrayRead(&sba, 0, 16);
				int fromY = SerialBitArrayRead(&sba, 0, 16);
				int toX = SerialBitArrayRead(&sba, 0, 16);
				int toY = SerialBitArrayRead(&sba, 0, 16);
				player->Units[0].XPos = fromX;
				player->Units[0].YPos = fromY;
			}
		}
	}

	return 0;
}

// main loop, after processing network packets, before updating game state
unsigned int GhostComFixAssistAddr = 0x4954ed;
int __stdcall GhostComFixAssistProc(PInlineX86StackBuffer X86StrackBuffer)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	unsigned char* data = (unsigned char*)X86StrackBuffer->Edx;
	PlayerStruct* player = (PlayerStruct*)X86StrackBuffer->Edi;

	if (taPtr->GameTime == 90) { // 3 secs
		for (int i = 0; i < 10; ++i) {
			PlayerStruct* p = &taPtr->Players[i];
			if (p->PlayerActive &&
				!(p->PlayerInfo->PropertyMask & WATCH) &&
				p->Units[0].IsUnit &&
				(p->My_PlayerType == Player_LocalHuman || p->My_PlayerType == Player_LocalAI))
			{
				// send out a dummy move command to assist GhostComFix
				PacketBuilderStruct pb;
				PacketBuilder_Initialise(&pb, 0);
				PacketBuilder_AppendBits(&pb, 0, 0x2c, 8);	// packet code
				PacketBuilder_AppendBits(&pb, 0, 0, 16);	// placeholder for size
				PacketBuilder_AppendBits(&pb, 0, taPtr->GameTime, 32);
				PacketBuilder_AppendBits(&pb, 0, 0, 16);	// unit index (commander=0)
				PacketBuilder_AppendBits(&pb, 0, p->Units[0].UnitID, taPtr->UNITINFOCount_SignificantBitsCount);
				PacketBuilder_AppendBits(&pb, 0, 4, 3);		// bits

				int fromX = p->Units[0].XPos;
				int fromY = p->Units[0].YPos;
				PacketBuilder_AppendBits(&pb, 0, fromX, 16);
				PacketBuilder_AppendBits(&pb, 0, fromY, 16);

				int toX = fromX, toY = fromY;
				UnitOrdersStruct* uo = p->Units[0].UnitOrders;
				if (uo != NULL && uo->Pos.X > 0 && uo->Pos.Y > 0) {
					// com has already been orderd to move but we continue regardless
					// because the remote might have missed the move command
					toX = uo->Pos.X;
					toY = uo->Pos.Y;
				}

				PacketBuilder_AppendBits(&pb, 0, toX, 16);
				PacketBuilder_AppendBits(&pb, 0, toY, 16);
				PacketBuilder_AppendBits(&pb, 0, 0xffff, 16);

				int size = (pb.bit_count + 7) / 8 + 4 * pb.dword_count;
				PacketBuilder_AssignByteAtOfs(&pb, 0, 1, size);
				PacketBuilder_AssignByteAtOfs(&pb, 0, 2, size >> 8);
				HAPI_BroadcastMessage(p->DirectPlayID, (char*)pb.buffer_ptr, size);
			}
		}
	}
	return 0;
}

// TA natively assigns the first available ID, starting its search from 0.
// Unfortunately if that ID was used by a recently-deceased unit, we may still receive damage packets for it.
// So we're going to additionally require that the ID be available for at least a few seconds before making it available for recycling.
static std::vector<int> unitIdRecycleTimestamps[10];	// The timestamp at which the ID becomes available
static const int RECYCLE_MARGIN_TIME = 5 * 30;			// 5 sec

unsigned int FixFactoryExplosionsInitAddr = 0x4854a0;	// around the time that the UnitStructs are being initialised
int __stdcall FixFactoryExplosionsInitProc(PInlineX86StackBuffer X86StrackBuffer)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	const int nIds = taPtr->PlayerUnitsNumber_Skim;
	for (int i = 0; i < 10; ++i) {
		unitIdRecycleTimestamps[i].resize(nIds, 0);		// all IDs available since t=0
	}
	return 0;
}

unsigned int FixFactoryExplosionsAssignUnitIdAddr = 0x486036;
int __stdcall FixFactoryExplosionsAssignUnitIdProc(PInlineX86StackBuffer X86StrackBuffer)
{
	// Assign next available unit index

	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	int playerIndex = *(int*)(X86StrackBuffer->Esp + 0x14 - 4) / 0x14b;
	int unitIndexRequested = X86StrackBuffer->Edx;

	PlayerStruct* player = &taPtr->Players[playerIndex];
	UnitStruct* units = (UnitStruct*)X86StrackBuffer->Esi;
	if (unitIndexRequested > 0) {
		if (units->UnitID == 0) {
			//IDDrawSurface::OutptTxt("[FixFactoryExplosionsAssignUnitIdProc] player=%d, assignedId(requested)=%d\n", playerIndex, unitIndexRequested);
			X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x48605d;
			return X86STRACKBUFFERCHANGE;
		}
		else {
			// requested unitID is not available
			X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x4861bd;
			return X86STRACKBUFFERCHANGE;
		}
	}

	for (int n = 0; n < taPtr->PlayerUnitsNumber_Skim; ++n) {
		if (0 == player->Units[n].UnitID && taPtr->GameTime >= unitIdRecycleTimestamps[playerIndex][n]) {
			//IDDrawSurface::OutptTxt("[FixFactoryExplosionsAssignUnitIdProc] player=%d, assignedId=%d\n", playerIndex, n);
			X86StrackBuffer->Esi = (DWORD)&player->Units[n];
			X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x48605d;
			return X86STRACKBUFFERCHANGE;
		}
	}

	// no UnitIds available
	X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x486053;
	return X86STRACKBUFFERCHANGE;
}

unsigned int FixFactoryExplosionsRecycleUnitIdAddr = 0x486dc1;
int __stdcall FixFactoryExplosionsRecycleUnitIdProc(PInlineX86StackBuffer X86StrackBuffer)
{
	// recycle a unit index

	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	UnitStruct* unit = (UnitStruct*)(X86StrackBuffer->Esi);
	char* packetData = *(char**)(X86StrackBuffer->Esp + 0x7c);
	int unitInGameIndex = *(unsigned short*)(packetData + 1);
	
	if (!unit) {
		IDDrawSurface::OutptTxt("[FixFactoryExplosionsRecycleUnitIdProc] null unit!\n");
	}
	else if (!unit->Owner_PlayerPtr0) {
		IDDrawSurface::OutptTxt("[FixFactoryExplosionsRecycleUnitIdProc] null unit->Owner_PlayerPtr0!\n");
	}
	else if (unit->UnitInGameIndex != unitInGameIndex) {
		IDDrawSurface::OutptTxt("[FixFactoryExplosionsRecycleUnitIdProc] UnitInGameIndex misatch! packet:%d, unit:%d\n",
			unitInGameIndex, unit->UnitInGameIndex);
	}
	else if (unit->OwnerIndex < 0 || unit->OwnerIndex >= 10) {
		IDDrawSurface::OutptTxt("[FixFactoryExplosionsRecycleUnitIdProc] Invalid unit->OwnerIndex:%d\n", unit->OwnerIndex);
	}
	else {
		int playerUnitIndex = unit->UnitInGameIndex - unit->Owner_PlayerPtr0->UnitsIndex_Begin;
		if (unsigned(playerUnitIndex) >= taPtr->PlayerUnitsNumber_Skim) {
			IDDrawSurface::OutptTxt("[FixFactoryExplosionsRecycleUnitIdProc] Out of range unit->UnitInGameIndex:%d. owner->UnitsIndex_Begin=%d, PlayerUnitsNumber_Skim=%d\n",
				unit->UnitInGameIndex, unit->Owner_PlayerPtr0->UnitsIndex_Begin, taPtr->PlayerUnitsNumber_Skim);
		}
		else {
			unitIdRecycleTimestamps[unit->OwnerIndex][playerUnitIndex] = taPtr->GameTime + RECYCLE_MARGIN_TIME;
			//IDDrawSurface::OutptTxt("[FixFactoryExplosionsRecycleUnitIdProc] player=%d, UnitId=%d, timestampWhenAvailable=%d\n",
			//	int(unit->OwnerIndex), playerUnitIndex, unitIdRecycleTimestamps[unit->OwnerIndex][playerUnitIndex]);
		}
	}
	return 0;
}

unsigned int JunkYardmapFixAddr = 0x42cf5e;
int __stdcall JunkYardmapFixProc(PInlineX86StackBuffer X86StrackBuffer)
{
	const UnitDefStruct* ud = (UnitDefStruct*)X86StrackBuffer->Ebp;
	const bool yardmapDefined = X86StrackBuffer->Eax;
	if (!yardmapDefined || ud->FootX == 0 || ud->FootY == 0) {
		// null yardmap
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x42d073;
		return X86STRACKBUFFERCHANGE;
	}
	return 0;
}

unsigned int PutDeadHostInWatchModeAddr = 0x4656e5;
int __stdcall PutDeadHostInWatchModeProc(PInlineX86StackBuffer X86StrackBuffer)
{
	TAProgramStruct* programPtr = *(TAProgramStruct**)0x0051fbd0;
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	PlayerStruct* host = FindPlayerByPlayerNum(1);
	if (host && host->PlayerAryIndex == taPtr->LocalHumanPlayer_PlayerID) {
		SendText(
			"You're out!  You are placed in watch mode because you're hosting.\n"
			"If you exit, the game may terminate and that would make the others sad,\n"
			"possibly even mad.", 1);
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x465508;
		return X86STRACKBUFFERCHANGE;
	}
	return 0;
}

// Ensure the random wind speed speed is the same for all players by using host's DPID as a random seed
unsigned int WindSpeedSyncAddr = 0x490c5a;
int __stdcall WindSpeedSyncProc(PInlineX86StackBuffer X86StrackBuffer)
{
	TAProgramStruct* programPtr = *(TAProgramStruct**)0x0051fbd0;
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;

	static std::unique_ptr<std::default_random_engine> RNG;
	if (RNG.get() == NULL) {
		PlayerStruct* host = FindPlayerByPlayerNum(1);
		if (host && (host->DirectPlayID > 0)) {
			IDDrawSurface::OutptTxt("[WindSpeedSyncProc] initialsing RNG using host DPID. host=%s, dpid=0x%x",
				host->Name, host->DirectPlayID);
			RNG = std::make_unique<std::default_random_engine>(host->DirectPlayID);
		}
		else {
			unsigned t = std::chrono::system_clock::now().time_since_epoch().count();
			IDDrawSurface::OutptTxt("[WindSpeedSyncProc] initialsing RNG using current time. t=%d", t);
			RNG = std::make_unique<std::default_random_engine>(t);
		}
	}

	taPtr->WindSpeedGameTicksNextUpdate += 30 * (5 + (*RNG)() % 10);
	if (taPtr->MaxWindSpeed <= taPtr->MinWindSpeed) {
		taPtr->WindSpeed = taPtr->MinWindSpeed;
	}
	else {
		taPtr->WindSpeed = (*RNG)() % (taPtr->MaxWindSpeed - taPtr->MinWindSpeed) + taPtr->MinWindSpeed;
	}

	if (taPtr->WindSpeed > 0) {
		taPtr->WindDirection = (*RNG)() % 0x10000;
	}

	X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x490ce8;
	return X86STRACKBUFFERCHANGE;
}

unsigned int NetworkRawReceiveLogAddr = 0x44fa68;
int __stdcall NetworkRawReceiveLogProc(PInlineX86StackBuffer X86StrackBuffer)
{
	TAProgramStruct* programPtr = *(TAProgramStruct**)0x0051fbd0;
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;

	HAPINETStruct* hapinet = *(HAPINETStruct**)(X86StrackBuffer->Esp + 0x118 + 0x04);
	std::uint8_t* buffer = *(std::uint8_t**)(X86StrackBuffer->Esp + 0x118 + 0x08);
	std::uint32_t* bufferSize = *(std::uint32_t**)(X86StrackBuffer->Esp + 0x118 + 0x0c);

	unsigned fromDpid = taPtr->hapinet.fromDpid;
	unsigned toDpid = taPtr->hapinet.toDpid;
	PlayerStruct* fromPlayer = FindPlayerByDPID(fromDpid);
	PlayerStruct* toPlayer = FindPlayerByDPID(toDpid);
	std::string fromName = fromPlayer ? fromPlayer->Name : "<unknown>";
	std::string toName = toPlayer ? toPlayer->Name : "<unknown>";

	std::ostringstream ss;
	ss << "[NetworkRawReceiveLogAddr] from: " << fromName << "(" << std::dec << fromDpid << '/' << std::hex << fromDpid << "), "
		<< "to: " << toName << "(" << std::dec << toDpid << '/' << std::hex << toDpid << ")\n";
	taflib::HexDump(buffer, *bufferSize, ss);
	std::string dump = ss.str();
	IDDrawSurface::OutptRawTxt(dump.c_str(), false);

	return 0;
}

unsigned int NetworkDispatchLogAddr = 0x453db4;
int __stdcall NetworkDispatchLogProc(PInlineX86StackBuffer X86StrackBuffer)
{
	TAProgramStruct* programPtr = *(TAProgramStruct**)0x0051fbd0;
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;

	if (taPtr->GameTime < 60 * 30)
	{
		unsigned fromDpid = taPtr->hapinet.fromDpid;
		unsigned toDpid = taPtr->hapinet.toDpid;
		PlayerStruct* fromPlayer = FindPlayerByDPID(fromDpid);
		PlayerStruct* toPlayer = FindPlayerByDPID(toDpid);
		std::string fromName = fromPlayer ? fromPlayer->Name : "<unknown>";
		std::string toName = toPlayer ? toPlayer->Name : "<unknown>";

		int len = tapacket::TPacket::getExpectedSubPacketSize((std::uint8_t*)taPtr->PacketBuffer_p, taPtr->PacketBufferSize);
		std::ostringstream ss;
		ss << "[NetworkDispatchLogProc] from: " << fromName << "(" << std::dec << fromDpid << '/' << std::hex << fromDpid << "), "
			<< "to: " << toName << "(" << std:: dec << toDpid << '/' << std::hex << toDpid << ")\n";
		taflib::HexDump(taPtr->PacketBuffer_p, len, ss);
		std::string dump = ss.str();
		IDDrawSurface::OutptRawTxt(dump.c_str(), false);
	}
	return 0;
}

#define LOG_TRACE_HOOK(hookAddr, n, fmtstr, regDisplay) \
unsigned int LogTrace##n##Addr = (hookAddr); \
int __stdcall LogTrace##n##Proc(PInlineX86StackBuffer X86StrackBuffer) \
{ \
	IDDrawSurface::OutptTxt("[LogTrace%d] "##fmtstr, n, X86StrackBuffer->##regDisplay); \
    return 0; \
}

// allocate 0x48 bytes instead of 0x3c bytes
// copy 0x12 dwords intead of 0x0f dwords
unsigned int CanBuildArrayBufferOverrunFixAddr = 0x42dac7;
BYTE CanBuildArrayBufferOverrunFixBytes[] = {
	0x6a, 0x48, 0x50, 0xe8, 0xe1, 0xa8, 0x0a, 0x00,
	0x89, 0x86, 0x56, 0x01, 0x00, 0x00, 0xb9, 0x12
};

LONG CALLBACK VectoredHandler(
	_In_  PEXCEPTION_POINTERS ExceptionInfo
	)
{
	//return EXCEPTION_CONTINUE_EXECUTION;
	return EXCEPTION_CONTINUE_SEARCH;
}

TABugFixing::TABugFixing ()
{

	MaxUnitID= 0;

	NullUnitDeathVictim= NULL;
	CircleRadius= NULL;
	CrackCd= NULL;
	CrackCd2= NULL;
	CrackCd3= NULL;
	BadModelHunter_ISH= NULL;
	for (int i= 0; i<GUIERRORCOUNT; i++)
	{
		GUIErrorLengthHookAry[i]= NULL;
	}


	CDMusic_TAB= NULL;

	CDMusic_Menu_Pause= NULL;
	CDMusic_Victory_Pause= NULL;

	CDMusic_StopButton= NULL;

	UnitVolumeYequZero= NULL;

	UnitIDOutRange= NULL;

	UnitDeath_BeforeUpdateUI= NULL;

	if (MyConfig->GetIniBool("DisplayModeMinHeight768", FALSE))
	{
		DisplayModeMinHeight768Enum.reset(new SingleHook(DisplayModeMinHeight768EnumAddr, sizeof(DisplayModeMinHeight768EnumBits), INLINE_UNPROTECTEVINMENT, DisplayModeMinHeight768EnumBits));
		DisplayModeMinHeight768Def.reset(new SingleHook(DisplayModeMinHeight768DefAddr, sizeof(DisplayModeMinHeight768DefBits), INLINE_UNPROTECTEVINMENT, DisplayModeMinHeight768DefBits));
		DisplayModeMinHeight768Reg.reset(new InlineSingleHook(DisplayModeMinHeight768RegAddr, 5, INLINE_5BYTESLAGGERJMP, CheckDisplayModeHeightReg));
	
		DisplayModeMinWidth1024Def.reset(new SingleHook(DisplayModeMinWidth1024DefAddr, sizeof(DisplayModeMinWidth1024DefBits), INLINE_UNPROTECTEVINMENT, DisplayModeMinWidth1024DefBits));
		DisplayModeMinWidth1024Reg.reset(new InlineSingleHook(DisplayModeMinWidth1024RegAddr, 5, INLINE_5BYTESLAGGERJMP, CheckDisplayModeWidthReg));
	}

	SinglePlayerStartButton.reset(new SingleHook(SinglePlayerStartButtonAddr, sizeof(SinglePlayerStartButtonBits), INLINE_UNPROTECTEVINMENT, SinglePlayerStartButtonBits));

	//new SingleHook(EnableClickSnapAddr, sizeof(EnableClickSnapBits), INLINE_UNPROTECTEVINMENT, EnableClickSnapBits);

	DrawPlayer11DT.reset(new InlineSingleHook(DrawPlayer11DTAddr, 5, INLINE_5BYTESLAGGERJMP, DrawPlayer11DTProc));
	//DrawPlayer11DTEnable[0].reset(new SingleHook(DrawPlayer11DTEnableAddrs[0], sizeof(DrawPlayer11DTEnableBits), INLINE_UNPROTECTEVINMENT, DrawPlayer11DTEnableBits));
	//DrawPlayer11DTEnable[1].reset(new SingleHook(DrawPlayer11DTEnableAddrs[1], sizeof(DrawPlayer11DTEnableBits), INLINE_UNPROTECTEVINMENT, DrawPlayer11DTEnableBits));
	//DrawPlayer11DTEnable[2].reset(new SingleHook(DrawPlayer11DTEnableAddrs[2], sizeof(DrawPlayer11DTEnableBits), INLINE_UNPROTECTEVINMENT, DrawPlayer11DTEnableBits));

	NullUnitDeathVictim.reset(new SingleHook ( NullUnitDeathVictimAddr, sizeof(NullUnitDeathVictimBits), INLINE_UNPROTECTEVINMENT, NullUnitDeathVictimBits));

	CircleRadius.reset(new SingleHook ( CircleRadiusAddr, sizeof(CircleRadiusBits), INLINE_UNPROTECTEVINMENT, CircleRadiusBits));

	BadModelHunter_ISH.reset(new InlineSingleHook ( BadModelHunterAddr, 5, INLINE_5BYTESLAGGERJMP, BadModelHunter));

	CrackCd.reset(new SingleHook ( CrackCdAddr, sizeof(CrackCDBits), INLINE_UNPROTECTEVINMENT, CrackCDBits));

	CrackCd2.reset(new SingleHook ( CrackCd2Addr, sizeof(CrackCD2Bits), INLINE_UNPROTECTEVINMENT, CrackCD2Bits));
	
	CrackCd3.reset(new SingleHook ( CrackCd3Addr, sizeof(CrackCD3Bits), INLINE_UNPROTECTEVINMENT, CrackCD3Bits));
	for (int i= 0; i<GUIERRORCOUNT; i++)
	{
		GUIErrorLengthHookAry[i].reset(new SingleHook ( GUIErrorLengthAry[i], sizeof(GUIErrorLengthBits), INLINE_UNPROTECTEVINMENT, GUIErrorLengthBits));
	}

    LosTypeShouldBeACheatCode.reset(new SingleHook(LosTypeShouldBeACheatCodeAddr, sizeof(LosTypeShouldBeACheatCodeBits), INLINE_UNPROTECTEVINMENT, LosTypeShouldBeACheatCodeBits));

	HMODULE Audiere_hm= GetModuleHandleA ( "audiere.dll");
	CDMusic_TAB= NULL;
	CDMusic_Menu_Pause= NULL;
	CDMusic_Victory_Pause= NULL;
	CDMusic_StopButton= NULL;
	if (NULL!=Audiere_hm)
	{// install music cd hook

		CDMusic_TAB.reset(new SingleHook ( CDMusic_TABAddr, sizeof(CDMusic_TABBits), INLINE_UNPROTECTEVINMENT, CDMusic_TABBits));
		CDMusic_Menu_Pause.reset(new InlineSingleHook ( CDMusic_Menu_PauseAddr, 5, INLINE_5BYTESLAGGERJMP, CDMusic_MenuProc));
		CDMusic_Victory_Pause.reset(new InlineSingleHook ( CDMusic_Victory_PauseAddr, 5, INLINE_5BYTESLAGGERJMP, CDMusic_VictoryProc));
		
	}

    SavePlayerColor.reset(new InlineSingleHook(SavePlayerColorHookAddr, 5, INLINE_5BYTESLAGGERJMP, SavePlayerColorProc));
    RestorePlayerColor.reset(new InlineSingleHook(RestorePlayerColorHookAddr, 5, INLINE_5BYTESLAGGERJMP, RestorePlayerColorProc));
	UnitDeath_BeforeUpdateUI.reset(new InlineSingleHook ( UnitDeath_BeforeUpdateUIAddr, 5, INLINE_5BYTESLAGGERJMP, UnitDeath_BeforeUpdateUI_Proc));
	ResourceStripHeightFix.reset(new InlineSingleHook(ResourceStripHeightFixAddr, 5, INLINE_5BYTESLAGGERJMP, ResourceStripHeightFixProc));
	PatrolDisableBuildRepair.reset(new InlineSingleHook(PatrolDisableBuildRepairAddr, 5, INLINE_5BYTESLAGGERJMP, PatrolDisableBuildRepairProc));
	//PatrolDisableReclaim.reset(new InlineSingleHook(PatrolDisableReclaimAddr, 5, INLINE_5BYTESLAGGERJMP, PatrolDisableReclaimProc));
	VTOLPatrolDisableBuildRepair.reset(new InlineSingleHook(VTOLPatrolDisableBuildRepairAddr, 5, INLINE_5BYTESLAGGERJMP, VTOLPatrolDisableBuildRepairProc));
	//VTOLPatrolDisableReclaim.reset(new InlineSingleHook(VTOLPatrolDisableReclaimAddr, 5, INLINE_5BYTESLAGGERJMP, VTOLPatrolDisableReclaimProc));
	JammingOwnRadar.reset(new InlineSingleHook(JammingOwnRadarAddr, 5, INLINE_5BYTESLAGGERJMP, JammingOwnRadarProc));
	KeepOnReclaimPreparedOrder.reset(new InlineSingleHook(KeepOnReclaimPreparedOrderAddr, 5, INLINE_5BYTESLAGGERJMP, KeepOnReclaimPreparedOrderProc));
	GhostComFix.reset(new InlineSingleHook(GhostComFixAddr, 5, INLINE_5BYTESLAGGERJMP, GhostComFixProc));
	GhostComFixAssist.reset(new InlineSingleHook(GhostComFixAssistAddr, 5, INLINE_5BYTESLAGGERJMP, GhostComFixAssistProc));
	FixFactoryExplosionsInit.reset(new InlineSingleHook(FixFactoryExplosionsInitAddr, 5, INLINE_5BYTESLAGGERJMP, FixFactoryExplosionsInitProc));
	FixFactoryExplosionsAssignUnitId.reset(new InlineSingleHook(FixFactoryExplosionsAssignUnitIdAddr, 5, INLINE_5BYTESLAGGERJMP, FixFactoryExplosionsAssignUnitIdProc));
	FixFactoryExplosionsRecycleUnitId.reset(new InlineSingleHook(FixFactoryExplosionsRecycleUnitIdAddr, 5, INLINE_5BYTESLAGGERJMP, FixFactoryExplosionsRecycleUnitIdProc));
	HostDoesntLeave.reset(new InlineSingleHook(PutDeadHostInWatchModeAddr, 5, INLINE_5BYTESLAGGERJMP, PutDeadHostInWatchModeProc));
	JunkYardmapFix.reset(new InlineSingleHook(JunkYardmapFixAddr, 5, INLINE_5BYTESLAGGERJMP, JunkYardmapFixProc));
	CanBuildArrayBufferOverrunFix.reset(new SingleHook(CanBuildArrayBufferOverrunFixAddr, sizeof(CanBuildArrayBufferOverrunFixBytes), INLINE_UNPROTECTEVINMENT, CanBuildArrayBufferOverrunFixBytes));
	WindSpeedSync.reset(new InlineSingleHook(WindSpeedSyncAddr, 5, INLINE_5BYTESLAGGERJMP, WindSpeedSyncProc));
	//NetworkRawReceiveLog.reset(new InlineSingleHook(NetworkRawReceiveLogAddr, 5, INLINE_5BYTESLAGGERJMP, NetworkRawReceiveLogProc));
	//NetworkDispatchLog.reset(new InlineSingleHook(NetworkDispatchLogAddr, 5, INLINE_5BYTESLAGGERJMP, NetworkDispatchLogProc));
	//SingleHooks.push_back(std::make_unique<InlineSingleHook>(LogTrace1Addr, 5, INLINE_5BYTESLAGGERJMP, LogTrace1Proc));

	AddVectoredExceptionHandler ( TRUE, VectoredHandler );
}

TABugFixing::~TABugFixing ()
{
	RemoveVectoredExceptionHandler  ( VectoredHandler);
}


BOOL TABugFixing::AntiCheat (void)
{
	// sync "+now Film Chris Include Reload Assert"  with cheating

	if (TRUE==*IsCheating)
	{
		(*TAmainStruct_PtrPtr)->SoftwareDebugMode|= 2;
	}
	else
	{
		(*TAmainStruct_PtrPtr)->SoftwareDebugMode= ((*TAmainStruct_PtrPtr)->SoftwareDebugMode)& (~ 2);
	}

	return TRUE;
}



void LogToErrorlog (LPSTR Str)
{
	HANDLE file = CreateFileA("ErrorLog.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS	, 0, NULL);
	DWORD tempWritten;
	SetFilePointer ( file, 0, 0, FILE_END);
	WriteFile ( file, Str, strlen(Str), &tempWritten, NULL);
	WriteFile ( file, "\r\n", 2, &tempWritten, NULL);

	CloseHandle ( file);
}
/*
.text:00458C5A 078 C1 E9 02                                                        shr     ecx, 2          ; let's add a check in here, if  ecx is bigger than 600*600
	.text:00458C5D 078 F3 AB                                                           rep stosd               ; init the CompositeBuffer as background*/

int __stdcall BadModelHunter (PInlineX86StackBuffer X86StrackBuffer)
{
	OFFSCREEN * Offscreen_p= (OFFSCREEN *)(X86StrackBuffer->Esp+ 0x48);
	if ((600<(Offscreen_p->Width))
		||(600<(Offscreen_p->Height)))
	{// record thsi shit.
		Object3doStruct * Obj_ptr=  *(Object3doStruct * *)(X86StrackBuffer->Esp+ 0x68+ 0x10+ 0x8);

		LogToErrorlog ( "\r\n===============================\r\n");
		LogToErrorlog ( "Erroneous unit model :");
		LogToErrorlog ( "Bad Unit ID:");
		LogToErrorlog ( Obj_ptr->ThisUnit->UnitType->Name);
		LogToErrorlog ( "Bad Unit Name:");
		LogToErrorlog ( Obj_ptr->ThisUnit->UnitType->UnitName);
		LogToErrorlog ( "Bad Unit Description:");
		LogToErrorlog ( Obj_ptr->ThisUnit->UnitType->UnitDescription);
		LogToErrorlog ( "Bad Unit ObjectName:");
		LogToErrorlog ( Obj_ptr->ThisUnit->UnitType->ObjectName);

		LogToErrorlog ( "\r\n===============================\r\n");

		SendText("Warning! Error detected and posted to ErrorLog.txt (in your TA Path).\r\nPlease forward along with your game's recording to Report-Bugs via our Discord site.\r\nNote units on screen and click OK to remove this message", 1);

		X86StrackBuffer->Edi= *(DWORD *)(X86StrackBuffer->Esp);

		X86StrackBuffer->Esi= *(DWORD *)(X86StrackBuffer->Esp+ 4);

		X86StrackBuffer->Ebp= *(DWORD *)(X86StrackBuffer->Esp+ 8);

		X86StrackBuffer->Ebx= *(DWORD *)(X86StrackBuffer->Esp+ 0xc);

		X86StrackBuffer->Esp= X86StrackBuffer->Esp+ 4+ 4+ 4+ 4+ 0x68+ 4+ 8;
		
		X86StrackBuffer->rtnAddr_Pvoid= (LPVOID)SafeModelAddr;

		return X86STRACKBUFFERCHANGE;
		//rtn to 	
	}
	return 0;
}

int __stdcall CDMusic_MenuProc (PInlineX86StackBuffer X86StrackBuffer)
{
	if (1==*(DWORD * )(X86StrackBuffer->Esp+ 4))
	{
		PauseCDMusic ( );
	}

	return 0;
}

int __stdcall CDMusic_VictoryProc (PInlineX86StackBuffer X86StrackBuffer)
{
	PauseCDMusic ( );
	return 0;
}


int __stdcall UnitVolumeYequZero_Proc (PInlineX86StackBuffer X86StrackBuffer)
{
	if (0==((UnitStruct *)X86StrackBuffer->Edx)->Turn.Y)
	{
		//CalcUnitTurn ( (UnitStruct *)X86StrackBuffer->Edx);
		((UnitStruct *)X86StrackBuffer->Edx)->Turn.Y= 1;
	}

	return 0;
}



int __stdcall UnitIDOutRange_Proc (PInlineX86StackBuffer X86StrackBuffer)
{
	if (((DWORD)X86StrackBuffer->myInlineHookClass_Pish->ParamOfHook)<(0xffff& X86StrackBuffer->Eax))
	{
		X86StrackBuffer->rtnAddr_Pvoid= (LPVOID)UnitIDOutRangeRtn;

		return X86STRACKBUFFERCHANGE;
	}
	return 0;
}


int __stdcall UnitDeath_BeforeUpdateUI_Proc  (PInlineX86StackBuffer X86StrackBuffer)
{
	if (ordertype::STOP!=(*TAmainStruct_PtrPtr)->PrepareOrder_Type)
	{//
		(*TAmainStruct_PtrPtr)->PrepareOrder_Type= ordertype::STOP;
	}

	return 0;
}


int __stdcall EnterProc  (PInlineX86StackBuffer X86StrackBuffer)
{
	EnterCriticalSection ((LPCRITICAL_SECTION)X86StrackBuffer->myInlineHookClass_Pish->ParamOfHook);
	return 0;
}
int __stdcall LeaveProc  (PInlineX86StackBuffer X86StrackBuffer)
{
	LeaveCriticalSection ((LPCRITICAL_SECTION)X86StrackBuffer->myInlineHookClass_Pish->ParamOfHook);
	return 0;
}

static PlayerInfoStruct* SavePlayerColorPtr[10] = { NULL };
static char SavePlayerColor[10] = { 0 };
int __stdcall SavePlayerColorProc(PInlineX86StackBuffer X86StrackBuffer)
{
	unsigned char playerNumber = *(unsigned char*)(X86StrackBuffer->Esp + 0x44);
	if (playerNumber < 10) {
		SavePlayerColorPtr[playerNumber] = (PlayerInfoStruct*)(*(unsigned*)(X86StrackBuffer->Eax + 0x1b8a));
		SavePlayerColor[playerNumber] = SavePlayerColorPtr[playerNumber]->PlayerLogoColor;
	}
	return 0;
}

int __stdcall RestorePlayerColorProc(PInlineX86StackBuffer X86StrackBuffer)
{
	if (DataShare->TAProgress == TAInGame && SavePlayerColorPtr != NULL) {
		unsigned char playerNumber = *(unsigned char*)(X86StrackBuffer->Esp + 0x44);
		if (playerNumber < 10) {
			SavePlayerColorPtr[playerNumber]->PlayerLogoColor = SavePlayerColor[playerNumber];
		}
	}
	return 0;
}

int __stdcall CheckDisplayModeHeightReg(PInlineX86StackBuffer X86StrackBuffer)
{
	if (X86StrackBuffer->Eax < 768)
	{
		X86StrackBuffer->Eax = 768;
	}

	return 0;
}

int __stdcall CheckDisplayModeWidthReg(PInlineX86StackBuffer X86StrackBuffer)
{
	if (X86StrackBuffer->Eax < 1024)
	{
		X86StrackBuffer->Eax = 1024;
	}

	return 0;
}
