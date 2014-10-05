#include "..\oddraw.h"
#include "hook.h"
#include "etc.h"

ModifyHook::ModifyHook ()
{
	mallocedBuf= NULL;
	m_RedirectOrgOpcodes_Pbyte= NULL;
	m_RedirectOrgOpcodesPtrInStub_Pvoid= NULL;
	m_OrgBytes_Pbyte= NULL;
}

ModifyHook::ModifyHook (unsigned int AddrToHook_Pvoid, int HookMode, DWORD MaxLen, LPBYTE OverWriteOpcode, DWORD OverWriteLen, DWORD OffToRedirect)
{
	mallocedBuf= NULL;
	m_RedirectOrgOpcodes_Pbyte= NULL;
	m_RedirectOrgOpcodesPtrInStub_Pvoid= NULL;
	m_OrgBytes_Pbyte= NULL;

	InitHookClass ( reinterpret_cast<LPBYTE>(AddrToHook_Pvoid), HookMode, MaxLen, OverWriteOpcode, OverWriteLen, OffToRedirect);
	if (NULL!=m_NewBytes_Pbyte)
	{
		Hook ( ) ;
	}
}
ModifyHook::ModifyHook (LPBYTE AddrToHook_Pvoid, int HookMode, DWORD MaxLen, LPBYTE OverWriteOpcode, DWORD OverWriteLen, DWORD OffToRedirect)
{
	mallocedBuf= NULL;
	m_RedirectOrgOpcodes_Pbyte= NULL;
	m_RedirectOrgOpcodesPtrInStub_Pvoid= NULL;
	m_OrgBytes_Pbyte= NULL;

	InitHookClass ( (AddrToHook_Pvoid), HookMode, MaxLen, OverWriteOpcode, OverWriteLen, OffToRedirect);
	if (NULL!=m_NewBytes_Pbyte)
	{
		Hook ( ) ;
	}
}
ModifyHook::~ModifyHook ()
{
	if ( NULL!=m_OrgBytes_Pbyte)
	{
		UnHook ( );
		delete []m_OrgBytes_Pbyte;
		m_OrgBytes_Pbyte= NULL;
	}

	if (NULL!=m_RedirectOrgOpcodes_Pbyte)
	{
		delete [] m_RedirectOrgOpcodes_Pbyte;
	}
	if (NULL!=mallocedBuf)
	{
		delete [] mallocedBuf;
		mallocedBuf= NULL;
	}
}

void ModifyHook::InitHookClass (LPBYTE AddrToHook_Pvoid, int HookMode, DWORD MaxLen, LPBYTE OverWriteOpcode, DWORD OverWriteLen, DWORD OffToRedirect)
{
	m_NewBytes_Pbyte= NULL;
	m_LenToModify_Dw= 0;
	AddrToHook ( AddrToHook_Pvoid);
	ThisHookMode ( HookMode);


	if ( NULL==AddrToHook ())
	{
		return ;
	}

	//int FirstOpcodeLen= 0;
	switch ( ThisHookMode ())
	{
	case INLINE_MODIFYCODE:
		//FirstOpcodeLen= GetOpCodeSize ( AddrToHook ( ));
/*
		if (OffToRedirect>=OverWriteLen)
		{
			m_NewBytes_Pbyte= OverWriteOpcode;
			m_LenToModify_Dw= MaxLen;
			m_OrgBytes_Pbyte= new BYTE [MaxLen];
			
			m_OverWriteLen= OverWriteLen;
			m_OffToRedirect= OffToRedirect;
			memcpy ( m_OrgBytes_Pbyte, AddrToHook ( ), MaxLen);

			return ;
		}*/

		if ((MaxLen<OverWriteLen)
			&&(MaxLen<5))
		{
			// WTF CODE to hook
			return ;
		}
		
		DWORD Len_Dw;
		DWORD tempForProtect_Dw;
		Len_Dw= MaxLen* 0x4+ 0x5;
		mallocedBuf= new BYTE [Len_Dw];

		VirtualProtect ( mallocedBuf, Len_Dw, PAGE_EXECUTE_READWRITE, &tempForProtect_Dw);

		m_NewBytes_Pbyte= mallocedBuf;
		m_OverWriteLen= OverWriteLen;

		memset ( mallocedBuf, 0x90, MaxLen* 0x4+ 0x5);
		memcpy ( mallocedBuf, OverWriteOpcode, OverWriteLen);
		DWORD RedirectOpCodeLen;
		RedirectOpCodeLen= 0;
		m_RedirectOrgOpcodes_Pbyte= NULL;
		m_LenToModify_Dw= MaxLen- OffToRedirect;

		
		if (OffToRedirect<MaxLen)
		{
			RedirectOpCodeLen= X86RedirectOpcodeToNewBase ( mallocedBuf+ OverWriteLen, (LPBYTE)AddrToHook ( )+ OffToRedirect, &m_LenToModify_Dw, &m_RedirectOrgOpcodes_Pbyte);
			memcpy ( mallocedBuf+ OverWriteLen, m_RedirectOrgOpcodes_Pbyte, RedirectOpCodeLen);
			
		}
		m_LenToModify_Dw= m_LenToModify_Dw+ OffToRedirect;

		LPBYTE JmpBack;
		JmpBack= &mallocedBuf[OverWriteLen+ RedirectOpCodeLen];
		JmpBack[0]= 0xe9;
		*((DWORD *)&JmpBack [1])= (DWORD)(((LPBYTE)AddrToHook ( )+ m_LenToModify_Dw)- JmpBack - 0x5);

		m_OffToRedirect= OffToRedirect;
		break;
	default:
		SingleHook::InitHookClass ( AddrToHook_Pvoid, MaxLen, HookMode, OverWriteOpcode);
	}
}

 void  ModifyHook::Hook (void)
 {
	 switch ( ThisHookMode ( ))
	 {
	 case INLINE_MODIFYCODE:
		if ((m_LenToModify_Dw<m_OffToRedirect)
			&&(m_LenToModify_Dw<5))
		{
			return ;
		}

		LPBYTE nopBts;
		nopBts= new BYTE[m_LenToModify_Dw];
		memset ( nopBts, 0x90, m_LenToModify_Dw);
		MemWriteWithBackup ( AddrToHook (), m_LenToModify_Dw, NULL, nopBts);
		if (m_OverWriteLen<=m_OffToRedirect)
		{
			MemWriteWithBackup ( AddrToHook (), 5, NULL, m_NewBytes_Pbyte);
		}
		else
		{
			nopBts[0]= 0xe9;
			*((DWORD *)&nopBts [1])= (DWORD)((m_NewBytes_Pbyte)- (LPBYTE)AddrToHook ( ) - 0x5);

			MemWriteWithBackup ( AddrToHook (), 5, NULL, nopBts);
		}

		 delete [] nopBts;
		 
		 break;
	 case INLINE_UNPROTECTEVINMENT:
	 default:
		 SingleHook::Hook ( );
	 }
 }