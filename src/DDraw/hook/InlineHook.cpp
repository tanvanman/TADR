#include "..\oddraw.h"
#include "hook.h"
#include "etc.h"
//InlineSingleHook:
//禁止掉格式转化的warn提示，这个的原因是确定了转化没问题的:
#pragma warning(disable:4311)
#pragma warning(disable:4312)
//
typedef PInlineX86StackBuffer (__stdcall *  defPX86CurrentThreadStackBuffer) (PInlineX86StackBuffer *);
defPX86CurrentThreadStackBuffer Global_PX86CurrentThreadStackBufferFixRtnAddr= X86CurrentThreadStackBufferFixRtnAddr;
defPX86CurrentThreadStackBuffer Global_PX86CurrentThreadStackBuffer= X86CurrentThreadStackBuffer;
const int Global_OffOfEnteredFlagFromInlineX86StackBuffer= (int)&((PInlineX86StackBuffer)0)->EnteredFlag_I;

void __declspec(naked) X86InlineHookRouter (void)
{//多线程的时候就出错了。需要保存的堆栈做成动态申请的内存，以及一个是否已进入过HOOK的标志。
 //重入的情况用一个标志处理过
	//__asm lea esp, DWORD PTR [esp+ 4]//当是jmp的时候，这个堆栈平衡。是call类型这儿要NOP掉
#define X86INLINEROUTERSTRACKADD4 0x0
	__asm lea esp, DWORD PTR [esp+ 4]//当是JMP的时候，这个堆栈平衡。是jmp类型这儿要NOP掉

	__asm pushad
	__asm pushfd

	__asm call TempAddr

#define X86INLINEROUTERSTACKBUFFEROFF 0xb
addrOfStrackBuffer:
	__asm __emit 0x00  ;//这儿的4个0是填充为以后放pushfd/ad的东西用的。 off 0x7
	__asm __emit 0x00
	__asm __emit 0x00
	__asm __emit 0x00
TempAddr:
 	__asm pop ebx
	__asm push ebx
	__asm call [Global_PX86CurrentThreadStackBufferFixRtnAddr]

	__asm mov ebx, eax;

	__asm mov eax, Global_OffOfEnteredFlagFromInlineX86StackBuffer;
	__asm mov eax, DWORD PTR [ebx+ eax]
	__asm cmp eax, 0xbd88
	__asm je AfterCallRouter
	__asm mov eax, Global_OffOfEnteredFlagFromInlineX86StackBuffer;
	__asm mov DWORD PTR [ebx+ eax], 0xbd88
	__asm mov ecx, 0x9 //多保存1个堆栈中的数据。以前写的0xa，不改了。
	__asm lea edi, DWORD PTR[ebx+ 4]
	__asm mov esi, esp
	__asm rep movsd

#define X86INLINEROUTERSTRACKSUB4 0x44
	__asm push ebx //移动一下堆栈
	__asm mov edi, esp//恢复寄存器
	__asm lea esi, DWORD PTR[ebx+ 0x4]
	__asm mov ecx, 0x9
	__asm rep movsd
	__asm mov DWORD PTR[esp+ 0x9* 0x4], ebx;
	__asm popfd
	__asm popad


	__asm __emit 0xe8  ;//这儿的也是要被填充的
#define X86INLINEROUTERCALLADDROFF 0x58
	__asm __emit 0x00  ;//这儿的4个0是填充为以后保存要调用的目标PROC用的。
	__asm __emit 0x00
	__asm __emit 0x00
	__asm __emit 0x00
	
	__asm mov edi, eax//保存hookrouter返回值

	__asm call OffToStrackBuffer
OffToStrackBuffer:
	__asm pop ebx
	__asm sub ebx, offset OffToStrackBuffer
	__asm add ebx, offset addrOfStrackBuffer
	__asm push ebx
	__asm call [Global_PX86CurrentThreadStackBuffer]

	__asm mov ebx, eax;

	__asm mov esp, DWORD PTR[ebx]InlineX86StackBuffer.Esp;//popad修改不了esp,只是+0x20，需要自己设置好ESP。

	__asm cmp edi, X86STRACKBUFFERCHANGE//判断hook router返回值
	__asm jne NormalJmpBack
	//修改了STRACKBUFFER，返回地址就要看hook router里指定的是什么了。但寄存器还是要恢复成HOOK ROUTER参数里的那个结构中的值。

	__asm mov eax, Global_OffOfEnteredFlagFromInlineX86StackBuffer;
	__asm mov DWORD PTR [ebx+ eax], 0//释放这个位置的标志

	

	__asm push DWORD PTR [ebx]
	__asm sub esp, 0x9* 0x4
	__asm mov ecx, 0x9
	__asm mov edi, esp
	__asm lea esi, DWORD PTR[ebx+ 4]
	__asm rep movsd
	__asm popfd
	__asm popad;

	__asm retn

NormalJmpBack:
	__asm sub esp, 0x9* 0x4//把pushad pushfd和call的堆栈空间减出来,jmp会改成0x9。
	__asm mov ecx, 0x9 //恢复堆栈
	__asm mov edi, esp
	__asm lea esi, DWORD PTR[ebx+ 4]
	__asm rep movsd

AfterCallRouter:
	__asm mov eax, Global_OffOfEnteredFlagFromInlineX86StackBuffer;
	__asm mov DWORD PTR [ebx+ eax], 0//释放这个位置的标志

	__asm popfd
	__asm popad;
	//这儿的没返回，是为以后放原始代码+ jmp/call返回的代码用的。
#define X86INLINEROUTERENDOFF 0xC1
}

