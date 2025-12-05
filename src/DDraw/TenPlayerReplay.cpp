#include "TenPlayerReplay.h"
#include "HexDump.h"

#include "iddrawsurface.h"
#include "hook/etc.h"
#include "hook/hook.h"
#include "tafunctions.h"
#include "tamem.h"
#include "TPacket.h"

#include <sstream>

static bool HACK_ON = false;
static PlayerStruct* LOCAL_HUMAN_PLAYER = NULL;

unsigned int HapiReceiveHookAddr = 0x4c9882;
int __stdcall HapiReceiveHookProc(PInlineX86StackBuffer X86StrackBuffer)
{
	HAPINETStruct* hapinet = *(HAPINETStruct**)(X86StrackBuffer->Esp + 0x00 + 0x04);
	if (hapinet && DataShare->PlayingDemo && TAInGame == DataShare->TAProgress)
	{
		std::uint8_t* buffer = *(std::uint8_t**)(X86StrackBuffer->Esp + 0x00 + 0x08);
		std::uint32_t* bufferSize = *(std::uint32_t**)(X86StrackBuffer->Esp + 0x00 + 0x0c);

		static const tapacket::bytestring hackon((const std::uint8_t*)"\x03\x04\x05\x06\x02\x08\x24\x0C\x0B\x0D", 10);
		static const tapacket::bytestring hackoff((const std::uint8_t*)"\x03\x04\x05\x06\x02\x08\x24\x0C\x0B\x0C", 10);

		const tapacket::bytestring hacktest(buffer + 3u, hackon.size());
		if (hacktest == hackon)
		{
			//IDDrawSurface::OutptTxt("[HapiReceiveHookProc] hack on ...");
			HACK_ON = true;
		}
		else// if (hacktest == hackoff)
		{
			//IDDrawSurface::OutptTxt("[HapiReceiveHookProc] hack off ...");
			HACK_ON = false;
		}

		if (HACK_ON)
		{
			TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
			if (!LOCAL_HUMAN_PLAYER && taPtr->Players[taPtr->LocalHumanPlayer_PlayerID].My_PlayerType == Player_LocalHuman)
			{
				LOCAL_HUMAN_PLAYER = &taPtr->Players[taPtr->LocalHumanPlayer_PlayerID];
				IDDrawSurface::OutptFmtTxt("[HapiReceiveHookProc] LOCAL_HUMAN_PLAYER.dpid=%d(%x)", LOCAL_HUMAN_PLAYER->DirectPlayID, LOCAL_HUMAN_PLAYER->DirectPlayID);
			}

			if (LOCAL_HUMAN_PLAYER)
			{
				taPtr->hapinet.fromDpid = LOCAL_HUMAN_PLAYER->DirectPlayID;
				LOCAL_HUMAN_PLAYER->My_PlayerType = Player_RemoteHuman;
				for (int i = 0; i < 10; ++i)
				{
					LOCAL_HUMAN_PLAYER->AllyFlagAry[i] = 1;
				}
			}
		}
	}

	return 0;
}

unsigned int GetLocalPlayerDpidHackAddr = 0x44fdb1;
int __stdcall GetLocalPlayerDpidHackProc(PInlineX86StackBuffer X86StrackBuffer)
{
	if (LOCAL_HUMAN_PLAYER)
	{
		X86StrackBuffer->Eax = LOCAL_HUMAN_PLAYER->DirectPlayID;
		//IDDrawSurface::OutptFmtTxt("[GetLocalPlayerDpidHackProc] dpid=%d(%x)", X86StrackBuffer->Eax, X86StrackBuffer->Eax);
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x44fdf3;	// return eax
		return X86STRACKBUFFERCHANGE;
	}

	return 0;
}

#define LOCAL_PLAYER_CHECK_HACK(hookAddr, n, playerRegister, returnAddr) \
unsigned int LocalPlayerCheckHack##n##Addr = (hookAddr); \
int __stdcall LocalPlayerCheckHack##n##Proc(PInlineX86StackBuffer X86StrackBuffer) \
{ \
    PlayerStruct* fromPlayer = (PlayerStruct*)(X86StrackBuffer->##playerRegister); \
    if (fromPlayer == LOCAL_HUMAN_PLAYER) \
    { \
		/*IDDrawSurface::OutptFmtTxt("[LocalPlayerCheckHack%d] fromPlayer=%d(%x)", n, fromPlayer->DirectPlayID, fromPlayer->DirectPlayID);*/ \
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)(returnAddr); \
        return X86STRACKBUFFERCHANGE; \
    } \
    return 0; \
}

