#ifndef tafunctionsH
#define tafunctionsH

#include "tamem.h"

class InlineSingleHook;
struct msgstruct{
	int xpos;
	int ypos;
	int shiftstatus; //should be 5 for shiftclick
};

struct posstruct{
	int x;
	int y;
};

struct _GAFFrame;


//////////////////////////////////////////////////////////////////////////////////////////
/// Working.
//////////////////////////////////////////////////////////////////////////////////////////

typedef void (*_UpdateTeamSelectionButtonIcons)();
extern _UpdateTeamSelectionButtonIcons UpdateTeamSelectionButtonIcons;

typedef void (*_UpdateAlliancesFromTeamSelections)();
extern _UpdateAlliancesFromTeamSelections UpdateAlliancesFromTeamSelections;

typedef int (__stdcall* _SendMessage_Team24)(PlayerStruct* p_Player);
extern _SendMessage_Team24 SendMessage_Team24;

typedef int(__stdcall* _OnTeamChange_BeforeChange_SendMessage_Conditonal_Team23)(PlayerStruct* player);
extern _OnTeamChange_BeforeChange_SendMessage_Conditonal_Team23 OnTeamChange_BeforeChange_SendMessage_Conditonal_Team23;

/// @param commandTable last entry has empty InternalCommandTableEntryStruct::name
typedef int(__stdcall* _InitInternalCommand)(const InternalCommandTableEntryStruct *commandTable);
extern _InitInternalCommand InitInternalCommand;

typedef int(__stdcall* _HAPI_SendBuf)(unsigned FromPID, unsigned ToPID, const char* Buffer, int BufferSize);
extern _HAPI_SendBuf HAPI_SendBuf;

typedef int(__stdcall* _HAPI_BroadcastMessage)(int FromPID, const char* Buffer, int BufferSize);
extern _HAPI_BroadcastMessage HAPI_BroadcastMessage;

typedef int(__stdcall* _getFrate)();
extern _getFrate getFrate;
typedef int(__stdcall* DrawTextInScreen_)(OFFSCREEN* offscreen_p, char* text, int xOff, int yOff, int length);
extern DrawTextInScreen_ DrawTextInScreen;
typedef bool(__stdcall* _DrawColorTextInScreen)(OFFSCREEN* offscreen_p, const char* text, int x_off, int y_off, int max_x_pixels, int color_);
extern _DrawColorTextInScreen DrawColorTextInScreen;
typedef int(__stdcall* _GetTextExtent)(void* fontHandle, const char* str);
extern _GetTextExtent GetTextExtent;
typedef unsigned short (__stdcall *_FindMouseUnit)(void); //find BeginUnitsArray_p  under mousepointer
extern _FindMouseUnit FindMouseUnit;
//fill TAdynmem->MouseMapPosX & TAdynmem->MouseMapPosY first
typedef void (__stdcall *_TAMapClick)(void *msgstruct);
extern _TAMapClick TAMapClick;
typedef void (__stdcall *_TestBuildSpot)(void);
extern _TestBuildSpot TestBuildSpot;
//Type - 0 = chat, 1 = popup
typedef int (__stdcall *_SendText)(const char *Text, int Type);
extern _SendText SendText;
typedef void (__stdcall *_ShowText)(PlayerStruct *Player, char *Text, int Unk1, int Unk2);
extern _ShowText ShowText;
typedef void(__stdcall* _NewChatText)(char* Text, char fontColor /* 1,2,4,8,16,32 but no practical effect?*/, short unkZero, char playerIndex);
extern _NewChatText NewChatText;
typedef void (__stdcall *_TADrawRect)(OFFSCREEN * Context, tagRECT *rect, int color);
extern _TADrawRect TADrawRect;
typedef void (__cdecl *_TADrawLine)(char *Context, int x1,int y1,int x2,int y2,int color);
extern _TADrawLine TADrawLine; 