PInlineX86StackBuffer __stdcall X86CurrentThreadStackBufferFixRtnAddr (PInlineX86StackBuffer * in_Pix86StackBuf)
{
	PInlineX86StackBuffer X86StackBufferAry= *in_Pix86StackBuf;
	DWORD ThreadID_Dw= GetCurrentThreadId ( );
	PInlineX86StackBuffer temp_Pix86Stackbuf= X86StackBufferAry;
	while ((NULL!=temp_Pix86Stackbuf))
	{
		if (ThreadID_Dw==temp_Pix86Stackbuf->TID_Dw)
		{
			temp_Pix86Stackbuf->rtnAddr_Pvoid= temp_Pix86Stackbuf->myInlineHookClass_Pish->RtnAddrOfHook();
			return temp_Pix86Stackbuf;
		}
		temp_Pix86Stackbuf= temp_Pix86Stackbuf->next;
	}

	temp_Pix86Stackbuf= new InlineX86StackBuffer;
	ZeroMemory ( temp_Pix86Stackbuf, sizeof(InlineX86StackBuffer));
	temp_Pix86Stackbuf->TID_Dw= ThreadID_Dw;
	temp_Pix86Stackbuf->next= X86StackBufferAry;
	temp_Pix86Stackbuf->myInlineHookClass_Pish= X86StackBufferAry->myInlineHookClass_Pish;
	temp_Pix86Stackbuf->rtnAddr_Pvoid= temp_Pix86Stackbuf->myInlineHookClass_Pish->RtnAddrOfHook();
	
	*in_Pix86StackBuf= temp_Pix86Stackbuf;

	return temp_Pix86Stackbuf;
}

PInlineX86StackBuffer __stdcall X86CurrentThreadStackBuffer (PInlineX86StackBuffer * in_Pix86StackBuf)
{

	PInlineX86StackBuffer X86StackBufferAry= *in_Pix86StackBuf;
	DWORD ThreadID_Dw= GetCurrentThreadId ( );
	PInlineX86StackBuffer temp_Pix86Stackbuf= X86StackBufferAry;
	while ((NULL!=temp_Pix86Stackbuf))
	{
		if (ThreadID_Dw==temp_Pix86Stackbuf->TID_Dw)
		{
			return temp_Pix86Stackbuf;
		}
		temp_Pix86Stackbuf= temp_Pix86Stackbuf->next;
	}

	temp_Pix86Stackbuf= new InlineX86StackBuffer;
	ZeroMemory ( temp_Pix86Stackbuf, sizeof(InlineX86StackBuffer));
	temp_Pix86Stackbuf->TID_Dw= ThreadID_Dw;
	temp_Pix86Stackbuf->next= X86StackBufferAry;
	temp_Pix86Stackbuf->myInlineHookClass_Pish= X86StackBufferAry->myInlineHookClass_Pish;

	*in_Pix86StackBuf= temp_Pix86Stackbuf;


	return temp_Pix86Stackbuf;
}


