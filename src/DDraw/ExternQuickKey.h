#ifndef EXTERNQUICKEY_H_316SDHSD
#define EXTERNQUICKEY_H_316SDHSD


enum TAUnitType
{
	ALL=0,
	COMMANDER= 1,
	ENGINEER= 2,
	FACTORYS= 4,
	WEAPONUNITS= 8, 
	LANDUNITS= 16,
	SEAUNITS= 32,
	AIRUNITS= 64, 

	INVALIDTYPE= -1,
};

enum SELECTMASK
{
	MOVEUNITSELECTABLE= 1,
	STATICBUILDINGSELECTABLE= 2,
	COMMANDERSELECTABLE= 4,
	AIRSELECTABLE= 8
};


class InlineSingleHook;
enum TAUnitType;

#define  COMMANDNAMELEN (0x40)
#define		RACENUMBER (5)

int __stdcall AddtionRoutine_CircleSelect (PInlineX86StackBuffer X86StrackBuffer);
int __stdcall AddtionRoutine_UnitINFOInit (PInlineX86StackBuffer X86StrackBuffer);

void AddRoutine_InitAfterExternKey ( void);

class ExternQuickKey
{
public:
	BOOL DoubleClick;

	LPDWORD CommanderMask;
	LPDWORD MobileWeaponMask;
	LPDWORD ConstructorMask;
	LPDWORD FactoryMask;
	LPDWORD BuildingMask;
	LPDWORD AirWeaponMask;
	LPDWORD AirConMask;

	CHAR Commanders[RACENUMBER][COMMANDNAMELEN];
	LPDWORD CommandersMask[RACENUMBER];
private:
	TAdynmemStruct * TAMainStruct_Ptr;
	HANDLE Semaphore_OnlyInScreenSameType;
	HANDLE Semaphore_FilterSelect;
	HANDLE Semaphore_OnlyInScreenWeapon;
	HANDLE Semaphore_IdleCons;
	HANDLE Semaphore_IdleFac;

	

	InlineSingleHook * HookInCircleSelect;
	InlineSingleHook * HookInUNITINFOInited;

	int VirtualKeyCode;
	int CategroyMaskSize;

	char *Add;
	char *Sub;
	char OldAdd;
	char OldSub;
	char NumAdd;
	char NumSub;
public:
	ExternQuickKey ();
	~ExternQuickKey ();
public:
	bool Message(HWND WinProcWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	int FilterSelectedUnitProc (TAUnitType NeededType);
	int FilterSelectedUnit (void);

	int InitExternTypeMask (void);
	void DestroyExternTypeMask (void);

	int SelectUnitInRect (TAUnitType NeededType, RECT * rect, bool shift=false);

	int SelectOnlyInScreenSameTypeUnit (BOOL FirstSelect_Flag);
	int SelectOnlyInScreenWeaponUnit (unsigned int SelectWay_Mask);

	void FindIdelFactory ();
	void FindIdleConst();

};



extern ExternQuickKey * myExternQuickKey;

#endif //EXTERNQUICKEY_H_316SDHSD