typedef int (__stdcall *_GetContext)(char *ptr);
extern _GetContext GetContext;

//CirclePointer = CirclePointer in tadynmemstruct
typedef void (__stdcall *_TADrawCircle)(OFFSCREEN * context, void *CirclePointer, Position_Dword *pos, int radius, int color, char *text, LPSTR comment);
extern _TADrawCircle TADrawCircle;

typedef int(__stdcall* _TADrawTransparentBox)(OFFSCREEN* context, RECT *box, int arg8);
extern _TADrawTransparentBox TADrawTransparentBox;

typedef void (__stdcall * _ApplySelectUnitMenu) (void);
extern _ApplySelectUnitMenu ApplySelectUnitMenu;
typedef void (__cdecl * _free_SafeWay) (LPVOID MemAddress);
extern _free_SafeWay free_SafeWay;
typedef void* (__cdecl * _malloc_SafeWay) (int MemSize_I);
extern _malloc_SafeWay malloc_SafeWay;
typedef void (__stdcall * _UpdateLOSState) (int Update_Type);
extern _UpdateLOSState UpdateLosState;

typedef BOOL (__stdcall * _LoadTARegConfig) (void);
extern _LoadTARegConfig LoadTARegConfig;

typedef int (__stdcall * _ViewCommandProc) (char * );//this is spec struct, but I'm lazy to define it.
extern _ViewCommandProc ViewCommandProc;

typedef int (__stdcall * _SubGUIIndex)(GUI0IDControl * GUIControl_p, char *SubControlName, int _0xe);
extern _SubGUIIndex SubGUIIndex;

typedef int (__stdcall * _SubControl_str2ptr)(GUI0IDControl * GUIINFO_P, char *ControlName);
extern _SubControl_str2ptr SubControl_str2ptr;

typedef int (__stdcall * _SetValue_GUI5ID)(GUIInfo * GUIINFO_P, char *ControlName, char * NewStr, int _zero);
extern _SetValue_GUI5ID SetValue_GUI5ID;

typedef int (__stdcall * _IsPressCommand)(GUIInfo * TAUI_p, char * ControlName);
extern _IsPressCommand IsPressCommand;

typedef int (__stdcall * _CallInternalCommandHandler)(const char *Command_ptr, int CommandLevel);
extern _CallInternalCommandHandler CallInternalCommandHandler;

typedef int (__stdcall *_ChangeGameSpeed)(int NewSpeed, int TellOther_B);
extern _ChangeGameSpeed ChangeGameSpeed;

typedef _GAFFrame * (__stdcall * _Index2Frame_InSequence)(_GAFSequence * ParsedGaf, int index);
extern _Index2Frame_InSequence  Index2Frame_InSequence;

typedef void(__stdcall* _GAF_DrawTransformed)(_OFFSCREEN *, _GAFFrame *, POINT destCorners[4], POINT sourceCorners[4]);
extern _GAF_DrawTransformed GAF_DrawTransformed;
 
typedef	int (__stdcall * _CopyGafToContext)(_OFFSCREEN * OFFSCREN_ptr, _GAFFrame * GafFrame, int Off_X, int Off_Y);
extern _CopyGafToContext CopyGafToContext;

typedef int (__stdcall * _CheckUnitInPlayerLOS)(PlayerStruct * Player_Ptr, UnitStruct * UnitsInGame_Ptr);
extern _CheckUnitInPlayerLOS CheckUnitInPlayerLOS;

                                                
typedef int (__stdcall * _UnitName2ID)(char * Str1);
extern _UnitName2ID UnitName2ID;

typedef int(__stdcall* _Order_Move_Ground)(UnitStruct* callerUnit, UnitOrdersStruct* orderUnit, int lastState);
extern _Order_Move_Ground Order_Move_Ground;

