#include "iddraw.h"
#include "iddrawsurface.h"
#include <vector>
using namespace std;

#include "hook/etc.h"
#include "hook/hook.h"

#include "tamem.h"
#include "tafunctions.h"


#include "AiSeardchMapEntriesLimit.h"
#include "UnitTypeLimit.h"
#include "IncreaseCompositeSize.h"
#include "IncreaseSfxLimit.h"
#include "SwitchAlt.h"
#include "UnicodeSupport.h"
#include "MenuResolution.h"
#include "UnitLimit.h"
#include "ExternQuickKey.h"
#include "sharedialog.h"
#include "fullscreenminimap.h"

#include "LimitCrack.h"

#include "TAConfig.h"




LimitCrack* NowCrackLimit;

/// I had put nealy everythings in to this file, so, the file name isn't that improtant at now :\

LimitCrack::LimitCrack ( void)
{
	NowIncreaseAISearchMapEntriesLimit= new IncreaseAISearchMapEntriesLimit ( MyConfig->GetIniInt ( "AISearchMapEntries", 66650));
	NowIncreaseUnitTypeLimit= new IncreaseUnitTypeLimit ( MyConfig->GetIniInt ( "UnitType", 16000) );
	NowIncreaseCompositeBuf= new IncreaseCompositeBuf ( MyConfig->GetIniInt ( "X_CompositeBuf", 1280) , MyConfig->GetIniInt ( "Y_CompositeBuf", 1280));
	NowIncreaseSfxLimit= new IncreaseSfxLimit ( MyConfig->GetIniInt ( "SfxLimit", 16000));

	SetUnitLimit= new UnitLimit (  MyConfig->GetIniInt ( "UnitLimit", 3663));

	//set the Reg things;

	DataShare->IniCRC= MyConfig->GetIniCrc ( );
}

LimitCrack::~LimitCrack ( void)
{
	delete NowIncreaseAISearchMapEntriesLimit;
	delete NowIncreaseUnitTypeLimit;
	delete NowIncreaseCompositeBuf;
	//delete NowIncreaseMixingBuffers;
	//delete NowSwitchalt;
	delete NowIncreaseSfxLimit;
	delete myExternQuickKey;

}

IncreaseAISearchMapEntriesLimit::IncreaseAISearchMapEntriesLimit ()
{
	//OrginalLimit= *AISearchMapEntriesLimit;
	IDDrawSurface::OutptTxt ("IncreaseAISearchMapEntriesLimit");
	DWORD Const66650= 66650;
	ModifyTheLimit= new SingleHook ( reinterpret_cast<LPBYTE>(AISearchMapEntriesLimit), 4, INLINE_UNPROTECTEVINMENT, reinterpret_cast<LPBYTE>(&Const66650));
};

IncreaseAISearchMapEntriesLimit::IncreaseAISearchMapEntriesLimit (DWORD NewLimit)
{
	//OrginalLimit= *AISearchMapEntriesLimit;
	IDDrawSurface::OutptTxt ("IncreaseAISearchMapEntriesLimit");
	ModifyTheLimit= new SingleHook ( reinterpret_cast<LPBYTE>(AISearchMapEntriesLimit), 4, INLINE_UNPROTECTEVINMENT, reinterpret_cast<LPBYTE>(&NewLimit));
}

IncreaseAISearchMapEntriesLimit::~IncreaseAISearchMapEntriesLimit ()
{
	delete ModifyTheLimit;
}

IncreaseUnitTypeLimit::IncreaseUnitTypeLimit ()
{

	WriteNewLimit ( 16000);
}

IncreaseUnitTypeLimit::IncreaseUnitTypeLimit ( int Number)
{	
	WriteNewLimit ( Number);
}
//
IncreaseUnitTypeLimit::~IncreaseUnitTypeLimit ()
{
	IDDrawSurface::OutptTxt ("Release IncreaseUnitTypeLimit");
	if (NULL!=Prologue_Weight)
	{
		delete Prologue_Weight;
	}
	if (NULL!=Argc_0_Weight)
	{
		delete Argc_0_Weight;
	}
	
	if (NULL!=Argc_1_Weight)
	{
		delete Argc_1_Weight;
	}
	if (NULL!=Epilogue_Weight)
	{
		delete Epilogue_Weight;
	}
	if (NULL!=Prologue_limit)
	{
		delete Prologue_limit;
	}
	if (NULL!=Argc_0_limit)
	{
		delete Argc_0_limit;
	}
	if (NULL!=Argc_1_limit)
	{
		delete Argc_1_limit;
	}
	if (NULL!=Argc_2_limit)
	{
		delete Argc_2_limit;
	}
	if (NULL!=Epilogue_limit)
	{
		delete Epilogue_limit;
	}
	if (NULL!=Push_FindSpot)
	{
		delete Push_FindSpot;
	}
	if (NULL!=Mov_FindSpot)
	{
		delete Mov_FindSpot;
	}
	

}