InlineSingleHook::InlineSingleHook ()
{
	InitHookClass ( NULL, 5, 0, NULL);
	if (NULL!=m_NewBytes_Pbyte)
	{
		Hook ( );
	}
}


InlineSingleHook::InlineSingleHook (unsigned int AddrToHook_Pvoid, DWORD Len_Dw, int HookMode, InlineX86HookRouter RouterAddr)
{
	InitHookClass ( reinterpret_cast<LPBYTE> (AddrToHook_Pvoid), Len_Dw, HookMode, (LPVOID)RouterAddr);
	if (NULL!=m_NewBytes_Pbyte)
	{
		Hook ( );
	}
}

InlineSingleHook::InlineSingleHook (unsigned int AddrToHook_Pvoid, DWORD Len_Dw, int HookMode, LPVOID RouterAddr)
{
	InitHookClass ( reinterpret_cast<LPBYTE> (AddrToHook_Pvoid), Len_Dw, HookMode, RouterAddr);
	if (NULL!=m_NewBytes_Pbyte)
	{
		Hook ( );
	}
}

InlineSingleHook::~InlineSingleHook ()
{
// 	if (Inline_5Bytes!=m_NewBytes_Pbyte)
// 	{//外部的buffer不用管
// 
// 	}
	if (NULL!=m_OrgBytes_Pbyte)
	{
		UnHook ( );
		delete []m_OrgBytes_Pbyte;
		m_OrgBytes_Pbyte= NULL;
	}


	if (NULL!=m_HookRouter_Pproc)
	{
		PInlineX86StackBuffer X86StackBufferAry= *((PInlineX86StackBuffer *)&m_HookRouter_Pproc[X86INLINEROUTERSTACKBUFFEROFF]);
		while (NULL!=X86StackBufferAry)
		{
			PInlineX86StackBuffer temp_Ix86Stackbuf= X86StackBufferAry->next;
			delete [] X86StackBufferAry;
			X86StackBufferAry= temp_Ix86Stackbuf;
		}

		delete [] m_HookRouter_Pproc;
		m_HookRouter_Pproc= NULL;
	}
	if (NULL!=m_RedirectOrgOpcodes_Pbyte)
	{
		delete [] m_RedirectOrgOpcodes_Pbyte;
		m_RedirectOrgOpcodes_Pbyte= NULL;
	}
}

LPBYTE InlineSingleHook::NewBytesForInlineHook (LPBYTE NewBytes_Pvoid)
{
	LPBYTE tempRtn_Pbyte= m_NewBytes_Pbyte;
	m_NewBytes_Pbyte= NewBytes_Pvoid;
	return tempRtn_Pbyte;
}

LPVOID InlineSingleHook::SetParamOfHook (LPVOID ParamOfHook_Pvoid)
{
	LPVOID rtn= ParamOfHook;
	ParamOfHook= ParamOfHook_Pvoid;
	return rtn;
}