typedef int (__stdcall * _MOUSE_EVENT_2UnitOrder_) (_MOUSEEVENT * MouseEvent_ptr, int ActionType, unsigned char ActionIndex, _Position_Dword * Position_DWORD_p, int unk, int unk1);
extern _MOUSE_EVENT_2UnitOrder_ MOUSE_EVENT_2UnitOrder_;

typedef int (__stdcall * _CorretCursor_InGame)(char PrepareOrder);
extern _CorretCursor_InGame CorretCursor_InGame;

typedef int (__stdcall * _SetUICursor)(GUIInfo * TAGUIInfo, _GAFSequence * CursorGafSqe_Ptr);
extern _SetUICursor SetUICursor;
typedef int (__cdecl * _SelectAllSelectedUnits)(void);
extern _SelectAllSelectedUnits SelectAllSelectedUnits;




typedef unsigned short ( __stdcall *_GetGridPosFeature)(FeatureStruct *);
extern _GetGridPosFeature GetGridPosFeature;


typedef int (__stdcall * _TARadarDrawCircle) (_OFFSCREEN * OFFSCREEN_p, int CenterX, int CenterY, int Radius, int color);
extern _TARadarDrawCircle TARadarDrawCircle;



typedef int (__stdcall * _TADrawDotteCircle)(_OFFSCREEN * OFFSCREEN_p, int CenterX, int CenterY, int Radius, int color, int Spacing, int Dotte_b);
extern _TADrawDotteCircle TADrawDotteCircle;

typedef int (__stdcall * _GetPosition_Dword)(int X, int Y, Position_Dword * out_p);
extern _GetPosition_Dword GetPosition_Dword;

typedef int (__stdcall* _GetPosHeight)(Position_Dword * Pos);
extern _GetPosHeight GetPosHeight;

typedef int (__stdcall *_ScrollMinimap)(void);
extern _ScrollMinimap ScrollMinimap;

typedef int (__stdcall * _PlaySound_Effect)(char *VoiceName, int);
extern _PlaySound_Effect PlaySound_Effect;

  
typedef int (__stdcall * _GafFrame2OFFScreen)(OFFSCREEN * Offscreen_p, _GAFFrame * GafFrame_p, int X, int Y);
extern _GafFrame2OFFScreen GafFrame2OFFScreen;

typedef int (__stdcall * _DrawGameScreen)(int IsDrawObject, int IsBlitScreen);
extern _DrawGameScreen DrawGameScreen_Addr;

typedef int (__stdcall* _DrawUnitUI)(GUIInfo * TAGUISummy_ptr, OFFSCREEN * offscreen, RECT * lprect);
extern _DrawUnitUI DrawUnitUI;


                    
typedef int (__stdcall * _DrawPopupF4Dialog)(OFFSCREEN * OFFSCREN_ptr);
extern _DrawPopupF4Dialog DrawPopupF4Dialog;


                                                     
typedef int (__stdcall* _DrawPopupButtomDialog)(OFFSCREEN * OFFSCREN_ptr);
extern _DrawPopupButtomDialog DrawPopupButtomDialog;


typedef int (__stdcall * _DrawChatText)(OFFSCREEN * Offscreen_p);
extern _DrawChatText DrawChatText;

typedef int (__stdcall *_CalcUnitTurn)(UnitStruct * Unit_p);
extern _CalcUnitTurn CalcUnitTurn;

typedef BOOL (__stdcall * _IsGUIMem)(GUIInfo * GUIInfo_p, char * GUIName);
extern _IsGUIMem IsGUIMem;

                                                     
typedef int (__stdcall * _IntoCurrentUnitGUI)(BOOL UpdateGUI_Bool);
extern _IntoCurrentUnitGUI IntoCurrentUnitGUI;

typedef int (__stdcall *_TestGridSpot)(UnitDefStruct *BuildUnit, unsigned packedMousePositionXY, int unk, PlayerStruct *Player); //unk=zero
extern _TestGridSpot TestGridSpot;


