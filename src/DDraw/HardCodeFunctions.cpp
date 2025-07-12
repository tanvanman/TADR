#include "oddraw.h"
#include "iddrawsurface.h"
#include "tamem.h"
#include "tafunctions.h"
#include "stdio.h"

#ifdef min
#undef min
#endif
#include <algorithm>

int ViewPlayerLos_Replay (int PlayerAryIndex, BOOL HaveControl)
{
	TAdynmemStruct * PTR1 = *(TAdynmemStruct * *)0x511de8;

	PTR1->LosType= PTR1->LosType& (~ Permanent);
	PTR1->LosType= PTR1->LosType& (~ NOMAPPING);

	
	//PTR1->LOS_Sight_PlayerID= 0;
	char Curt_LOS_Sight_PlayerID= PTR1->LOS_Sight_PlayerID;

	if (PlayerAryIndex<10
		&&(0!=PTR1->Players[PlayerAryIndex].PlayerActive)
		&& (10!=PTR1->Players[PlayerAryIndex].PlayerAryIndex))
	{//valid player ID
		unsigned char CurtPlayerType= PTR1->Players[PlayerAryIndex].My_PlayerType;

		if ((Player_LocalHuman==CurtPlayerType)
			|| (Player_LocalAI==CurtPlayerType)
			|| (Player_RemoteHuman==CurtPlayerType)
			|| (Player_RemoteAI==CurtPlayerType))
		{
			if (0==PlayerAryIndex)
			{//back to my view
				
				PlayerAryIndex= LocalShare->OrgLocalPlayerID;
				PTR1->LocalHumanPlayer_PlayerID= PlayerAryIndex;
			}
			else
			{
				PTR1->LosType|= Permanent;
				PTR1->LosType|= NOMAPPING;
			}

			Curt_LOS_Sight_PlayerID= PlayerAryIndex;
			//PTR1->LOS_Sight_PlayerID= Curt_LOS_Sight_PlayerID;
			char ChatBuff[0x100];
			memset ( ChatBuff, 0, 0x100);
			reinterpret_cast<char * *>(ChatBuff)[0]= &ChatBuff[0x10];
			reinterpret_cast<char * *>(ChatBuff)[1]= &ChatBuff[0x20];
			lstrcpyA ( &ChatBuff[0x10], "view");
			
			sprintf_s(&ChatBuff[0x20], 0xd0-0x20, "%d", int(Curt_LOS_Sight_PlayerID));
			ChatBuff[0xd0]= 2;
			ViewCommandProc ( ChatBuff);
			if (HaveControl)
			{
				PTR1->LocalHumanPlayer_PlayerID= Curt_LOS_Sight_PlayerID;
			}
		}
	}


	LoadTARegConfig ( );
	UpdateLosState ( 0);
	
	//PTR1->LOS_Sight_PlayerID= Curt_LOS_Sight_PlayerID;
	//UpdateLosState ( 0);
	return Curt_LOS_Sight_PlayerID;
}

int UpdateTAProcess (void)
{
	TAdynmemStruct * PTR1 = *(TAdynmemStruct * *)0x511de8;


// 	if (2==PTR1->State_GUI_CallBack)
// 	{
// 		DataShare->TAProgress= TALobby;
// 	}
//	else 
	if (5==PTR1->State_GUI_CallBack)
	{
		DataShare->TAProgress= TALoading;
	}
	else if (6==PTR1->State_GUI_CallBack)
	{
		DataShare->TAProgress= TAInGame;
	}
	else
	{
		DataShare->TAProgress= TALobby;
	}
	if (0==(4& (*TAmainStruct_PtrPtr)->WorkStatusMask))
	{
		DataShare->TAProgress= TAExiting;
	}

	return DataShare->TAProgress;
}

int CountSelectedUnits (void)
{
	int Count= 0;
	TAdynmemStruct * TAmainStruct_Ptr= * TAmainStruct_PtrPtr;

	UnitStruct * Start= TAmainStruct_Ptr->Players[TAmainStruct_Ptr->LocalHumanPlayer_PlayerID].Units;
	UnitStruct * EndOfUnitsArray_p=  TAmainStruct_Ptr->Players[TAmainStruct_Ptr->LocalHumanPlayer_PlayerID].UnitsAry_End;

	if ((NULL==Start)
		||(NULL==EndOfUnitsArray_p))
	{
		return 0;
	}

	while (Start<=EndOfUnitsArray_p)
	{
		if (Start->UnitID)
		{
			if (0!=(0x20& Start->UnitSelected))
			{
				if (0.0F==(Start->Nanoframe))
				{
					if (0!=(0x10& Start->UnitSelected))
					{
						Count++;
					}
				}
			}
		}
		Start= &Start[1];
	}

	return Count;
}

