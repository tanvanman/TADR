#pragma once

#define PTRARYNUM (14)
#define LENARYNUM (3)

#define IDLIMITARYNUMBER (23)

extern BYTE IDlimit0_bits[];
extern BYTE IDlimit1_bits[];
extern BYTE IDlimit3_bits[];
extern BYTE IDlimit4_bits[];
extern BYTE IDlimit5_bits[];
extern BYTE IDlimit6_bits[];
extern BYTE IDlimit7_bits[];
extern BYTE IDlimit8_bits[];

extern BYTE IDlimit11_bits[];
extern BYTE IDlimit12_bits[];
extern BYTE IDlimit13_bits[];

extern BYTE IDlimit2_bits[];
extern unsigned int IDlimit2Addr;

extern BYTE WeaponPtr2Bits[];
extern unsigned int WeaponPtr2Addr;

extern LPVOID * WeaponRelatedPtr[];
extern LPDWORD WeaponAryLen [];
extern DWORD IDlimitAddrAry[];

extern LPBYTE IDlimitBitsAry[];
extern DWORD IDlimitLenAry[];

class IncreaseWeaponTypeLimit
{
public:
	LPVOID CurtPtr;
	DWORD CurtLen;

	SingleHook * PtrHookAry[PTRARYNUM];
	SingleHook * LenHookAry[LENARYNUM];

	ModifyHook * WeaponPtr2;

	SingleHook * IDlimitAry[IDLIMITARYNUMBER];

	ModifyHook * IDlimit2;

private:
	LPVOID OrgPtr;
	DWORD OrglLen;

public:
	IncreaseWeaponTypeLimit ();
	IncreaseWeaponTypeLimit (DWORD NewLen);
	~IncreaseWeaponTypeLimit ();

private:
	LPVOID WriteWeaponAryPtr ( LPVOID * NewPtr);
	DWORD WriteWeaponAryLen ( DWORD Newlen);
	void NewLimit (DWORD NewLen);
	void WeaponType_IdLimit (void);
};