////-----------
typedef int (__stdcall * _GetIniFileInt) (LPCSTR lpKeyName, INT nDefault);
extern _GetIniFileInt GetIniFileInt;

typedef void (__stdcall * _ApplySelectUnitGUI) (void);
typedef void (__cdecl * _free_SafeWay) (LPVOID MemAddress);
typedef LPDWORD (__stdcall * __GetUnitIDMaskAryByCategory) (LPSTR);

typedef int  (__cdecl * _InitTAPath) (void);
extern _InitTAPath InitTAPath;

typedef int(__stdcall* _InitPlayerStruct) (PlayerStruct* PlayerPtr);
extern _InitPlayerStruct InitPlayerStruct;

typedef int (__stdcall * _Init_srand)(int seed);
extern _Init_srand Init_srand;

typedef UnitStruct*(__stdcall* _UNITS_CreateUnit)(
	int playerIndex, int unitInfoId, int posX, int posZ, int posY, bool fullHp, unsigned unitStateMask, int unitId);
extern _UNITS_CreateUnit UNITS_CreateUnit;

typedef int(__stdcall* _LoadCampaign_UniqueUnits)(void);
extern _LoadCampaign_UniqueUnits LoadCampaign_UniqueUnits;

typedef char(__stdcall* _Campaign_ParseUnitInitialMissionCommands)(UnitStruct* UnitPtr, const char* Source, void* outUniqueUnitID);
extern _Campaign_ParseUnitInitialMissionCommands Campaign_ParseUnitInitialMissionCommands;

typedef int(__fastcall* _SerialBitArrayRead)(SerialBitArrayStruct* serialBitArray, int ignored, int nBitsToRead);
extern _SerialBitArrayRead SerialBitArrayRead;

typedef int(__fastcall* _PacketBuilder_Initialise)(PacketBuilderStruct* pb, int ignored);
extern _PacketBuilder_Initialise PacketBuilder_Initialise;

typedef int(__fastcall* _PacketBuilder_AppendBits)(PacketBuilderStruct* pb, int ignored, int value, int bitsToAdd);
extern _PacketBuilder_AppendBits PacketBuilder_AppendBits;

typedef int(__fastcall* _PacketBuilder_Resize)(PacketBuilderStruct* pb, int ignored);
extern _PacketBuilder_Resize PacketBuilder_Resize;

typedef int(__fastcall* _PacketBuilder_AssignByteAtOfs)(PacketBuilderStruct* pb, int ignored, int byte_ofs, char byte_value);
extern _PacketBuilder_AssignByteAtOfs PacketBuilder_AssignByteAtOfs;

typedef int(__stdcall* _TaCalcCRC)(unsigned char* data, int len);
extern _TaCalcCRC TaCalcCRC;

typedef void(__stdcall* _DPlayAddNewPlayer)(int newPlayerIndex, PlayerType playerType);
extern _DPlayAddNewPlayer DPlayAddNewPlayer;

typedef void(__stdcall* _battleroom_OnCommand)(GUIInfo *guiInfo);
extern _battleroom_OnCommand battleroom_OnCommand;

int ViewPlayerLos_Replay (int PlayerAryIndex, BOOL HaveControl= FALSE);
int UpdateTAProcess (void);
void SendOrder (unsigned int TAX, unsigned int TAY, unsigned int TAZ, int OrderType, bool Shift);
void SendOrder(UnitStruct* unit, unsigned int TAX, unsigned int TAY, unsigned int TAZ, UnitStruct* target, ordertype::ORDERTYPE orderType, int orCobIndex, bool shift);
void PushOrder(UnitStruct* unit, unsigned int TAX, unsigned int TAY, unsigned int TAZ, UnitStruct* target, ordertype::ORDERTYPE orderType);
void DeselectUnits(void);
void freeTAMem (LPVOID MemAddress);
LPDWORD GetUnitIDMaskAryByCategory (LPSTR CategoryName_cstrp);
void UpdateSelectUnitEffect (void);
void ApplySelectUnitMenu_Wapper (void);
int ChatText (LPCSTR str);
bool SetIDMaskInTypeAry (WORD ID, DWORD SelectedUnitTypeIDAry_Dw[]);
bool CleanIDMaskInTypeAry (WORD ID, DWORD SelectedUnitTypeIDAry_Dw[]);
bool MatchInTypeAry (WORD ID, DWORD SelectedUnitTypeIDAry_Dw[]);
int GetMaxScrollY();
int GetMaxScrollX();
unsigned char GetPlayerDotColor(int Player);	// 0..255
void ScrollToCenter(int x, int y);