void InlineSingleHook::InitHookClass (const LPBYTE AddrToHook_Pvoid, DWORD Len_Dw, int HookMode, LPVOID RouterAddr, LPVOID ParamOfHook_Pvoid)
{
	m_NewBytes_Pbyte= NULL;
	m_LenToModify_Dw= 0;
	AddrToHook ( AddrToHook_Pvoid);
	ThisHookMode ( HookMode);
	ParamOfHook= ParamOfHook_Pvoid;

	ZeroMemory ( Inline_5Bytes, sizeof(Inline_5Bytes)* sizeof(BYTE));

/*
	if (! IsBadWritePtr ( , 1))
	{//怎么判断m_OrgBytes_Pbyte是不是有效的指针呢？
		delete [] m_OrgBytes_Pbyte;
	}
*/
	m_OrgBytes_Pbyte= NULL;

	if (NULL==AddrToHook ())
	{
		return ;
	}

	switch (HookMode)
	{
	case INLINE_UNPROTECTEVINMENT:
		//
		SingleHook::InitHookClass ( AddrToHook_Pvoid, Len_Dw, HookMode);
		break;
	case INLINE_5BYTESNOREDIECTCALL:
	case INLINE_5BYTESLAGGERCALL:
		if (NULL==m_NewBytes_Pbyte)
		{//还没有指定NewBytes_Pvoid,就自己初始化一个。
			Inline_5Bytes[0]= 0xe8;
			m_NewBytes_Pbyte= Inline_5Bytes;
		}
		
	case INLINE_5BYTESNOREDIECTJMP:
	case INLINE_5BYTESLAGGERJMP:

		if (5>Len_Dw)
		{//JMP和CALL方式的不能在5字节以内实现。
			HookMode= ERRORMODE;
		}	

		if (NULL==m_NewBytes_Pbyte)
		{//还没有指定NewBytes_Pvoid,就自己初始化一个。
			Inline_5Bytes[0]= 0xe9;
			m_NewBytes_Pbyte= Inline_5Bytes;
		}
		
		//这儿是初始化HOOK的wapper m_HookRouter_Pproc
		DWORD HookRouterLen;
		HookRouterLen= X86INLINEROUTERENDOFF+ Len_Dw* 0x4+ 0x4+ 0x5;
		m_HookRouter_Pproc= new BYTE[HookRouterLen];//X86InlineHookRouter+ orgcode+ jmp back, orgcode可能全是要被扩充4被大小的jecxz
		LPBYTE CurrentPtrIn_m_HookRouter_Pproc;
		DWORD tempForProtect_Dw;
		VirtualProtect ( m_HookRouter_Pproc, HookRouterLen, PAGE_EXECUTE_READWRITE, &tempForProtect_Dw);
		
#ifdef DEBUG
		//VC的debug模式，调用的函数都是0xe9 ?? ?? ?? ??这样的地址。
		LPBYTE tempCopyFrom_Pproc;
		tempCopyFrom_Pproc= *((LPBYTE *)((DWORD)&X86InlineHookRouter+ 1));
		tempCopyFrom_Pproc= (DWORD)&X86InlineHookRouter+ tempCopyFrom_Pproc+ 5;
		memcpy ( m_HookRouter_Pproc, tempCopyFrom_Pproc, X86INLINEROUTERENDOFF);
#else
		memcpy ( m_HookRouter_Pproc, X86InlineHookRouter, X86INLINEROUTERENDOFF);
#endif

		CurrentPtrIn_m_HookRouter_Pproc= &m_HookRouter_Pproc[X86INLINEROUTERSTACKBUFFEROFF]; 
		PInlineX86StackBuffer tempPtrToX86StackBufForRouter;
		tempPtrToX86StackBufForRouter= new InlineX86StackBuffer;
		ZeroMemory ( tempPtrToX86StackBufForRouter, sizeof(InlineX86StackBuffer));
		tempPtrToX86StackBufForRouter->rtnAddr_Pvoid= RtnAddrOfHook ();
		tempPtrToX86StackBufForRouter->myInlineHookClass_Pish= this;
		*((PInlineX86StackBuffer *)CurrentPtrIn_m_HookRouter_Pproc)= tempPtrToX86StackBufForRouter;//设置保存堆栈的缓冲地址。
		
		if (INLINE_5BYTESLAGGERJMP==HookMode)
		{//当JMP==hookmode时，把恢复堆栈+4的代码nop掉。
			CurrentPtrIn_m_HookRouter_Pproc= &m_HookRouter_Pproc[X86INLINEROUTERSTRACKADD4];
			*((DWORD *)CurrentPtrIn_m_HookRouter_Pproc)= 0x90909090u;//设置为4字节(lea esp, DWORD PTR[esp+ 4])的nop nop nop nop
// 
// 			CurrentPtrIn_m_HookRouter_Pproc= &m_HookRouter_Pproc[X86INLINEROUTERSTRACKSUB4];
// 			*((BYTE *)CurrentPtrIn_m_HookRouter_Pproc)= 0x90u;//设置为4字节(lea esp, DWORD PTR[esp+ 4])的nop nop nop nop
		}

		CurrentPtrIn_m_HookRouter_Pproc= &m_HookRouter_Pproc[X86INLINEROUTERCALLADDROFF];
		*((DWORD *)CurrentPtrIn_m_HookRouter_Pproc)= (DWORD) ((LPBYTE)RouterAddr- &m_HookRouter_Pproc[X86INLINEROUTERCALLADDROFF]+ 1- 0x5);//设置HOOK中跳转往的目标。注意X86INLINEROUTERCALLADDROFF不是指向的0xe8，而是0xe8之后1个字节，需要+1


		CurrentPtrIn_m_HookRouter_Pproc= &m_HookRouter_Pproc[X86INLINEROUTERENDOFF];

		//Len_Dw= 0;

		if ((INLINE_5BYTESLAGGERCALL==HookMode)||
			INLINE_5BYTESLAGGERJMP==HookMode)
		{
			DWORD RedirectOpCodeLen;
			m_RedirectOrgOpcodes_Pbyte= NULL;
			RedirectOpCodeLen= X86RedirectOpcodeToNewBase ( CurrentPtrIn_m_HookRouter_Pproc, AddrToHook_Pvoid, &Len_Dw, &m_RedirectOrgOpcodes_Pbyte);

			m_RedirectOrgOpcodesPtrInStub_Pvoid= CurrentPtrIn_m_HookRouter_Pproc;
			memcpy ( CurrentPtrIn_m_HookRouter_Pproc, m_RedirectOrgOpcodes_Pbyte, RedirectOpCodeLen);
			CurrentPtrIn_m_HookRouter_Pproc= CurrentPtrIn_m_HookRouter_Pproc+ RedirectOpCodeLen;
		}

		//if (INLINE_5BYTESLAGGERCALL==HookMode||INLINE_5BYTESLAGGERJMP==HookMode)
		{//jmp这样跳过来的，就jmp回去
		//其他的类型不可能在这儿出现。
			CurrentPtrIn_m_HookRouter_Pproc[0]= 0xe9;
			*((DWORD *)&CurrentPtrIn_m_HookRouter_Pproc [1])= (DWORD)((AddrToHook_Pvoid+ Len_Dw)- CurrentPtrIn_m_HookRouter_Pproc - 0x5);
		}
		
		*((DWORD *)(&Inline_5Bytes[1]))= (DWORD) (reinterpret_cast <LPBYTE>(m_HookRouter_Pproc)- AddrToHook_Pvoid- 0x5);
	default:
		if (0<Len_Dw)
		{
			m_OrgBytes_Pbyte= new BYTE[Len_Dw];
			memcpy ( m_OrgBytes_Pbyte, AddrToHook_Pvoid, Len_Dw);
		}
		m_LenToModify_Dw= Len_Dw;
		break;
	}
}