void SendOrder (unsigned int TAX, unsigned int TAY, unsigned int TAZ, int OrderType, bool Shift)
{
	MOUSEEVENT Mevent;
	Position_Dword Pos;

	Mevent.X= 0;
	Mevent.Y= 0;
	Mevent.Msg= WM_NULL;
	Mevent.PressTime_sec= 0;
	Mevent.fwKeys= Shift? 1<<2: 0;

	Pos.X= TAX;
	Pos.Y= TAY;
	Pos.Z= TAZ;

	MOUSE_EVENT_2UnitOrder_ ( &Mevent, OrderType, 0, &Pos, 0, 0);
}

void SendOrder(UnitStruct* unit, unsigned int TAX, unsigned int TAY, unsigned int TAZ, UnitStruct* target, ordertype::ORDERTYPE orderType, int orCobIndex, bool shift)
{
	typedef char*(__stdcall* _ScriptAction_Type2Index)(int* RtnIndex_ptr, unsigned Action_ID, UnitStruct* OrderUnit, UnitStruct* TargetUnittr, Position_Dword* Position);
	_ScriptAction_Type2Index ScriptAction_Type2Index = (_ScriptAction_Type2Index)0x043f0e0;

	typedef int* (__thiscall* _ScriptAction_Index2Handler)(char* indexTo_trn);
	_ScriptAction_Index2Handler ScriptAction_Index2Handler = (_ScriptAction_Index2Handler)0x0438830;

	typedef void(__stdcall* _ORDERS_NewMainOrder2Unit)(int ActionIndex, int shift_key, UnitStruct* OrderUnit_p, UnitStruct* TargatUnit, Position_Dword* Position_DWORD_p, int lPar1, int lPar2);
	_ORDERS_NewMainOrder2Unit ORDERS_NewMainOrder2Unit = (_ORDERS_NewMainOrder2Unit)0x043afc0;

	(*TAmainStruct_PtrPtr)->MouseMapPos;
	Position_Dword pos;
	std::memset(&pos, 0, sizeof(pos));
	pos.X = TAX;
	pos.Y = TAY;
	pos.Z = TAZ;

	int index = orCobIndex;
	char* actionIndex = int(orderType) != 0
		? ScriptAction_Type2Index(&index, orderType, unit, target, &pos)
		: (char*)&orCobIndex;

	if (actionIndex)
	{
		ScriptAction_Index2Handler(actionIndex);
		ORDERS_NewMainOrder2Unit(*actionIndex, shift, unit, target, &pos, 0, shift);
	}
}

void PushOrder(UnitStruct* unit, unsigned int TAX, unsigned int TAY, unsigned int TAZ, UnitStruct* target, ordertype::ORDERTYPE orderType)
{
	typedef char* (__stdcall* _ScriptAction_Type2Index)(int* RtnIndex_ptr, unsigned Action_ID, UnitStruct* OrderUnit, UnitStruct* TargetUnittr, Position_Dword* Position);
	_ScriptAction_Type2Index ScriptAction_Type2Index = (_ScriptAction_Type2Index)0x043f0e0;

	typedef int(__cdecl* _cmalloc_MM__)(size_t Size);
	_cmalloc_MM__ cmalloc_MM__ = (_cmalloc_MM__)0x04b4f10;

	typedef int(__thiscall* _UnitOrdersStructConstructor)(UnitOrdersStruct* orders, int Scrip_index, UnitStruct* TargatUnit, Position_Dword* Position_DWORD_p, int lPar1, int lPar2, int UnitOrderFlags);
	_UnitOrdersStructConstructor UnitOrdersStructConstructor = (_UnitOrdersStructConstructor)0x43a0c0;

	typedef void(__stdcall* _ORDERS_PushOrder)(UnitStruct * UnitInGame_ptr, UnitOrdersStruct *UnitOrder_ptr);
	_ORDERS_PushOrder ORDERS_PushOrder = (_ORDERS_PushOrder)0x43acb0;

	(*TAmainStruct_PtrPtr)->MouseMapPos;
	Position_Dword pos;
	std::memset(&pos, 0, sizeof(pos));
	pos.X = TAX;
	pos.Y = TAY;
	pos.Z = TAZ;

	int index = -1;
	char actionIndex = *ScriptAction_Type2Index(&index, orderType, unit, target, &pos);

	UnitOrdersStruct* mainOrder = unit->UnitOrders;

	UnitOrdersStruct* newOrders = (UnitOrdersStruct*)cmalloc_MM__(sizeof(UnitOrdersStruct));
	UnitOrdersStructConstructor(newOrders, actionIndex, target, &pos, 0, 0, 0);
	ORDERS_PushOrder(unit, newOrders);
}

