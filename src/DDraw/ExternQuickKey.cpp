//扩展的快捷键

#include "oddraw.h"
#include "iddrawsurface.h"
#include "tamem.h"
#include "tafunctions.h"
#include "hook\hook.h"
#include "hook\etc.h"
#include "ExternQuickKey.h"
#include "WeaponIDLimit.h"
#include "tahook.h"
#include <vector>
using namespace std;
#include "TAConfig.h"
#include "LimitCrack.h"
#include "UnitTypeLimit.h"

#include "UnitDrawer.h"
#include "fullscreenminimap.h"
#include "GUIExpand.h"



ExternQuickKey * myExternQuickKey;

///////--------------

ExternQuickKey::ExternQuickKey ()
{
	TAMainStruct_Ptr= * TAmainStruct_PtrPtr;
	 
	//AddtionInit ( );
	Semaphore_OnlyInScreenSameType= CreateSemaphore ( NULL, 1, 1, NULL);
	Semaphore_FilterSelect= CreateSemaphore ( NULL, 1, 1, NULL);
	Semaphore_OnlyInScreenWeapon= CreateSemaphore ( NULL, 1, 1, NULL);
	Semaphore_IdleCons= CreateSemaphore ( NULL, 1, 1, NULL);
	Semaphore_IdleFac=  CreateSemaphore ( NULL, 1, 1, NULL);

	DoubleClick= MyConfig->GetIniBool ( "DoubleClick", TRUE);

	 CommanderMask= NULL;
	 MobileWeaponMask= NULL;
	 ConstructorMask= NULL;
	 FactoryMask= NULL;
	 BuildingMask= NULL;
	 AirWeaponMask= NULL;
	 AirConMask= NULL;
	 for (int i= 0; i<RACENUMBER; ++i)
	 {
		 Commanders[i][0]= '\0';

		 CommandersMask[i]= NULL;
	 }

	HookInCircleSelect= new InlineSingleHook ( (unsigned int)AddrAboutCircleSelect, 5, 
		INLINE_5BYTESLAGGERJMP, AddtionRoutine_CircleSelect);
	HookInCircleSelect->SetParamOfHook ( (LPVOID)this);
	HookInUNITINFOInited= new InlineSingleHook ( (unsigned int)AddrUNITINFOInited, 5, 
		INLINE_5BYTESLAGGERJMP, AddtionRoutine_UnitINFOInit);
	HookInUNITINFOInited->SetParamOfHook ( (LPVOID)this);
	
	HKEY hKey;
	HKEY hKey1;
	DWORD dwDisposition;
	DWORD Size;

	RegCreateKeyEx(HKEY_CURRENT_USER, MyConfig->ModRegistryName.c_str(), NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	RegCreateKeyEx(hKey, "Eye", NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey1, &dwDisposition);

	Size = sizeof(int);
	if(RegQueryValueEx(hKey1, "KeyCode", NULL, NULL, (unsigned char*)&VirtualKeyCode, &Size) != ERROR_SUCCESS)
	{
		VirtualKeyCode = 88;
	}

	RegCloseKey ( hKey);
	RegCloseKey ( hKey1);

	Add = (char*)0x41ac14;
	Sub = (char*)0x41ac18;

	OldAdd = *Add;
	OldSub = *Sub;
	NumAdd = 100;
	NumSub = -100;

	IDDrawSurface::OutptTxt ( "New ExternQuickKey");
}


ExternQuickKey::~ExternQuickKey ()
{
	//AddtionRelease ( );
	if (NULL!=Semaphore_OnlyInScreenSameType)
	{
		CloseHandle ( Semaphore_OnlyInScreenSameType);
	}
	if (NULL!=Semaphore_FilterSelect)
	{
		CloseHandle ( Semaphore_FilterSelect);
	}
	if (NULL!=Semaphore_OnlyInScreenWeapon)
	{
		CloseHandle ( Semaphore_OnlyInScreenWeapon);
	}

	if (NULL!=Semaphore_IdleCons)
	{
		CloseHandle ( Semaphore_IdleCons);
	}
	if (NULL!=Semaphore_IdleFac)
	{
		CloseHandle ( Semaphore_IdleFac);
	}

	
	if (NULL!=HookInCircleSelect)
	{
		delete HookInCircleSelect;
	}

	if (NULL!=HookInUNITINFOInited)
	{
		delete HookInUNITINFOInited;
	}
	
	WriteProcessMemory ( GetCurrentProcess(), (void*)Add, &OldAdd, 1, NULL);
	WriteProcessMemory ( GetCurrentProcess(), (void*)Sub, &OldSub, 1, NULL);

	DestroyExternTypeMask ( );
}

bool ExternQuickKey::Message(HWND WinProcWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (TAInGame!=DataShare->TAProgress)
	{
		return false;
	}
	__try
	{
		switch (Msg)
		{
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:

			if (! DoubleClick)
			{
				break;
			}

// 			if (0==(TAMainStruct_Ptr->InterfaceType))
// 			{
// 				if (Msg==WM_RBUTTONDBLCLK)
// 				{
// 					break;
// 				}
// 			}
// 			else
// 			{
// // 				if (Msg==WM_LBUTTONDBLCLK)
// // 				{
// // 					break;
// // 				}
// 			}
#ifdef USEMEGAMAP

			if (GUIExpander)
			{
				if (GUIExpander->myMinimap)
				{
					if (GUIExpander->myMinimap->IsBliting())
					{
						break;
					}
				}
			}
#endif
			if ((GetAsyncKeyState(VirtualKeyCode)&0x8000)==0)
			{// don't catch the msg when whiteboard key down.
				int xPos;
				xPos= LOWORD(lParam);
				int yPos;
				yPos = HIWORD(lParam);
				if ((xPos>(TAMainStruct_Ptr->GameSreen_Rect.left))&&(xPos<(TAMainStruct_Ptr->GameSreen_Rect.right))&&(yPos>(TAMainStruct_Ptr->GameSreen_Rect.top))&&(yPos<(TAMainStruct_Ptr->GameSreen_Rect.bottom)))
				{
					if (0!=TAMainStruct_Ptr->MouseOverUnit)
					{
						if (TAMainStruct_Ptr->Players[TAMainStruct_Ptr->LocalHumanPlayer_PlayerID].PlayerAryIndex==(TAMainStruct_Ptr->BeginUnitsArray_p [TAMainStruct_Ptr->MouseOverUnit].Owner_PlayerPtr0->PlayerAryIndex))
						{
							SelectOnlyInScreenSameTypeUnit ( FALSE);
							UpdateSelectUnitEffect ( ) ;
							ApplySelectUnitMenu_Wapper ( );
							return true;
						}
					}
				}
			}
			break;
		case WM_KEYDOWN:
			if ((int)wParam==17)
			{
				WriteProcessMemory(GetCurrentProcess(), (void*)Add, &NumAdd, 1, NULL);
				WriteProcessMemory(GetCurrentProcess(), (void*)Sub, &NumSub, 1, NULL);
				//break;
			}

			if (0x53==(int)wParam)
			{//ctrl+s
				if ((GetAsyncKeyState(VK_CONTROL)&0x8000)>0 && (GetAsyncKeyState(VK_SHIFT)&0x8000)==0)
				{

					SelectOnlyInScreenWeaponUnit ( MOVEUNITSELECTABLE);
					UpdateSelectUnitEffect ( ) ;
					ApplySelectUnitMenu_Wapper ( );
					return true;
				}
			}
			if(wParam == 66 && (GetAsyncKeyState(17)&0x8000)>0 && (GetAsyncKeyState(16)&0x8000)==0) 
			{// ctrl + b
				DeselectUnits ();
				FindIdleConst();
				UpdateSelectUnitEffect ( ) ;
				ApplySelectUnitMenu_Wapper ( );
				return true;
			}
			/*
			if(wParam == 70  && (GetAsyncKeyState(17)&0x8000)>0 && (GetAsyncKeyState(16)&0x8000)==0) 
			{// ctrl + f
				DeselectUnits ();
				FindIdelFactory ();
				UpdateSelectUnitEffect ( ) ;
				ApplySelectUnitMenu_Wapper ( );
				return true;
			} */
			
			break;
		case WM_KEYUP:
			switch((int)wParam)
			{
			case 17:
				WriteProcessMemory(GetCurrentProcess(), (void*)Add, &OldAdd, 1, NULL);
				WriteProcessMemory(GetCurrentProcess(), (void*)Sub, &OldSub, 1, NULL);
				//break;
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		;
	}

	return false;
}

void ExternQuickKey::FindIdelFactory ()
{
	DWORD Wait_rtn= WaitForSingleObject ( Semaphore_IdleFac, INFINITE);
	if (WAIT_FAILED==Wait_rtn)
	{
		return ;
	}

	if (WAIT_TIMEOUT==Wait_rtn)
	{
		ReleaseSemaphore ( Semaphore_IdleFac, 1, NULL);
		return ;
	}

	static int LastNum = 0;
reTry:
	TAdynmemStruct *PTR = TAMainStruct_Ptr;

	UnitStruct *  Begin=  PTR->BeginUnitsArray_p ;
/*	UnitStruct *  End= PTR->Players[PTR->LocalHumanPlayer_PlayerID].UnitsAry_End;*/
	UnitStruct *  Current= Begin;


	int j= LastNum;
	int MyMaxUnit= PTR->Players[PTR->LocalHumanPlayer_PlayerID].UnitsNumber;

	while (j<=MyMaxUnit)
	{

		Current= &(PTR->Players[PTR->LocalHumanPlayer_PlayerID].Units[j]);

		if ((0!=((UnitValid2_State)& Current->UnitSelected)))
		{
			if (0.0F==(Current->Nanoframe))
			{
				//ID= ;
				if ((NULL!=Current->Owner_PlayerPtr1))//&&(Player_LocalHuman==Current->ValidOwner_PlayerPtr->My_PlayerType))
				{
					if (MatchInTypeAry ( Current->UnitID, FactoryMask))
					{
						if (NULL!=Current->UnitOrders)
						{
							char UnitState= Current->UnitOrders->COBHandler_index;
							if((UnitState==0xc)) //not idle
							{
								++j;
								continue;
							}
						}
						//i= i+ 1;
						if (LastNum<j)
						{
							LastNum = j;
							break;
						}
					}
				}
			}
		}
		//Current= &Current[1];
		++j;
	}
	if (j<=MyMaxUnit)
	{
		//founded!
		Current->UnitSelected|=  UnitSelected_State;

		ScrollToCenter( Current->XPos,  Current->YPos);
	}
	else
	{
		// not found once
		if (0!=LastNum)
		{
			LastNum= 0;
			goto reTry;
		}
	}
	//really not found 


	ReleaseSemaphore ( Semaphore_IdleFac, 1, NULL);
	return ;
}

void ExternQuickKey::FindIdleConst()
{
	//FixAck();
	//

	IDDrawSurface::OutptTxt ( "Search Idle Const");

	DWORD Wait_rtn= WaitForSingleObject ( Semaphore_IdleCons, INFINITE);
	if (WAIT_FAILED==Wait_rtn)
	{
		return ;
	}
		
	if (WAIT_TIMEOUT==Wait_rtn)
	{
		goto ReleaseIdleConsSemaphore;
	}

	static int LastNum = 0;

	TAdynmemStruct *PTR = TAMainStruct_Ptr;
	UnitStruct * Start;//

	int i;
	int MyMaxUnit= PTR->Players[PTR->LocalHumanPlayer_PlayerID].UnitsNumber;

	if (MyMaxUnit<LastNum)
	{
		LastNum= 0;
	}
	i= LastNum;

	while (i<=MyMaxUnit)
	{
		Start= &(PTR->Players[PTR->LocalHumanPlayer_PlayerID].Units[i]);

		char *UnitDead = (char*)(&Start->HealthPerB);
		short *XPos = (short*)(&Start->XPos);
		short *YPos = (short*)(&Start->YPos);
		int *IsUnit = (int*)(&Start->IsUnit);
		char *UnitSelected = (char*)(&Start->UnitSelected);

		int *UnitOrderPTR = (int*)(&Start->UnitOrders);

		if(*UnitDead!=0 && *UnitDead!=1)
		{
			if(*IsUnit)
			{
				if (ConstructorMask
					&&((MatchInTypeAry ( Start->UnitID, ConstructorMask))))
				{

					char *UnitState = (char*)(*UnitOrderPTR + 4);
					
					if((NULL==*UnitOrderPTR)
						||(*UnitState==41 || *UnitState==64)) //idle
					{
						if  (LastNum<i)
						{
							LastNum = i;

							*UnitSelected|= UnitSelected_State;

							ScrollToCenter(*XPos, *YPos);
							goto ReleaseIdleConsSemaphore;
						}

					}
				}
			}
		}
		++i;

		if(MyMaxUnit<i)
		{
			if(LastNum == 0) //no units found and all units searched. this is 2rd time enter FindIdleConst;
			{
				goto ReleaseIdleConsSemaphore;
			}
			break;
		}
		//Start= &Start[1];
	}

	//search from the beginning, cause last num be reset at this case
	LastNum = 0;

	ReleaseSemaphore ( Semaphore_IdleCons, 1, NULL);
	FindIdleConst();
	return;
ReleaseIdleConsSemaphore:
	ReleaseSemaphore ( Semaphore_IdleCons, 1, NULL);
	return;
}



int ExternQuickKey::SelectOnlyInScreenSameTypeUnit (BOOL FirstSelect_Flag)
{// FirstSelect mean if already selected  in screen same type units, then will select all same units ever not in screen
//  return Selected units number
	WaitForSingleObject ( Semaphore_OnlyInScreenSameType, INFINITE);
	
	LPDWORD SelectedUnitTypeIDAry_Dw= new DWORD[CategroyMaskSize/ 4];
	memset ( SelectedUnitTypeIDAry_Dw, 0, CategroyMaskSize);

	WORD ID= 0;
	TAdynmemStruct *PTR = TAMainStruct_Ptr;
	UnitStruct * Begin= PTR->Players[PTR->LocalHumanPlayer_PlayerID].Units;
	UnitStruct * End= PTR->Players[PTR->LocalHumanPlayer_PlayerID].UnitsAry_End;

	while (Begin<=End)
	{
		if (0!=((Begin->UnitSelected)& 0x10))
		{//this one are Selected BeginUnitsArray_p 
			SetIDMaskInTypeAry ( Begin->UnitID, SelectedUnitTypeIDAry_Dw);
		}
		Begin= &Begin[1];
	}
	DeselectUnits();
	// now we got all selected BeginUnitsArray_p  type
	Begin=  PTR->BeginUnitsArray_p ;
	UnitStruct *  Current;
	int MaxHotUnitCount_I= PTR->NumHotUnits;
	int Counter= 0;
	short int * HotUnitAry_Ptr= PTR->HotUnits;
	int SelectedUnits= 0;
	while (Counter<MaxHotUnitCount_I)
	{
		Current= &(Begin[HotUnitAry_Ptr[Counter]]);

		if (0!=(0x20& Current->UnitSelected))
		{
			if (0.0F==(Current->Nanoframe))
			{
				//ID= ;
				if (MatchInTypeAry ( Current->UnitID, SelectedUnitTypeIDAry_Dw))
				{
					if ((NULL!=Current->Owner_PlayerPtr1))
					{
						if (TAMainStruct_Ptr->LocalHumanPlayer_PlayerID==Current->Owner_PlayerPtr1->PlayerAryIndex)
						{
							Current->UnitSelected= (Current->UnitSelected)| (0x10);
							++SelectedUnits;
						}

					}
				}
			}
		}
		++Counter;
	}

	UpdateSelectUnitEffect ( ) ;
	ApplySelectUnitMenu_Wapper ( );

	delete [] SelectedUnitTypeIDAry_Dw;
	ReleaseSemaphore ( Semaphore_OnlyInScreenSameType, 1, NULL);
	return SelectedUnits;
}


int ExternQuickKey::SelectOnlyInScreenWeaponUnit (unsigned int SelectWay_Mask)
{
	WaitForSingleObject ( Semaphore_OnlyInScreenWeapon, INFINITE);

	int SelectedUnits= 0;
	bool DoSelect_b= false;
	if (MOVEUNITSELECTABLE==SelectWay_Mask)
	{
		DeselectUnits ( );

		TAdynmemStruct *PTR = TAMainStruct_Ptr;
		
		UnitStruct * Begin= PTR->BeginUnitsArray_p ;
		UnitStruct *  Current;
		int MaxHotUnitCount_I= PTR->NumHotUnits;
		int Counter= 0;
		short int * HotUnitAry_Ptr= PTR->HotUnits;
		
		while (Counter<MaxHotUnitCount_I)
		{
			Current= &(Begin[HotUnitAry_Ptr[Counter]]);

			if (0!=(UnitValid2_State& Current->UnitSelected))
			{
				if (0.0F==(Current->Nanoframe))
				{
					if ((NULL!=Current->Owner_PlayerPtr1))
					{
						if (TAMainStruct_Ptr->LocalHumanPlayer_PlayerID==Current->Owner_PlayerPtr1->PlayerAryIndex)
						{
							if (MobileWeaponMask
								&&MatchInTypeAry ( Current->UnitID, MobileWeaponMask))
							{
								Current->UnitSelected= (Current->UnitSelected)| (0x10);
								++SelectedUnits;
								DoSelect_b= true;
							}
							
						}
					}
				}
			}
			++Counter;
		}
		
		UpdateSelectUnitEffect ( ) ;
		ApplySelectUnitMenu_Wapper ( );
		//freeTAMem ( WeaponUnitAry);
	}

	ReleaseSemaphore ( Semaphore_OnlyInScreenWeapon, 1, NULL);
	return SelectedUnits;
}

int ExternQuickKey::SelectUnitInRect (TAUnitType NeededType, RECT * rect, bool shift)
{
	if (NULL==rect)
	{
		return 0;
	}
	TAdynmemStruct *PTR = TAMainStruct_Ptr;

	UnitStruct *  Begin= PTR->Players[PTR->LocalHumanPlayer_PlayerID].Units;
	UnitStruct *  End= PTR->Players[PTR->LocalHumanPlayer_PlayerID].UnitsAry_End;
	UnitStruct *  Current= Begin;
	int SelectedCounter= 0;

	bool DoSelect_b;

	while (Current<=End)
	{
		if (0!=((UnitValid2_State)& Current->UnitSelected))
		{
			if (0.0F==(Current->Nanoframe))
			{
				//ID= ;
				if ((NULL!=Current->Owner_PlayerPtr1))//&&(Player_LocalHuman==Current->ValidOwner_PlayerPtr->My_PlayerType))
				{
					//这儿是过滤  filter:
					DoSelect_b= false;

					if ((rect->left<Current->XPos)
						&&(Current->XPos<rect->right)
						&&(rect->top<(Current->YPos- Current->ZPos/ 2))
						&&((Current->YPos- Current->ZPos/ 2)<rect->bottom))
					{
						DoSelect_b= true;
					}
					if (shift)
					{
						if (DoSelect_b)
						{
							if (Current->UnitSelected& (UnitSelected_State))
							{
								Current->UnitSelected= Current->UnitSelected& (~ UnitSelected_State);
							}
							else
							{
								Current->UnitSelected= Current->UnitSelected| ( UnitSelected_State);
								++SelectedCounter;
							}

						}
					}
					else
					{
						if (DoSelect_b)
						{

							Current->UnitSelected=  Current->UnitSelected| UnitSelected_State;
							++SelectedCounter;
						}
						else
						{
							Current->UnitSelected= Current->UnitSelected& (~ UnitSelected_State);
						}
					}
	
				}
			}
		}

		Current= &Current[1];

	}
	if (0!=SelectedCounter)
	{
		UpdateSelectUnitEffect ( );
		ApplySelectUnitMenu_Wapper ( );
	}
	return SelectedCounter;
}

int ExternQuickKey::FilterSelectedUnit (void)
{
	if (0<(0x8000&GetKeyState ( 0x57)))
	{
		return FilterSelectedUnitProc ( WEAPONUNITS);
	}
	else if (0<(0x8000&GetKeyState ( 0x42)))
	{
		return FilterSelectedUnitProc ( ENGINEER);
	}
	else if (0<(0x8000&GetKeyState ( 0x59)))
	{
		return FilterSelectedUnitProc ( FACTORYS);
	}
	return CountSelectedUnits( ) ;
}
int ExternQuickKey::FilterSelectedUnitProc (TAUnitType NeededType) //只会在已选中的单位中选择!!!!速度很慢，全部单位都枚举一遍!
{
// 	if (INVALIDTYPE==NeededType)
// 	{
// 		return 0;
// 	}
	WaitForSingleObject ( Semaphore_FilterSelect, INFINITE);

	TAdynmemStruct *PTR = TAMainStruct_Ptr;
	
	UnitStruct *  Begin= PTR->Players[PTR->LocalHumanPlayer_PlayerID].Units;
	UnitStruct *  End= PTR->Players[PTR->LocalHumanPlayer_PlayerID].UnitsAry_End;
	UnitStruct *  Current= Begin;
	int SelectedCounter= 0;

	bool DoSelect_b;


	//"NoWeapon"
	while (Current<=End)
	{
		if ((0!=((UnitSelected_State)& Current->UnitSelected))&&(0!=((UnitValid2_State)& Current->UnitSelected)))
		{
			if (0.0F==(Current->Nanoframe))
			{
				//ID= ;
				if ((NULL!=Current->Owner_PlayerPtr1))//&&(Player_LocalHuman==Current->ValidOwner_PlayerPtr->My_PlayerType))
				{
					//这儿是过滤  filter:
					DoSelect_b= false;

					if (0!=(COMMANDER& NeededType)&&
						(NULL!=CommanderMask)&&
						(MatchInTypeAry ( Current->UnitID, CommanderMask)))
					{
						DoSelect_b= true;
					}
					
					if (0!=(WEAPONUNITS& NeededType)
						&&MatchInTypeAry ( Current->UnitID, MobileWeaponMask))
					{
						DoSelect_b= true;
					}

					if (0!=(ENGINEER& NeededType)
						&&MatchInTypeAry ( Current->UnitID, ConstructorMask))
					{
						DoSelect_b= true;
					}
					if (0!=(FACTORYS& NeededType)
						&&MatchInTypeAry ( Current->UnitID, FactoryMask))
					{
						DoSelect_b= true;
					}

					if (DoSelect_b)
					{
						Current->UnitSelected=  Current->UnitSelected| UnitSelected_State;
						++SelectedCounter;
					}
					else
					{
						Current->UnitSelected= Current->UnitSelected& (~ UnitSelected_State);
					}
				}
			}
		}

		Current= &Current[1];
		
	}


	UpdateSelectUnitEffect ( );
	ApplySelectUnitMenu_Wapper ( );
	

	ReleaseSemaphore ( Semaphore_FilterSelect, 1, NULL);

	return SelectedCounter;
}

int ExternQuickKey::InitExternTypeMask (void)
{
	CategroyMaskSize=  ((NowCrackLimit->NowIncreaseUnitTypeLimit->CurtUnitTypeNum)/ 8/ 0x40)* 0x40;
	UnitDefStruct * Begin= TAMainStruct_Ptr->UnitDef;
	UnitDefStruct * Current;
	int TypeCount= TAMainStruct_Ptr->UNITINFOCount;
	unsigned long NoWeaponPtr= reinterpret_cast<unsigned long> (NowCrackLimit->NowIncreaseWeaponTypeLimit->CurtPtr);

	int Inited= 0;

	if(NULL==CommanderMask)
	{
		CommanderMask= (LPDWORD)malloc (  CategroyMaskSize);
	}

	
	memset ( Commanders, 0, RACENUMBER* COMMANDNAMELEN);
	for (int i= 0; i<static_cast<int>(TAMainStruct_Ptr->RaceCounter); ++i)
	{
		strcpy_s ( &Commanders[i][0], 30, TAMainStruct_Ptr->RaceSideDataAry[i].name);
		if (NULL==CommandersMask[i])
		{
			CommandersMask[i]=  (LPDWORD)malloc (  CategroyMaskSize);
		}
		memset ( CommandersMask[i], 0, CategroyMaskSize);
	}

	
	memset ( CommanderMask, 0, CategroyMaskSize);
	
	for (int i= 0; i<TypeCount; ++i)
	{
		Current= &Begin[i];
		if ((0!=(showplayername& Current->UnitTypeMask_1))
			&&0!=(hidedamage& Current->UnitTypeMask_0))
		{// decoy and commander both set showplayername=1 and hidedamage= 1
			SetIDMaskInTypeAry (  Current->UnitTypeID, CommanderMask);

			for (DWORD i= 0; i<TAMainStruct_Ptr->RaceCounter; ++i)
			{
				char GamedataSide[0x10];
				char UnitSide[0x10];

				strcpy_s ( GamedataSide, 8, &Commanders[i][0]);
				strcpy_s ( UnitSide, 8, Current->Side);

				_strlwr_s ( GamedataSide, 8);
				_strlwr_s ( UnitSide, 8);
				if (0== _strcmpi ( GamedataSide, UnitSide))
				{
					SetIDMaskInTypeAry (  Current->UnitTypeID,  CommandersMask[i]);
				}
			}
		}
	}
	Inited++;
	

	if (NULL==MobileWeaponMask)
	{
		MobileWeaponMask= (LPDWORD)malloc (  CategroyMaskSize);
		
	}
		
	memset ( MobileWeaponMask, 0, CategroyMaskSize);
	for (int i= 0; i<TypeCount; ++i)
	{
		Current= &Begin[i];
		if((NoWeaponPtr!=reinterpret_cast<unsigned long>(Current->weapon1)&&(NULL!=Current->weapon1)&&(0==(stockpile_mask&(Current->weapon1->WeaponTypeMask))))
			|| (NoWeaponPtr!=reinterpret_cast<unsigned long>(Current->weapon2)&&(NULL!=Current->weapon2)&&(0==(stockpile_mask&(Current->weapon2->WeaponTypeMask))))
			|| (NoWeaponPtr!=reinterpret_cast<unsigned long>(Current->weapon3)&&(NULL!=Current->weapon3)&&(0==(stockpile_mask&(Current->weapon3->WeaponTypeMask))))
			)
		{
			if ((NULL!=CommanderMask)&&
				(! MatchInTypeAry ( Current->UnitTypeID, CommanderMask)))
			{//don't select commander in here
				if((0==(builder&Current->UnitTypeMask_0)))
				{
					if (NULL==Current->YardMap)
					{//not building
						if (canfly!=(canfly& Current->UnitTypeMask_0))
						{
							SetIDMaskInTypeAry ( Current->UnitTypeID, MobileWeaponMask);
						}
					}
				}
			}
		}
		
	}
		Inited++;

	
	
	if (NULL==ConstructorMask)
	{
		ConstructorMask= (LPDWORD)malloc (  CategroyMaskSize);
		
	}
		
	memset ( ConstructorMask, 0, CategroyMaskSize);
	for (int i= 0; i<TypeCount; ++i)
	{
		Current= &Begin[i];
		if ((0!=(builder&Current->UnitTypeMask_0))
			&&(NULL==Current->YardMap)//building

			&&(! MatchInTypeAry ( Current->UnitTypeID, CommanderMask))
			)
		{
			SetIDMaskInTypeAry ( Current->UnitTypeID, ConstructorMask);
		}
	}
	Inited++;
	
	if (NULL==FactoryMask)
	{
		FactoryMask= (LPDWORD)malloc (  CategroyMaskSize);
	}
	memset ( FactoryMask, 0, CategroyMaskSize);
	for (int i= 0; i<TypeCount; ++i)
	{
		Current= &Begin[i];

		if ((0!=(builder&Current->UnitTypeMask_0))
			&&(NULL!=Current->YardMap)
			&&(! MatchInTypeAry ( Current->UnitTypeID, CommanderMask)))
		{
			SetIDMaskInTypeAry ( Current->UnitTypeID, FactoryMask);
		}
	}
	Inited++;
	

	if (NULL==BuildingMask)
	{
		BuildingMask= (LPDWORD)malloc (  CategroyMaskSize);


	}
	memset ( BuildingMask, 0, CategroyMaskSize);
	for (int i= 0; i<TypeCount; ++i)
	{
		Current= &Begin[i];

		if ((NULL!=Current->YardMap)
			&&(! MatchInTypeAry ( Current->UnitTypeID, CommanderMask)))
		{
			SetIDMaskInTypeAry ( Current->UnitTypeID, BuildingMask);
		}
	}
	Inited++;
	if (NULL==AirWeaponMask)
	{
		AirWeaponMask= (LPDWORD)malloc (  CategroyMaskSize);
	
	}
	memset ( AirWeaponMask, 0, CategroyMaskSize);
	for (int i= 0; i<TypeCount; ++i)
	{
		Current= &Begin[i];
	
		if((NoWeaponPtr!=reinterpret_cast<unsigned long>(Current->weapon1)&&(NULL!=Current->weapon1)&&(0==(stockpile_mask&(Current->weapon1->WeaponTypeMask))))
			|| (NoWeaponPtr!=reinterpret_cast<unsigned long>(Current->weapon2)&&(NULL!=Current->weapon2)&&(0==(stockpile_mask&(Current->weapon2->WeaponTypeMask))))
			|| (NoWeaponPtr!=reinterpret_cast<unsigned long>(Current->weapon3)&&(NULL!=Current->weapon3)&&(0==(stockpile_mask&(Current->weapon3->WeaponTypeMask))))
			)
		{

			if ((NULL!=CommanderMask)&&
				(! MatchInTypeAry ( Current->UnitTypeID, CommanderMask)))
			{//don't select commander in here
				if((0==(builder&Current->UnitTypeMask_0)))
				{
					if (NULL==Current->YardMap)
					{//not building
						if (canfly==(canfly& Current->UnitTypeMask_0))
						{
							SetIDMaskInTypeAry ( Current->UnitTypeID, AirWeaponMask);

						}
					}
				}
			}
		}
		
	}
	Inited++;
	if (NULL==AirConMask)
	{
		AirConMask= (LPDWORD)malloc (  CategroyMaskSize);
	
	}
	memset ( AirConMask, 0, CategroyMaskSize);
	for (int i= 0; i<TypeCount; ++i)
	{
		Current= &Begin[i];
		if ((0!=(builder&Current->UnitTypeMask_0))
			&&(NULL==Current->YardMap)//building
			&&(! MatchInTypeAry ( Current->UnitTypeID, CommanderMask))
			&&(canfly==(canfly& Current->UnitTypeMask_0)))
		{
			SetIDMaskInTypeAry ( Current->UnitTypeID, AirConMask);
		}
	}
	Inited++;

	return Inited;
}

void ExternQuickKey::DestroyExternTypeMask (void)
{
	for	(int i= 0; i<RACENUMBER; ++i)
	{
		free ( CommandersMask[i]);
	}
	if (CommanderMask)
	{
		free (CommanderMask);
		CommanderMask= NULL;
	}
	if (MobileWeaponMask)
	{
		free ( MobileWeaponMask);
		MobileWeaponMask= NULL;
	}
	if (ConstructorMask)
	{
		free ( ConstructorMask);
		ConstructorMask= NULL;
	}
	if (FactoryMask)
	{
		free ( FactoryMask);
		FactoryMask= NULL;
	}
	if (BuildingMask)
	{
		free ( BuildingMask);
		BuildingMask= NULL;
	}
	if (AirWeaponMask)
	{
		free ( AirWeaponMask);
		AirWeaponMask= NULL;
	}
	if (AirConMask)
	{
		free ( AirConMask);
		AirConMask= NULL;
	}
}
int __stdcall AddtionRoutine_CircleSelect (PInlineX86StackBuffer X86StrackBuffer)
{
	//int Selected= X86StrackBuffer->Ebp;
	__try	
	{
		if(DataShare->ehaOff == 1 && !DataShare->PlayingDemo)
		{
			__leave;
		}

		if (TAInGame==DataShare->TAProgress)
		{
			ExternQuickKey * this_myExternQuickKey= (ExternQuickKey *)(X86StrackBuffer->myInlineHookClass_Pish->ParamOfHook);
			int Temp=  this_myExternQuickKey->FilterSelectedUnit ( );
			if (X86StrackBuffer->Ebp!=Temp)
			{
				return X86STRACKBUFFERCHANGE;
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		;
	}
	
	return 0;
}


int __stdcall AddtionRoutine_UnitINFOInit (PInlineX86StackBuffer X86StrackBuffer)
{
	ExternQuickKey * this_myExternQuickKey= (ExternQuickKey *)(X86StrackBuffer->myInlineHookClass_Pish->ParamOfHook);
	this_myExternQuickKey->InitExternTypeMask ( );
	AddRoutine_InitAfterExternKey ( );
	return 0;
};


void AddRoutine_InitAfterExternKey ( void)
{
#ifdef USEMEGAMAP

	if (GUIExpander)
	{
		if ((GUIExpander->myMinimap)
			&&(GUIExpander->myMinimap->UnitsMap))
		{
			GUIExpander->myMinimap->UnitsMap->LoadUnitPicture ( );
		}
	}
#endif
}