void IncreaseUnitTypeLimit::WriteNewLimit (DWORD Number)
{

	IDDrawSurface::OutptTxt ("WriteNewLimit");
	//weight func
	DWORD Orginal= 0x40;

	CurtUnitTypeNum= Number* 8;

	

	DWORD New;
	New= ((Number/ 8/ 0x40)+ 1)* 0x40;// 0x200 Number== 0x40 New

	
	Prologue_Weight= NULL;
	Argc_0_Weight= NULL;
	Argc_1_Weight= NULL;
	Epilogue_Weight= NULL;
	Prologue_limit= NULL;
	Argc_0_limit= NULL;
	Argc_1_limit= NULL;
	Argc_2_limit= NULL;
	Epilogue_limit= NULL;
	Push_FindSpot= NULL;
	Mov_FindSpot= NULL;

	if (Number<=(Orginal* 8))
	{
		CurtUnitTypeNum= 0x200;
		return ;
	}

	

	DWORD RepsdEcx= New/ 4;
	
	BYTE Prologue_Weight_bits[]={0x81 ,0xEC ,0x00 ,0x01 ,0x00 ,0x00};
	//00406DB5   83EC 44          SUB ESP,44
	*(DWORD *)(&Prologue_Weight_bits[2])= New- 0x40+ 0x44;
	Prologue_Weight= new ModifyHook ( 0x0406DB5, INLINE_MODIFYCODE, 0x5, Prologue_Weight_bits, sizeof( Prologue_Weight_bits), 0x3);

	BYTE Argc_0_Weight_bits[]={0x8B ,0xB4 ,0x24 ,0x00 ,0x01 ,0x00 ,0x00};
	//00406DC9   8B7424 50        MOV ESI,DWORD PTR SS:[ESP+50]
	*(DWORD *)(&Argc_0_Weight_bits[3])= New- 0x40+ 0x50;
	Argc_0_Weight= new ModifyHook ( 0x406DC9, INLINE_MODIFYCODE, 0x6, Argc_0_Weight_bits, sizeof( Argc_0_Weight_bits), 0x4);
	BYTE Argc_1_Weight_bits[]={0xD9 ,0x9C ,0x24 ,0x00 ,0x01 ,0x00 ,0x00 ,0x8B ,0xAC ,0x24 ,0x00 ,0x01 ,0x00 ,0x00};
	//00406DFD   D95C24 58        FSTP DWORD PTR SS:[ESP+58]
	//00406E01   8B6C24 58        MOV EBP,DWORD PTR SS:[ESP+58]
	*(DWORD *)(&Argc_1_Weight_bits[3])= New- 0x40+ 0x58;
	*(DWORD *)(&Argc_1_Weight_bits[10])= New- 0x40+ 0x58;

	Argc_1_Weight= new ModifyHook ( 0x00406DFD, INLINE_MODIFYCODE, 0xd, Argc_1_Weight_bits, sizeof( Argc_1_Weight_bits), 0x8);
	BYTE Epilogue_Weight_bits[]={0x81 ,0xC4 ,0x00 ,0x01 ,0x00 ,0x00};
	//00406E3A   83C4 44          ADD ESP,44
	*(DWORD *)(&Epilogue_Weight_bits[2])= New- 0x40+ 0x44;
	Epilogue_Weight= new ModifyHook ( 0x0406E3A, INLINE_MODIFYCODE, 0x6, Epilogue_Weight_bits, sizeof( Epilogue_Weight_bits), 0x3);

	//limit func
	BYTE Prologue_limit_bits[]={0x81 ,0xEC ,0x00 ,0x01 ,0x00 ,0x00};
	//00406E45   83EC 40          SUB ESP,40
	*(DWORD *)(&Prologue_limit_bits[2])= New- 0x40+ 0x40;
	Prologue_limit= new ModifyHook ( 0x0406E45, INLINE_MODIFYCODE, 0x5, Prologue_limit_bits, sizeof( Prologue_limit_bits), 0x3);
	BYTE Argc_0_limit_bits[]={0x8B ,0xB4 ,0x24 ,0x00 ,0x01 ,0x00 ,0x00};
	//00406E5D   8B7424 4C        MOV ESI,DWORD PTR SS:[ESP+4C]
	*(DWORD *)(&Argc_0_limit_bits[3])= New- 0x40+ 0x4c;

	Argc_0_limit= new ModifyHook ( 0x00406E5D, INLINE_MODIFYCODE, 0x6, Argc_0_limit_bits, sizeof( Argc_0_limit_bits), 0x4);
	BYTE Argc_1_limit_bits[]={0x8D ,0x84 ,0x24 ,0x00 ,0x01 ,0x00 ,0x00};
	//00406E64   8D4424 50        LEA EAX,DWORD PTR SS:[ESP+50]
	*(DWORD *)(&Argc_1_limit_bits[3])= New- 0x40+ 0x50;

	Argc_1_limit= new ModifyHook ( 0x00406E64, INLINE_MODIFYCODE, 0x5, Argc_1_limit_bits, sizeof( Argc_1_limit_bits), 0x4);
	BYTE Argc_2_limit_bits[]= {0x8B ,0x8C ,0x24 ,0x00 ,0x01 ,0x00 ,0x00};
	//00406EB2   8B4C24 54        MOV ECX,DWORD PTR SS:[ESP+54]
	*(DWORD *)(&Argc_2_limit_bits[3])= New- 0x40+ 0x54;

	Argc_2_limit= new ModifyHook ( 0x00406EB2, INLINE_MODIFYCODE, 0x8, Argc_2_limit_bits, sizeof( Argc_2_limit_bits), 0x4);
	BYTE Epologue_limit_bits[]= {0x81 ,0xC4 ,0x00 ,0x01 ,0x00 ,0x00};//
	//00406ED6   83C4 40          ADD ESP,40
	*(DWORD *)(&Epologue_limit_bits[2])= New- 0x40+ 0x40;

	Epilogue_limit= new ModifyHook ( 0x00406ED6, INLINE_MODIFYCODE, 0x6, Epologue_limit_bits, sizeof( Epologue_limit_bits), 0x3);

	//FindSpot 
	BYTE Push_FindSpot_bits[]= {0x68 ,0x00 ,0x01 ,0x00 ,0x00};
	// 00488CC2 6A 40           push    40h   
	*(DWORD *)(&Push_FindSpot_bits[1])= New- 0x40+ 0x40;
	Push_FindSpot= new ModifyHook ( 0x488CC2, INLINE_MODIFYCODE, 0x7, Push_FindSpot_bits, sizeof( Push_FindSpot_bits), 0x2);

	Mov_FindSpot= new SingleHook ( 0x488CD3, 0x4, INLINE_UNPROTECTEVINMENT, (LPBYTE)(&RepsdEcx)); 
}