LPDWORD GetUnitIDMaskAryByCategory (LPSTR CategoryName_cstrp)
{
	__try
	{// don't trust param and don't trust the way to call TA functions

		__GetUnitIDMaskAryByCategory _GetUnitIDMaskAryByCategory= (__GetUnitIDMaskAryByCategory)0x0488C50;
		return _GetUnitIDMaskAryByCategory ( CategoryName_cstrp);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		;
	}
	return NULL;
}

unsigned char GetPlayerDotColor(int n)
{
	if (n < 0 || n >= 10) {
		return 0u;
	}

	TAdynmemStruct* Ptr = *TAmainStruct_PtrPtr;
	char c;
	if (Ptr && Ptr->Players[n].PlayerInfo) {
		c = Ptr->Players[n].PlayerInfo->PlayerLogoColor;
	}
	else {
		c = DataShare->PlayerColors[n];
	}
	return DataShare->PlayerDotColors[c];
}

bool SetIDMaskInTypeAry (WORD ID, DWORD SelectedUnitTypeIDAry_Dw[])
{
	SelectedUnitTypeIDAry_Dw[ID/ 32]|= (1<< (ID& 0x1f));
	return true;
}

bool CleanIDMaskInTypeAry (WORD ID, DWORD SelectedUnitTypeIDAry_Dw[])
{
	SelectedUnitTypeIDAry_Dw[ID/ 32]&= ! (1<< (ID& 0x1f));
	return true;
}


bool MatchInTypeAry (WORD ID, DWORD SelectedUnitTypeIDAry_Dw[])
{
	if (0!=(SelectedUnitTypeIDAry_Dw[ID/ 32]& (1<< (ID& 0x1f))))
	{
		return true;
	}
	return false;
}

int GetMaxScrollX()
{
	int *PTR = (int*)0x00511de8;
	int *MapSizeX = (int*)(*PTR + 0x1422b);
	return *MapSizeX - (LocalShare->ScreenWidth-128);
}

int GetMaxScrollY()
{
	int *PTR = (int*)0x00511de8;
	int *MapSizeY = (int*)(*PTR + 0x1422f);
	return *MapSizeY - (LocalShare->ScreenHeight-64);
}

void ScrollToCenter(int x, int y)
{
	int *PTR = (int*)TAmainStruct_PtrPtr;
	int *XPointer = (int*)(*PTR + 0x1431f);
	int *YPointer = (int*)(*PTR + 0x14323);
	
	x -= (((*TAProgramStruct_PtrPtr)->ScreenWidth)-128)/2;
	y -= (((*TAProgramStruct_PtrPtr)->ScreenHeight)-64)/2;

	if(x<0)
		x = 0;
	if(y<0)
		y = 0;
	if(x>GetMaxScrollX())
		x = GetMaxScrollX();
	if(y>GetMaxScrollY())
		y = GetMaxScrollY();

	//*XPointer = x;
	*(XPointer + 2) = x;
	//*YPointer = y;
	*(YPointer + 2)= y;
}


void freeTAMem (LPVOID MemAddress)
{
	__try
	{// don't trust param and don't trust the way to call TA functions

		_free_SafeWay free_SafeWay=	(_free_SafeWay)0x004B4F20;

		free_SafeWay ( MemAddress);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		;
	}
}

void UpdateSelectUnitEffect (void)
{
	int *PTR = (int*)0x00511de8;
	BYTE temp= *((BYTE *)(*PTR+ 0x37EBE));
	*((BYTE *)(*PTR+ 0x37EBE))= (BYTE)(temp| 0x10);

	
	*((unsigned short int *)(*PTR+ 0x37E9C))= 0;
}

