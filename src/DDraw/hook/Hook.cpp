#include "..\oddraw.h"
#include "hook.h"
#include "etc.h"


//SingleHook:
SingleHook::SingleHook ()
{
	InitHookClass ( NULL, 0, 0);
	Hook ( );
}

SingleHook::SingleHook (LPBYTE AddrToHook_Pvoid, DWORD Len_Dw, int HookMode, LPBYTE NewBytes_Pbyte)
{
	InitHookClass ( AddrToHook_Pvoid, Len_Dw, HookMode, NewBytes_Pbyte);
	Hook ( );
}

SingleHook::SingleHook (unsigned int AddrToHook_Pvoid, DWORD Len_Dw, int HookMode, LPBYTE NewBytes_Pbyte)
{
	InitHookClass ( reinterpret_cast<LPBYTE>(AddrToHook_Pvoid), Len_Dw, HookMode, NewBytes_Pbyte);
	Hook ( );
}

SingleHook::~SingleHook ()
{	
	if (NULL!=m_OrgBytes_Pbyte)
	{
		UnHook ( );
		delete []m_OrgBytes_Pbyte;
		m_OrgBytes_Pbyte= NULL;
	}
	if (NULL!=mallocedBuf)
	{
		delete [] mallocedBuf;
		mallocedBuf= NULL;
	}
}

void SingleHook::Hook(void)
{	
	if ((0<m_LenToModify_Dw)
		&&(NULL!=m_NewBytes_Pbyte))
	{
		MemWriteWithBackup ( AddrToHook (), m_LenToModify_Dw, NULL, m_NewBytes_Pbyte);
	}
}

void SingleHook::UnHook (void)
{
	if ((0<m_LenToModify_Dw)
		&&(NULL!=m_OrgBytes_Pbyte))
	{
		MemWriteWithBackup ( AddrToHook (), m_LenToModify_Dw, NULL, m_OrgBytes_Pbyte);
	}
}

LPVOID SingleHook::AddrToHook (PVOID Addr_Pvoid)
{
	m_AddrToHook= Addr_Pvoid;
	return m_AddrToHook;
}


int SingleHook::ThisHookMode (int HookMode)
{
	m_HookMode_I= HookMode;
	return m_HookMode_I;
}

const int SingleHook::ThisHookMode (void)
{
	return m_HookMode_I;
}

const LPVOID SingleHook::AddrToHook (void)
{
	return m_AddrToHook;
}
DWORD SingleHook::LenToHook (DWORD Len_Dw)
{
	m_LenToModify_Dw= Len_Dw;
	return m_LenToModify_Dw;
}

LPVOID SingleHook::AddrToRtn (void)
{
	return reinterpret_cast<LPBYTE>(AddrToHook ( ))+ m_LenToModify_Dw;
}

void SingleHook::InitHookClass (LPBYTE AddrToHook_Pvoid, DWORD Len_Dw, int HookMode, LPBYTE NewBytes_Pbyte)
{
	mallocedBuf= NULL;
	m_NewBytes_Pbyte= NULL;
	m_LenToModify_Dw= 0;
	AddrToHook ( AddrToHook_Pvoid);
	ThisHookMode ( HookMode);

	m_OrgBytes_Pbyte= NULL;

	if (NULL==AddrToHook ())
	{
		return ;
	}

	if (HookMode==INLINE_UNPROTECTEVINMENT)
	{
		m_NewBytes_Pbyte= NewBytes_Pbyte;
		m_LenToModify_Dw= Len_Dw;

		//和dafault的申请org byte是一样的。
		m_OrgBytes_Pbyte= new BYTE[Len_Dw];
		memcpy ( m_OrgBytes_Pbyte, AddrToHook_Pvoid, Len_Dw);
	}
	else if (HookMode==INLINE_SINGLEJMP)
	{
		if (Len_Dw<0x5)
		{
			return ;
		}

		mallocedBuf= new BYTE [Len_Dw];
		memset ( mallocedBuf, 0x90, Len_Dw);
		m_LenToModify_Dw= Len_Dw;

		mallocedBuf[0]= 0xe9;

		*((DWORD *)&mallocedBuf [1])= (DWORD)((NewBytes_Pbyte)- AddrToHook_Pvoid - 0x5);

		m_NewBytes_Pbyte= mallocedBuf;

		m_OrgBytes_Pbyte= new BYTE[Len_Dw];
		memcpy ( m_OrgBytes_Pbyte, AddrToHook_Pvoid, Len_Dw);
	}
}