void InlineSingleHook::Hook (void)
{
	switch ( ThisHookMode ( ))
	{
	case INLINE_5BYTESNOREDIECTCALL:
	case INLINE_5BYTESNOREDIECTJMP:
	case INLINE_5BYTESLAGGERCALL:
	case INLINE_5BYTESLAGGERJMP:
		LPBYTE nopBts;
		nopBts= new BYTE[m_LenToModify_Dw];
		memset ( nopBts, 0x90, m_LenToModify_Dw);
		
		MemWriteWithBackup ( AddrToHook (), m_LenToModify_Dw, NULL, nopBts);
		MemWriteWithBackup ( AddrToHook (), 5, NULL, m_NewBytes_Pbyte);
		delete [] nopBts;
		break;
	case INLINE_UNPROTECTEVINMENT:
	default:
		SingleHook::Hook ( );
	}
}


const LPVOID InlineSingleHook::RtnAddrOfHook (void)
{
	return (const LPBYTE)AddrToHook ()+ m_LenToModify_Dw;
}

const LPBYTE InlineSingleHook::RedirectedOrgOpcodes (void)
{
	return m_RedirectOrgOpcodes_Pbyte;
}

const LPVOID InlineSingleHook::RedirectedOpcodeInStub (void)
{
	return m_RedirectOrgOpcodesPtrInStub_Pvoid;
}