void ApplySelectUnitMenu_Wapper (void)
{
	int old= (*TAmainStruct_PtrPtr)->PrepareOrder_Type;

	typedef void (__stdcall * _ApplySelectUnitMenu) (void);
	_ApplySelectUnitMenu ApplySelectUnitMenu= (_ApplySelectUnitMenu)0x00495860;
	ApplySelectUnitMenu ();

	(*TAmainStruct_PtrPtr)->PrepareOrder_Type= old;
// 	typedef int( __stdcall *_sub_4A6A40)( GUIInfo *, TAdynmemStruct *);
// 	_sub_4A6A40 sub_4A6A40= (_sub_4A6A40)0x4A6A40;
// 
// 	sub_4A6A40 ( &((*TAmainStruct_PtrPtr)->desktopGUI), *TAmainStruct_PtrPtr);
// 
//  	typedef int (__stdcall * _SetUIUpdateFlag)(GUIInfo * TAUIInfo); 	
// 	_SetUIUpdateFlag SetUIUpdateFlag= (_SetUIUpdateFlag) 0x49FA90;
//  
//  	SetUIUpdateFlag ( &((*TAmainStruct_PtrPtr)->desktopGUI));
}
void DeselectUnits(void)
{
	//clean selected units
	TAdynmemStruct *PTR = *((TAdynmemStruct * *)0x00511de8);
	UnitStruct * Start= PTR->Players[PTR->LocalHumanPlayer_PlayerID].Units;
	UnitStruct * EndOfUnitsArray_p=  PTR->Players[PTR->LocalHumanPlayer_PlayerID].UnitsAry_End;


	while (Start<=EndOfUnitsArray_p)
	{
		if (Start->UnitID)
		{
			if (0!=(0x20& Start->UnitSelected))
			{
				if (0.0F==(Start->Nanoframe))
				{
					Start->UnitSelected= (Start->UnitSelected)& (0xFFFFFFEF);
				}
			}
		}
		Start= &Start[1];
	}
	PTR->ShowRangeUnitIndex= 0;
}
int PauseCDMusic()
{

//  	typedef  int (*_TAPauseMusic)(int Pause_B);
//  	_TAPauseMusic TAPauseMusic= (_TAPauseMusic)0x004CE910;
// 		int Rtn;
// // 
// 	__asm
// 	{
// 		push Pause_b;
// 		mov ecx, [TAmainStruct_PtrPtr];
// 		mov ecx, [ecx];
// 		mov ecx, [ecx+ 0x10];
// 		call TAPauseMusic;
// 		mov Rtn, eax;
// 	}
typedef  MCIERROR  (WINAPI * _mciSendStringA)(  LPCSTR lpstrCommand,  LPSTR lpstrReturnString,  UINT uReturnLength,  HWND hwndCallback);
	_mciSendStringA TAmciSendStringA= *(_mciSendStringA *)0x4FC3D8;
	
	
	return TAmciSendStringA ( "pause cdaudio", NULL, 0, (*TAProgramStruct_PtrPtr)->TAClass_Hwnd);

}

// int DrawRadarCircleComment (LPBYTE Bits, POINT * Aspect, int CenterX, int CenterY, int Radius, int color, LPSTR comment)
// {
// 	//typedef void (__stdcall *_TADrawCircle)(char *context, void *CirclePointer, posstruct *pos, int radius, int color, char *text, int unk);
// 	DrawRadarCircle ( Bits, Aspect, CenterX, CenterY, Radius, color);
// 	//DrawText
// }
int DrawRadarCircle (LPBYTE Bits, POINT * Aspect, int CenterX, int CenterY, int Radius, int color)
{
	OFFSCREEN OffScreen;

	memset ( &OffScreen, 0, sizeof(OFFSCREEN));
	OffScreen.Height=Aspect->y;
	OffScreen.Width= Aspect->x;
	OffScreen.lPitch= Aspect->x;
	OffScreen.lpSurface= Bits;

	OffScreen.ScreenRect.left= 0;
	OffScreen.ScreenRect.right=  Aspect->x- 1;

	OffScreen.ScreenRect.top= 0;
	OffScreen.ScreenRect.bottom= Aspect->y;

	return TARadarDrawCircle ( &OffScreen,  CenterX,  CenterY,  Radius,  color);
}