int CountSelectedUnits (void);
int PauseCDMusic();

int DrawRadarCircle (LPBYTE Bits, POINT * Aspect, int CenterX, int CenterY, int Radius, int color);
int DrawDotteCircle (LPBYTE Bits, POINT * Aspect, int CenterX, int CenterY, int Radius, int color, int Spacing, int Dotte_b);

BOOL IsPlayerAllyUnit (int  UnitID,int PlayerLosID);

void GetWeatherReport(int& _solar, int& windPower, int& windPowerMin, int& windPowerMax, int& tidalPower);

PlayerStruct* FindPlayerByName(const char* name);
PlayerStruct* FindPlayerByDPID(unsigned dpid);
PlayerStruct* FindPlayerByPlayerNum(int playerNum);

PlayerType GetInferredPlayerType(PlayerStruct* p);
bool InferredPlayerTypeIsLocal(PlayerStruct* p);
bool InferredPlayerTypeIsHuman(PlayerStruct* p);

extern TAProgramStruct * * TAProgramStruct_PtrPtr;

extern TAdynmemStruct * * TAmainStruct_PtrPtr;

extern LPBYTE AddrAboutCircleSelect;
extern LPBYTE AddrUNITINFOInited;

extern LPDWORD AISearchMapEntriesLimit;
extern DWORD Sfx_mallocBufSizeAddr;

extern LPCSTR CompanyName_CCSTR;
extern LPCSTR GameName_CCSTR;

extern unsigned int EnterOption_Address;
extern unsigned int PressInOption_Address;
extern unsigned int AddtionInitAddr;
extern unsigned int AddtionInitAfterDDrawAddr;

extern unsigned int Blt_BottomState0_TextRtn;
extern unsigned int Blt_BottomState0_TextAddr;

extern unsigned int Blt_BottomState1_TextRtn;
extern unsigned int Blt_BottomState1_TextAddr;

extern unsigned int PopadStateAddr;
//////////////////////////////////////////////////////////////////////////
extern InlineSingleHook * AddtionInitHook;
extern unsigned int GetTextExtent_AssignCharLenAddr;
extern unsigned int GetStrExtentAddr;

extern unsigned int Addr_0049E91C;
extern unsigned int Addr_0049E93B;
extern unsigned int Addr_00491A75;
extern unsigned int Addr_00491B01;
extern unsigned int Addr_0049802B;
extern unsigned int Addr_004980AD;

extern unsigned int TADontInit_Addr;

extern unsigned int MPUnitLimitAddr;
extern unsigned int UnitLimit0Addr;
extern unsigned int UnitLimit1Addr;
extern unsigned int UnitLimit2Addr;

extern unsigned int BadModelHunterAddr;
extern unsigned int SafeModelAddr;


extern BOOL * IsCheating;                        
extern WNDPROC TAWndProc_Addr;
extern unsigned int TAWndProcSH_Addr;


extern COBHandle *  * COBSciptHandler_Begin;
extern COBHandle *  * COBSciptHandler_End;

extern unsigned int KeepActiveAddr;
extern unsigned int KeepActiveAddr1;

extern unsigned int DrawGameScreenEnd_Addr;

extern unsigned int LoadMap_Addr;
extern unsigned int DrawTAScreenBlitAddr;
#endif