LOCAL_PLAYER_CHECK_HACK(0x45650d, 1, Ebp, 0x456518)
LOCAL_PLAYER_CHECK_HACK(0x4564c7, 2, Ebp, 0x4564d6)
LOCAL_PLAYER_CHECK_HACK(0x45635c, 3, Ebp, 0x45636b)
LOCAL_PLAYER_CHECK_HACK(0x454aa8, 4, Ebx, 0x454ab3)
LOCAL_PLAYER_CHECK_HACK(0x45324a, 5, Eax, 0x453263)
LOCAL_PLAYER_CHECK_HACK(0x4531c2, 6, Edx, 0x4531db)
LOCAL_PLAYER_CHECK_HACK(0x453126, 7, Ecx-0x73, 0x453140)
LOCAL_PLAYER_CHECK_HACK(0x452bda, 8, Ecx, 0x452beb)
LOCAL_PLAYER_CHECK_HACK(0x451e8f, 9, Eax, 0x451ea0) // HAPI_BroadcastMessage
LOCAL_PLAYER_CHECK_HACK(0x451cd0, 10, Esi, 0x451cda) // HAPI_SendBuf
LOCAL_PLAYER_CHECK_HACK(0x451ca5, 11, Esi, 0x451cb4) // HAPI_SendBuf
LOCAL_PLAYER_CHECK_HACK(0x451ac5, 12, Ebp, 0x451ad0)
LOCAL_PLAYER_CHECK_HACK(0x451a7f, 13, Ebp, 0x451a8e)
LOCAL_PLAYER_CHECK_HACK(0x451010, 14, Ebp, 0x45101b)
LOCAL_PLAYER_CHECK_HACK(0x450fca, 15, Ebp, 0x450fd9)
LOCAL_PLAYER_CHECK_HACK(0x450cc6, 16, Ebp, 0x450cd1)
LOCAL_PLAYER_CHECK_HACK(0x450c80, 17, Ebp, 0x450c8f)
LOCAL_PLAYER_CHECK_HACK(0x455237, 18, Ebx, 0x455241) // Packet_Dispatcher CHAT_05 handler
LOCAL_PLAYER_CHECK_HACK(0x45339d, 19, Edx-0x73, 0x4534c2) // BroadcastText


//unsigned int PermSonarLosAddr = 0x42c3e6;  // patch commander unit definition
unsigned int PermSonarLosAddr = 0x46754a;	 // patch sonar calculation
int __stdcall PermSonarLosProc(PInlineX86StackBuffer X86StrackBuffer)
{
	static const int RADIUS = 30000;

	// patch commander unit definition
	//UnitDefStruct* u = (UnitDefStruct*)X86StrackBuffer->Ebp;
	//if (DataShare->PlayingDemo && !strcmp(u->Name, "Commander"))
	//{
	//	u->nSonarDistance = RADIUS;
	//}
	//return 0;

	// patch sonar calculation
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	if (DataShare->PlayingDemo && TAInGame == DataShare->TAProgress &&
		taPtr->LOS_Sight_PlayerID == taPtr->LocalHumanPlayer_PlayerID)
	{
		UnitStruct* unit = (UnitStruct*)X86StrackBuffer->Esi;
		PlayerStruct* viewingPlayer = &taPtr->Players[taPtr->LOS_Sight_PlayerID];
		if (unit->Owner_PlayerPtr0 == viewingPlayer)
		{
			for (int i = 0; i < taPtr->PlayerUnitsNumber_Skim; ++i)
			{
				if (viewingPlayer->Units[i].IsUnit)
				{
					if (viewingPlayer->Units[i].UnitInGameIndex == unit->UnitInGameIndex)
					{
						// viewing player's first unit gets a sonar buff
						int posy = *(short*)(X86StrackBuffer->Esi + 0x70);
						X86StrackBuffer->Ecx = RADIUS;// +2 * posy;
						X86StrackBuffer->Edx = RADIUS;
						X86StrackBuffer->Eax = RADIUS;
						X86StrackBuffer->Ecx *= X86StrackBuffer->Ecx;
						X86StrackBuffer->Edx *= X86StrackBuffer->Edx;

						//X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x467586;
						//return X86STRACKBUFFERCHANGE;

						// large nSonarDistance doesn't seem to work unless unit is near top left of map ...
						static DWORD fakePosition[3] = { 0, 0, 0 };
						*(DWORD*)(X86StrackBuffer->Esp + 0x24) = X86StrackBuffer->Ecx;
						X86StrackBuffer->Ecx = (DWORD)&fakePosition[0];
						X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x46758d;
						return X86STRACKBUFFERCHANGE;
					}
					return 0;
				}
			}
		}
	}
	return 0;
}

std::unique_ptr<TenPlayerReplay> TenPlayerReplay::m_instance = NULL;

TenPlayerReplay::TenPlayerReplay()
{
	IDDrawSurface::OutptTxt("[TenPlayerReplay] initialising ...");
	m_hooks.push_back(std::make_unique<InlineSingleHook>(HapiReceiveHookAddr, 5, INLINE_5BYTESLAGGERJMP, HapiReceiveHookProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(GetLocalPlayerDpidHackAddr, 5, INLINE_5BYTESLAGGERJMP, GetLocalPlayerDpidHackProc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack1Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack1Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack2Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack2Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack3Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack3Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack4Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack4Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack5Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack5Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack6Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack6Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack7Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack7Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack8Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack8Proc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack9Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack9Proc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack10Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack10Proc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack11Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack11Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack12Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack12Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack13Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack13Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack14Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack14Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack15Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack15Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack16Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack16Proc));
	//m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack17Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack17Proc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack18Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack18Proc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(LocalPlayerCheckHack19Addr, 5, INLINE_5BYTESLAGGERJMP, LocalPlayerCheckHack19Proc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(PermSonarLosAddr, 5, INLINE_5BYTESLAGGERJMP, PermSonarLosProc));
}


TenPlayerReplay* TenPlayerReplay::GetInstance()
{
	if (m_instance == NULL) {
		m_instance.reset(new TenPlayerReplay());
	}
	return m_instance.get();
}