int DrawDotteCircle (LPBYTE Bits, POINT * Aspect, int CenterX, int CenterY, int Radius, int color, int Spacing, int Dotte_b)
{
	OFFSCREEN OffScreen;

	memset ( &OffScreen, 0, sizeof(OFFSCREEN));
	OffScreen.Height=Aspect->y;
	OffScreen.Width= Aspect->x;
	OffScreen.lPitch= Aspect->x;
	OffScreen.lpSurface= Bits;

	OffScreen.ScreenRect.left= 0;
	OffScreen.ScreenRect.right=  Aspect->x;;

	OffScreen.ScreenRect.top= 0;
	OffScreen.ScreenRect.bottom= Aspect->y;

	return TADrawDotteCircle ( &OffScreen,  CenterX,  CenterY,  Radius,  color, Spacing, Dotte_b);
}

int ChatText (LPCSTR str)
{
    SendText(str, 0);
    if ('+'==str[0])
	{
		CallInternalCommandHandler ( &str[1], 1);
	}
	return strlen ( str);
}

BOOL IsPlayerAllyUnit (int UnitID,int PlayerLosID)
{
	if((0!=(WATCH& ((*TAmainStruct_PtrPtr)->Players[LocalShare->OrgLocalPlayerID].PlayerInfo->PropertyMask)))
		||DataShare->PlayingDemo)
	{// watcher
		return TRUE;
	}
	int UnitLosID= (*TAmainStruct_PtrPtr)->BeginUnitsArray_p [UnitID].myLos_PlayerID;
	if(PlayerLosID==UnitLosID)
	{
		return TRUE;
	}

	if ((*TAmainStruct_PtrPtr)->Players[PlayerLosID].AllyFlagAry[UnitLosID])
	{
		return TRUE;
	}
	return FALSE;
}

void GetWeatherReport(int& _solar, int& windPower, int & windPowerMin, int & windPowerMax, int& tidalPower)
{
	TAdynmemStruct* ptrMain = *(TAdynmemStruct**)0x0511DE8;

	static int solarPower = 0;
	static int stdWindGenMaxPower = 0;
	if (solarPower == 0 || stdWindGenMaxPower == 0)
	{
		for (int n = 0; n < ptrMain->UNITINFOCount; ++n)
		{
			UnitDefStruct& u = ptrMain->UnitDef[n];
			if (!std::strncmp(u.UnitName, "ARMSOLAR", sizeof(u.Name)))
			{
				solarPower = -int(u.energyuse);
			}
			else if (!std::strncmp(u.UnitName, "ARMWIN", sizeof(u.Name)))
			{
				stdWindGenMaxPower = int(u.windgenerator);
			}
		}
	}
	if (solarPower == 0 || stdWindGenMaxPower == 0)
	{
		solarPower = 20;
		stdWindGenMaxPower = 30;
	}

	_solar = solarPower;
	tidalPower = ptrMain->TidalStrength;
	windPower = 0;
	windPowerMin = 0;
	windPowerMax = 0;

	const int windSpeedHardLimit = ptrMain->WindSpeedHardLimit;
	if (windSpeedHardLimit > 0) {
		windPower = (stdWindGenMaxPower * ptrMain->WindSpeed + windSpeedHardLimit / 2) / windSpeedHardLimit;
		windPowerMin = (stdWindGenMaxPower * ptrMain->MinWindSpeed + windSpeedHardLimit / 2) / windSpeedHardLimit;
		windPowerMax = (stdWindGenMaxPower * ptrMain->MaxWindSpeed + windSpeedHardLimit / 2) / windSpeedHardLimit;

		windPower = std::min(windPower, stdWindGenMaxPower);
		windPowerMin = std::min(windPowerMin, stdWindGenMaxPower);
		windPowerMax = std::min(windPowerMax, stdWindGenMaxPower);
	}
}

PlayerStruct* FindPlayerByName(const char* name)
{
	TAdynmemStruct* ptrMain = *(TAdynmemStruct**)0x0511DE8;
	for (int n = 0; n < 10; ++n)
	{
		if (ptrMain->Players[n].PlayerActive && !strcmpi(ptrMain->Players[n].Name, name))
		{
			return &ptrMain->Players[n];
		}
	}
	return NULL;
}

PlayerStruct* FindPlayerByDPID(unsigned dpid)
{
	TAdynmemStruct* ptrMain = *(TAdynmemStruct**)0x0511DE8;
	for (int n = 0; n < 10; ++n)
	{
		if (ptrMain->Players[n].PlayerActive && ptrMain->Players[n].DirectPlayID == dpid)
		{
			return &ptrMain->Players[n];
		}
	}
	return NULL;
}