IncreaseCompositeBuf::IncreaseCompositeBuf ()
{
	WriteNewLimit ( 0x1000, 0x1000);
}

IncreaseCompositeBuf::IncreaseCompositeBuf (DWORD x, DWORD y)
{
	WriteNewLimit ( x, y);
}

IncreaseCompositeBuf::~IncreaseCompositeBuf ()
{
	IDDrawSurface::OutptTxt ("Release IncreaseCompositeBuf");
	delete CompositeBufSizeHook;
}

void IncreaseCompositeBuf::WriteNewLimit (DWORD x, DWORD y)
{
	IDDrawSurface::OutptTxt ("IncreaseCompositeBuf");
	CurtX= x;
	CurtY= y;
	BYTE NewBits[]= {0x68,0x58,0x02,0x00,0x00,0x68,0x58,0x02,0x00,0x00};

	*(DWORD *)(&NewBits[1])= y;
	*(DWORD *)(&NewBits[6])= x;
	CompositeBufSizeHook= new SingleHook ( 0x458195, 10, INLINE_UNPROTECTEVINMENT, NewBits);
}


unsigned int SfxVectorLimitAry[SFXVECTORLIMITNUM]=
{
	0x00471184,
	0x004713D9,
	0x00471509,
	0x0047163E,
	0x00471783,

	0x004718B2,
	0x00471AD8,
	0x00472072,
	0x004721A0,
	0x004722D0,

	0x004723D7,
	0x004724D6,
	0x004725D5,
	0x004726C1,
	0x004727B1,

	0x0047289B,
	0x00472A5B,
	0x00472CDA,
	0x0047297B,
	0x00472BF4
};
/*
hook
	00471C80   6A 4C            PUSH 4C
	00471C82   68 E8030000      PUSH 3E8
	00471C87 B9 10 E6 51 00      mov     ecx, offset SfxEffectProperty
*/


