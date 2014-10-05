#ifndef HOOK_H74DJN6554FD
#define HOOK_H74DJN6554FD

	class SingleHook
	{//这个类是Hook的elmt，功能是实现具体的一个HOOK。
	public:
		LPVOID m_AddrToHook;
		int m_HookMode_I;
		LPBYTE m_NewBytes_Pbyte;
		LPBYTE m_HookRouter_Pproc;
		LPBYTE m_OrgBytes_Pbyte;
		DWORD m_LenToModify_Dw;
		LPBYTE mallocedBuf;

	public:
		SingleHook ();
		SingleHook (LPBYTE AddrToHook_Pvoid, DWORD Len_Dw, int HookMode= 0, LPBYTE NewBytes_Pbyte= NULL);//
		SingleHook (unsigned int AddrToHook_Pvoid, DWORD Len_Dw, int HookMode= 0, LPBYTE NewBytes_Pbyte= NULL);

		~SingleHook();

		LPVOID AddrToRtn (void);

		virtual void Hook (void);
		virtual void UnHook (void);

		virtual LPVOID AddrToHook (LPVOID Addr_Pvoid);
		virtual int ThisHookMode (int HookMode);
		virtual const int ThisHookMode (void);
		virtual const LPVOID AddrToHook (void);
		virtual DWORD LenToHook (DWORD Len_D);
	//private:
		virtual void InitHookClass (LPBYTE AddrToHook_Pvoid, DWORD Len_Dw, int HookMode= 0, LPBYTE NewBytes_Pbyte= NULL);;
		
	};

	//INLINE_UNPROTECTEVINMENT
	//不保护环境的hookRouter的new byte需要自己实现。
	// INLINE_5BYTESNOREDIECTCALL (0x1004)
	// INLINE_5BYTESNOREDIECTJMP (0x1005)会保护环境，但不会把覆盖了的代码复制来自动执行。
	//	#define INLINE_5BYTESCALL 0x1001
	//  #define INLINE_5BYTESJMP 0x1002
	//这个是inlineHOOK的保护环境的hookrouter所必须要用的格式!
	// 	HOOKROUTERBEGIN_NAKED(blablabalabla)
	// 		AfxMessageBox ( _T("这儿是HOOK的函数体"));
	// 	HOOKROUTEREND(blablabalabla)
	//  在函数体里，堆栈和寄存器和HOOK地方的一模一样。只要不修改低处堆栈(esp+4之类)的值，就不会破坏堆栈和寄存器环境，也不会修改那些值。
	//  而且要保证这个hookRouter只被一个inlineHook使用！！！不然可能线程不同步了。

	class InlineSingleHook;
	typedef struct tagInlineX86StackBuffer
	{
		LPVOID rtnAddr_Pvoid;
		DWORD EFlags_Dw;
		DWORD Edi;
		DWORD Esi;
		DWORD Ebp;
		DWORD Esp;
		DWORD Ebx;
		DWORD Edx;
		DWORD Ecx;
		DWORD Eax;
		DWORD RtnEsp;//额外保存的一个
		int EnteredFlag_I;
		DWORD TID_Dw;
		InlineSingleHook * myInlineHookClass_Pish;
		tagInlineX86StackBuffer * next;
	}InlineX86StackBuffer, * PInlineX86StackBuffer;
	PInlineX86StackBuffer __stdcall X86CurrentThreadStackBufferFixRtnAddr (PInlineX86StackBuffer * in_Pix86StackBuf);
	PInlineX86StackBuffer __stdcall X86CurrentThreadStackBuffer (PInlineX86StackBuffer * in_Pix86StackBuf);

#define INLINE_5BYTESLAGGERCALL (0x1001)
#define INLINE_5BYTESLAGGERJMP (0x1002)
#define INLINE_UNPROTECTEVINMENT (0x1003)

#define INLINE_5BYTESNOREDIECTCALL (0x1004)
#define INLINE_5BYTESNOREDIECTJMP (0x1005)

#define INLINE_MODIFYCODE (0x1006)
#define INLINE_SINGLEJMP (0x1007)

#define ERRORMODE 0x99987

#define X86STRACKBUFFERCHANGE 0x7798FFAA // HOOK ROUTER中修改了返回地址时候必须返回这个！修改了寄存器的值时候不用返回这个，修改堆栈的值也不用。但你自己设置了返回地址，就要自己备份那些被HOOK掉的指令了。可以从InlineSingleHook::RedirectedOpcodeInStub获得stub中返回位置的代码。



	typedef int  (__stdcall * InlineX86HookRouter) (PInlineX86StackBuffer X86StrackBuffer);

	class InlineSingleHook: public SingleHook
	{
	public:
		LPVOID ParamOfHook;
	private:
		LPBYTE m_RedirectOrgOpcodes_Pbyte;
		LPBYTE m_RedirectOrgOpcodesPtrInStub_Pvoid;
		
		BYTE Inline_5Bytes[0x10];

	public:
		LPBYTE NewBytesForInlineHook (LPBYTE NewBytes_Pvoid= NULL);
		InlineSingleHook ();
		InlineSingleHook (unsigned int AddrToHook_Pvoid, DWORD Len_Dw, int HookMode= 0, LPVOID RouterAddr= NULL);
		InlineSingleHook (unsigned int AddrToHook_Pvoid, DWORD Len_Dw, int HookMode, InlineX86HookRouter RouterAddr);
		~InlineSingleHook ();

		
		const LPVOID RtnAddrOfHook (void);
		const LPBYTE RedirectedOrgOpcodes (void);
		const LPVOID RedirectedOpcodeInStub (void);
		LPVOID SetParamOfHook (LPVOID ParamOfHook_Pvoid);

		virtual void Hook (void);
		
	private:
		void InitHookClass (const LPBYTE AddrToHook_Pvoid, DWORD Len_Dw, int HookMode= 0, LPVOID RouterAddr= NULL, LPVOID ParamOfHook_Pvoid= NULL);
	};

	

	class ModifyHook: public SingleHook
	{
		//public:

	private:
		
		LPBYTE m_RedirectOrgOpcodes_Pbyte;
		LPBYTE m_RedirectOrgOpcodesPtrInStub_Pvoid;
		DWORD m_OverWriteLen;
		DWORD m_OffToRedirect;
	public:
		ModifyHook ();

		ModifyHook (LPBYTE AddrToHook_Pvoid, int HookMode, DWORD MaxLen, LPBYTE OverWriteOpcode, DWORD OverWriteLen, DWORD OffToRedirect= 0);
		ModifyHook (unsigned int AddrToHook_Pvoid, int HookMode, DWORD MaxLen, LPBYTE OverWriteOpcode, DWORD OverWriteLen, DWORD OffToRedirect= 0);
		~ModifyHook ();

		virtual void Hook (void);

	private:
		void InitHookClass (LPBYTE AddrToHook_Pvoid, int HookMode, DWORD MaxLen, LPBYTE OverWriteOpcode, DWORD OverWriteLen, DWORD OffToRedirect= 0);
	};

#endif //HOOK_H74DJN6554FD