PlayerStruct* FindPlayerByPlayerNum(int playerNum)
{
	TAdynmemStruct* ptrMain = *(TAdynmemStruct**)0x0511DE8;
	for (int n = 0; n < 10; ++n)
	{
		if (ptrMain->Players[n].PlayerActive && ptrMain->Players[n].PlayerNum == playerNum && InferredPlayerTypeIsHuman(&ptrMain->Players[n])) {
			return &ptrMain->Players[n];
		}
	}
	return NULL;
}

PlayerType GetInferredPlayerType(PlayerStruct* p)
{
	if (p->My_PlayerType == Player_LocalHuman) {
		return Player_LocalHuman;
	}
	else if (p->My_PlayerType == Player_LocalAI) {
		return Player_LocalAI;
	}
	else if (p->PlayerInfo->PlayerType == Player_LocalHuman) {
		return Player_RemoteHuman;
	}
	else if (p->PlayerInfo->PlayerType == Player_LocalAI) {
		return Player_RemoteAI;
	}
	else {
		return Player_none;
	}
}

bool InferredPlayerTypeIsLocal(PlayerStruct* p)
{
	return p->My_PlayerType != Player_RemoteHuman && p->My_PlayerType != Player_none;
}

bool InferredPlayerTypeIsHuman(PlayerStruct* p)
{
	return p->PlayerInfo->PlayerType == Player_LocalHuman;
}

_UpdateTeamSelectionButtonIcons UpdateTeamSelectionButtonIcons = (_UpdateTeamSelectionButtonIcons)0x446a50;
_UpdateAlliancesFromTeamSelections UpdateAlliancesFromTeamSelections = (_UpdateAlliancesFromTeamSelections)0x446c70;
_SendMessage_Team24 SendMessage_Team24 = (_SendMessage_Team24)0x452bd0;
_OnTeamChange_BeforeChange_SendMessage_Conditonal_Team23 OnTeamChange_BeforeChange_SendMessage_Conditonal_Team23 = (_OnTeamChange_BeforeChange_SendMessage_Conditonal_Team23)0x446e90;
_InitInternalCommand InitInternalCommand = (_InitInternalCommand)0x4b7760;
_HAPI_SendBuf HAPI_SendBuf = (_HAPI_SendBuf)0x451bc0;
_HAPI_BroadcastMessage HAPI_BroadcastMessage = (_HAPI_BroadcastMessage)0x451df0;
_getFrate getFrate = (_getFrate)0x4B66A0;
DrawTextInScreen_ DrawTextInScreen = (DrawTextInScreen_)0x04C14F0;
_DrawColorTextInScreen DrawColorTextInScreen = (_DrawColorTextInScreen)0x4A50E0;
_GetTextExtent GetTextExtent = (_GetTextExtent)0x4c1480;

//find BeginUnitsArray_p  under mousepointer
_FindMouseUnit FindMouseUnit = (_FindMouseUnit)0x48CD80;

//fill TAdynmem->MouseMapPosX & TAdynmem->MouseMapPosY first

_TAMapClick TAMapClick = (_TAMapClick)0x498F70;

_TestBuildSpot TestBuildSpot = (_TestBuildSpot)0x4197D0;

//Type - 0 = chat, no logo, wraps NewChatText, 1 = popup
_SendText SendText = (_SendText)0x46bc70;

//Send message to player by BroadcastText and echo locally using NewChatText with no logo
_ShowText ShowText = (_ShowText)0x463E50;

//Local chat text with (playerIndex<10) or without (playerIndex=10) logo
_NewChatText NewChatText = (_NewChatText)0x463ca0;

_TADrawRect TADrawRect = (_TADrawRect)0x4BF8C0;

_TADrawLine TADrawLine = (_TADrawLine)0x4CC7AB;				

_GetContext GetContext = (_GetContext)0x4C5E70;

//CirclePointer = CirclePointer in tadynmemstruct
_TADrawCircle TADrawCircle = (_TADrawCircle)0x438EA0;				

_TADrawTransparentBox TADrawTransparentBox = (_TADrawTransparentBox)0x4bf4d0;

_ApplySelectUnitMenu ApplySelectUnitMenu= (_ApplySelectUnitMenu)0x00495860;


_free_SafeWay free_SafeWay=	(_free_SafeWay)0x004B4F20;

_malloc_SafeWay malloc_SafeWay= (_malloc_SafeWay) 0x004B4F10;

