#ifndef UNICODESUPPORT_HAYU32Y

#define UNICODESUPPORT_HAYU32Y
#ifndef __DDRAW_INCLUDED__
	struct IDirectDrawSurface;
	typedef struct IDirectDrawSurface FAR *LPDIRECTDRAWSURFACE;
	struct IDirectDraw;
	typedef struct IDirectDraw FAR *LPDIRECTDRAW;

#endif

	class InlineSingleHook;
	struct _OFFSCREEN;
	typedef struct _OFFSCREEN OFFSCREEN;

#ifndef _WINDEF_
	struct tagRECT;
	typedef struct tagRECT *LPRECT;
	typedef unsigned int UINT; 
#endif

	typedef struct tagSpecScreenSurface
	{
		DWORD Width;
		DWORD Height;
		DWORD lPitch;
		LPVOID SurfaceMem_Pvoid;
		int SurfaceMemBytesLen;
		LPDIRECTDRAWSURFACE DDSurface_P;
		HDC mySurface_HDC;
		DDSURFACEDESC DDSurfaceDescForLock;
		LPDIRECTDRAW myDDraw_Ptr;
		struct tagSpecScreenSurface * Next;
	} SpecScreenSurface, * PSpecScreenSurface;

/*
	typedef struct _VisibleWnd
	{
		HWND hwnd;
		HDC hdc;
		RECT rect;
	}VisibleWnd, * PVisibleWnd;
*/

	class UnicodeSupport 
	{
	public:
	/*	vector<VisibleWnd> VisibleWnd_Vec;*/
		BOOL ImeShowing;
		BOOL UnicodeValid;

		SIZE FontExtent;
	//*公共函数 *//
	public:	
		UnicodeSupport(LPCSTR FontName, DWORD Color, DWORD Background, BOOL VidMem_a= TRUE);
		UnicodeSupport (BOOL VidMem_a= TRUE);
		~UnicodeSupport();
		LPDIRECTDRAWSURFACE CreateSurfaceFromContextScreen (OFFSCREEN * OFFSCREEN_Ptr, LPDIRECTDRAW lpDDraw, LPDIRECTDRAWSURFACE * lplpDDSurface);

		BOOL UnicodeDrawTextA (OFFSCREEN * OFFSCREEN_Ptr, char * Str_cstrp, LPDIRECTDRAW TADDrawArgc_lp, int X_Off, int Y_Off);
	    bool Message(HWND WinProcWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
		bool Blt (LPDIRECTDRAWSURFACE DescSurface);
		
		void Set (BOOL VidMem_a);
	//*内部变量 *//
	private:

		BOOL VidMem;

		HFONT UnicodeWhiteFont;
		HFONT UnicodeYellowFont;
		LPDIRECTDRAW TADDraw_lp;
		
		InlineSingleHook * DrawTextInScreen_ISH;
		InlineSingleHook * ValidChar_ISH;
		InlineSingleHook * DeleteChar_ISH;
		InlineSingleHook * DBSC2rdByte_ISH;
		InlineSingleHook * Blt_BottomState0_Text_ISH;
		InlineSingleHook * Blt_BottomState1_Text_ISH;
		InlineSingleHook * PopadState_ISH;
		InlineSingleHook * GetStrExtent_ISH;
		InlineSingleHook * GetTextExtent_AssignCharLen_ISH;
/*
		InlineSingleHook * CreateWindowExW_ISH;
*/

		PSpecScreenSurface UnicodeFontDrawCache;

		HIMC hIMC; 
		HIMC Orginal_hIMC;

		int CursorX;
		int CursorY;
		int xPos;
		int yPos;
		int Width;
		int Height;
		int CharStartHeight;
		int CharElemHeight;

		COLORREF FontColor;
		COLORREF HDCBackground;

		DWORD ImeSurfaceBackground;
		LPDIRECTDRAWSURFACE lpImeSurface;

		char IMEName[0x100];
		//char CompositingBuf[0x100];
		char InputStrBuf[0x200];

		char CompReadStr[0x100];

		std::vector<LPSTR> CandidateList;
		LPCANDIDATELIST lpCandList;
		DWORD CurrentCandListLen;

		BOOL LMouseDown;
		
	//*内部函数 *//
	private:
		PSpecScreenSurface NewSpecScreenSurface (OFFSCREEN * OFFSCREEN_Ptr);
		void FreeSpecScreenSurface (PSpecScreenSurface ForFree_PSSS);
		PSpecScreenSurface GetSpecScreenSurface (OFFSCREEN * OFFSCREEN_Ptr);

		HWND GetMaxChildRect (HWND InHwnd, RECT * Rtn_Rect);
		void ReleaseCandidateList (void);

		bool UpdateImeFrame (void);
		void DrawALine_Ime (HDC ImeHdc, LPSTR StrBuf, int * Curt_Width, int * Curt_Height);
		int DrawSeparator (HDC ImeHdc, int Length, int * Curt_Width, int * Curt_Height, HBRUSH Brush= (HBRUSH) COLOR_BTNHIGHLIGHT);

		void RestoreLocalSurf (  BOOL VidMem_a);
		BOOL IsIDDrawLost (void);
		void SendStr (char * InputStrBuf);
		void SetHDCFont (HDC Hdc, HFONT font);
		bool UpdateImeInput (void);
		void freelpImeSurface (void);
	};

	void CopyToContextScreenMem (OFFSCREEN * OFFSCREEN_Ptr, PSpecScreenSurface SrcSpecScreenSurface);
	void CopyToSpecSurfaceMem (PSpecScreenSurface SrcSpecScreenSurface, OFFSCREEN * OFFSCREEN_Ptr);

	int USDrawTextInScreen_HookRouter (PInlineX86StackBuffer X86StrackBuffer);

	int myDeleteChar (PInlineX86StackBuffer X86StrackBuffer);
	int myValidChar  (PInlineX86StackBuffer X86StrackBuffer);
	int DBSC2rdByte (PInlineX86StackBuffer X86StrackBuffer);
	int __stdcall Blt_BottomState1_Text (PInlineX86StackBuffer X86StrackBuffer);
	int __stdcall Blt_BottomState0_Text (PInlineX86StackBuffer X86StrackBuffer);
	int __stdcall PopadState (PInlineX86StackBuffer X86StrackBuffer);
	int __stdcall GetStrExtent (PInlineX86StackBuffer X86StrackBuffer);
	int __stdcall GetTextExtent_AssignCharLen(PInlineX86StackBuffer X86StrackBuffer);
	
	extern LPSTR IME_NOTIFY_STR[];
	extern LPSTR IME_COMPOSITION_STR[];
/*

	extern unsigned int CreateWindowExW_rtn;
	extern unsigned int InnerCreateWindow;
	int __stdcall CreateWindowExW_new (PInlineX86StackBuffer X86StrackBuffer);
	*/
	extern UnicodeSupport* NowSupportUnicode;
#endif //UNICODESUPPORT_HAYU32Y