int __stdcall Sfx_mallocBufSizeRouter (PInlineX86StackBuffer X86StrackBuffer)
{
	*reinterpret_cast<DWORD *>(X86StrackBuffer->Esp)= NowCrackLimit->NowIncreaseSfxLimit->CurtMaxLimit;
	return 0;
}

IncreaseSfxLimit::IncreaseSfxLimit ()
{
	SetNewLimit ( 0x5000, 0x32000);
}

IncreaseSfxLimit::IncreaseSfxLimit (DWORD SfxVectorNum)
{
	SetNewLimit ( SfxVectorNum, SfxVectorNum* 10);
}

IncreaseSfxLimit::~IncreaseSfxLimit ()
{
	IDDrawSurface::OutptTxt ("Release IncreaseSfxLimit");
	for (int i= 0; i<SFXVECTORLIMITNUM; ++i)
	{
		if (NULL!=SHookSfxVectorAry[i])
		{
			delete SHookSfxVectorAry[i];
		}
	}
	if (NULL!=Sfx_mallocBufSize)
	{
		delete Sfx_mallocBufSize;
	}
}

void IncreaseSfxLimit::SetNewLimit ( DWORD SfxVectorNum, DWORD MaxLimit)
{
	IDDrawSurface::OutptTxt ("IncreaseSfxLimit");
	for (int i= 0; i<SFXVECTORLIMITNUM; ++i)
	{
		SHookSfxVectorAry[i]= NULL;
	}
	Sfx_mallocBufSize= NULL;

	CurtSfxVectorNum= SfxVectorNum;
	CurtMaxLimit= MaxLimit;

	for (int i= 0; i<SFXVECTORLIMITNUM; ++i)
	{
		SHookSfxVectorAry[i]= new SingleHook ( SfxVectorLimitAry[i], 4, INLINE_UNPROTECTEVINMENT, reinterpret_cast<LPBYTE>(&SfxVectorNum));
	}
	Sfx_mallocBufSize= new InlineSingleHook ( Sfx_mallocBufSizeAddr, 0x5, INLINE_5BYTESLAGGERJMP, Sfx_mallocBufSizeRouter);
}



UnitLimit::UnitLimit ()
{

	MPUnitLimit= NULL;
	UnitLimit0= NULL;
	UnitLimit1= NULL;
	UnitLimit2= NULL;
	//= 1500;
}

UnitLimit::UnitLimit (DWORD NewLimit)
{

	MPUnitLimit= NULL;
	UnitLimit0= NULL;
	UnitLimit1= NULL;
	UnitLimit2= NULL;
	//= 1500;
	NewUnitLimit ( NewLimit);
}
UnitLimit::~UnitLimit ()
{
	if (NULL!=MPUnitLimit)
	{
		delete MPUnitLimit;
		MPUnitLimit= NULL;
	}
	if (NULL!=UnitLimit0)
	{
		delete UnitLimit0;
		UnitLimit0= NULL;
	}
	if (NULL!=UnitLimit1)
	{
		delete UnitLimit1;
		UnitLimit1= NULL;
	}
	if (NULL!=UnitLimit2)
	{
		delete UnitLimit2;
		UnitLimit2= NULL;
	}
}

void UnitLimit::NewUnitLimit (DWORD NewLimit)
{

	MPUnitLimit= new SingleHook ( reinterpret_cast<LPBYTE>(MPUnitLimitAddr), 4, INLINE_UNPROTECTEVINMENT, reinterpret_cast<LPBYTE>( &NewLimit));

	UnitLimit0= new SingleHook ( reinterpret_cast<LPBYTE>(UnitLimit0Addr), 4, INLINE_UNPROTECTEVINMENT, reinterpret_cast<LPBYTE>( &NewLimit));
	UnitLimit1= new SingleHook ( reinterpret_cast<LPBYTE>(UnitLimit1Addr), 4, INLINE_UNPROTECTEVINMENT, reinterpret_cast<LPBYTE>( &NewLimit));
	UnitLimit2= new SingleHook ( reinterpret_cast<LPBYTE>(UnitLimit2Addr), 4, INLINE_UNPROTECTEVINMENT, reinterpret_cast<LPBYTE>( &NewLimit));
}