_UpdateLOSState UpdateLosState= (_UpdateLOSState) 0x004816A0;
_LoadTARegConfig LoadTARegConfig= (_LoadTARegConfig) 0x00430F00;
_ViewCommandProc ViewCommandProc= (_ViewCommandProc) 0x00416B50;

_SubGUIIndex SubGUIIndex= (_SubGUIIndex)0x0049FDF0 ;

_SetValue_GUI5ID SetValue_GUI5ID= (_SetValue_GUI5ID)0x04A0BF0;
_SubControl_str2ptr SubControl_str2ptr= (_SubControl_str2ptr)0x004A0200;

_IsPressCommand IsPressCommand= (_IsPressCommand) 0x0049FD60;

_CallInternalCommandHandler CallInternalCommandHandler= (_CallInternalCommandHandler)0x0417B50;

_ChangeGameSpeed ChangeGameSpeed= (_ChangeGameSpeed)0x0490DF0;

_Index2Frame_InSequence Index2Frame_InSequence= (_Index2Frame_InSequence )0x04B7F30;
_GAF_DrawTransformed GAF_DrawTransformed = (_GAF_DrawTransformed)0x4c7580;
_CopyGafToContext CopyGafToContext= (_CopyGafToContext)0x04B7F90;

_CheckUnitInPlayerLOS CheckUnitInPlayerLOS= (_CheckUnitInPlayerLOS)0x00465AC0;

_UnitName2ID UnitName2ID= (_UnitName2ID)0x0488B10;

_Order_Move_Ground Order_Move_Ground = (_Order_Move_Ground)0x4031d0;

_MOUSE_EVENT_2UnitOrder_ MOUSE_EVENT_2UnitOrder_= (_MOUSE_EVENT_2UnitOrder_) 0x0048CF30;

_CorretCursor_InGame CorretCursor_InGame= (_CorretCursor_InGame)0x048D220;

_SetUICursor SetUICursor= (_SetUICursor)0x4AB400;

_SelectAllSelectedUnits SelectAllSelectedUnits= (_SelectAllSelectedUnits)0x0048BE00;


_GetPosHeight GetPosHeight= (_GetPosHeight) 0x0485070;
_TARadarDrawCircle TARadarDrawCircle= (_TARadarDrawCircle)0x04C0070;
_GetGridPosFeature GetGridPosFeature= (_GetGridPosFeature)0x0421E60;

_GetPosition_Dword GetPosition_Dword= (_GetPosition_Dword) 0x00484B50;
_TADrawDotteCircle TADrawDotteCircle= (_TADrawDotteCircle) 0x004C01A0;

_ScrollMinimap ScrollMinimap= (_ScrollMinimap )0x041C3C0 ;

                                                     ; 

_PlaySound_Effect PlaySound_Effect= (_PlaySound_Effect) 0x47F1A0;


_GafFrame2OFFScreen GafFrame2OFFScreen= (_GafFrame2OFFScreen) 0x4B8500;

_DrawGameScreen DrawGameScreen_Addr= (_DrawGameScreen)0x0468CF0;


_DrawUnitUI DrawUnitUI= (_DrawUnitUI) 0x04AB170;

_DrawPopupF4Dialog DrawPopupF4Dialog= (_DrawPopupF4Dialog) 0x4948E0;

_DrawPopupButtomDialog DrawPopupButtomDialog= (_DrawPopupButtomDialog) 0x04689C0;

_DrawChatText DrawChatText= (_DrawChatText) 0x0464060;



_CalcUnitTurn CalcUnitTurn= (_CalcUnitTurn) 0x48A490;
                                                     

_IsGUIMem IsGUIMem= (_IsGUIMem) 0x4AB060;

_IntoCurrentUnitGUI IntoCurrentUnitGUI= (_IntoCurrentUnitGUI) 0x0491D70;

_TestGridSpot TestGridSpot = (_TestGridSpot)0x47D2E0;

///--------------------TAMainStruct
TAdynmemStruct * * TAmainStruct_PtrPtr= (TAdynmemStruct * *)0x0511DE8;
TAProgramStruct * * TAProgramStruct_PtrPtr= (TAProgramStruct * *)0x051FBD0;

DWORD Sfx_mallocBufSizeAddr= 0x00471C87;


_GetIniFileInt GetIniFileInt= (_GetIniFileInt) 0x49F5A0;

LPDWORD AISearchMapEntriesLimit= (LPDWORD) 0x0040EAD6;

LPBYTE AddrAboutCircleSelect= (LPBYTE)0x48C599;
LPBYTE AddrUNITINFOInited= (LPBYTE)0x42DB52;

_ApplySelectUnitGUI ApplySelectUnitGUI= (_ApplySelectUnitGUI)0x00495860;

_Init_srand Init_srand= (_Init_srand)0x4B62D0;
_UNITS_CreateUnit UNITS_CreateUnit = (_UNITS_CreateUnit)0x485f50;
_LoadCampaign_UniqueUnits LoadCampaign_UniqueUnits = (_LoadCampaign_UniqueUnits)0x488310;
_Campaign_ParseUnitInitialMissionCommands Campaign_ParseUnitInitialMissionCommands = (_Campaign_ParseUnitInitialMissionCommands)0x487bf0;
_SerialBitArrayRead SerialBitArrayRead = (_SerialBitArrayRead)0x415dc0;
_PacketBuilder_Initialise PacketBuilder_Initialise = (_PacketBuilder_Initialise)0x415b60;
_PacketBuilder_AppendBits PacketBuilder_AppendBits = (_PacketBuilder_AppendBits)0x415c10;
_PacketBuilder_Resize PacketBuilder_Resize = (_PacketBuilder_Resize)0x415bb0;
_PacketBuilder_AssignByteAtOfs PacketBuilder_AssignByteAtOfs = (_PacketBuilder_AssignByteAtOfs)0x415da0;
_InitTAPath InitTAPath = (_InitTAPath)0x4BCE10;
_InitPlayerStruct InitPlayerStruct = (_InitPlayerStruct)0x464700;
_TaCalcCRC CalcCRC = (_TaCalcCRC)0x4b6ba0;
_DPlayAddNewPlayer DPlayAddNewPlayer = (_DPlayAddNewPlayer)0x451220;
_battleroom_OnCommand battleroom_OnCommand = (_battleroom_OnCommand)0x447b10;

LPCSTR TAIniStr=  reinterpret_cast<LPCSTR> (0x5098A3);

LPCSTR CompanyName_CCSTR= (LPCSTR)0x50DDF4;
LPCSTR GameName_CCSTR= (LPCSTR) 0x5032E8;

unsigned int EnterOption_Address = 0x0460160;

unsigned int PressInOption_Address= 0x0045FC60;

unsigned int AddtionInitAddr= 0x0049E909;
unsigned int AddtionInitAfterDDrawAddr= 0x049E9A0;

unsigned int Blt_BottomState0_TextRtn= 0x004C16D9 ;
unsigned int Blt_BottomState0_TextAddr= 0x004C16D4 ;
unsigned int Blt_BottomState1_TextRtn= 0x004C1749 ;
unsigned int Blt_BottomState1_TextAddr= 0x004C1744 ;

unsigned int GetTextExtent_AssignCharLenAddr= 0x004C1480;
unsigned int GetStrExtentAddr= 0x004A5030;

unsigned int PopadStateAddr= 0x0468AC0;


unsigned int Addr_0049E91C= 0x0049E91C;
unsigned int Addr_0049E93B= 0x0049E93B;
unsigned int Addr_00491A75= 0x00491A75;
unsigned int Addr_00491B01= 0x00491B01;
unsigned int Addr_0049802B= 0x0049802B;
unsigned int Addr_004980AD= 0x004980AD;


unsigned int TADontInit_Addr= 0x4B5AB6;

unsigned int MPUnitLimitAddr= 0x0044CAFE;
unsigned int UnitLimit0Addr= 0x491640;
unsigned int UnitLimit1Addr=0x491659;
unsigned int UnitLimit2Addr=0x491666;

unsigned int BadModelHunterAddr= 0x00458C5A;
unsigned int SafeModelAddr= 0x4596EB; //0x458D13;

BOOL * IsCheating= reinterpret_cast<BOOL *>(0x5091CC);



COBHandle * * COBSciptHandler_Begin= (COBHandle  * *)0x0512344;
COBHandle *  * COBSciptHandler_End= (COBHandle  * *) 0x0512348;

unsigned int KeepActiveAddr= 0x4AB111;

unsigned int KeepActiveAddr1= 0x4AB13F;

unsigned int DrawGameScreenEnd_Addr= 0x046A3CC;

unsigned int LoadMap_Addr=  0x00483638;

unsigned int DrawTAScreenBlitAddr= 0x0046A3CE;

WNDPROC TAWndProc_Addr= (WNDPROC )0x4B5CC0;



unsigned int TAWndProcSH_Addr=0x4B5B82;