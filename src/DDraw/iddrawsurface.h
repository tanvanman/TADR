//---------------------------------------------------------------------------
#ifndef iddrawsurfaceH
#define iddrawsurfaceH

#include <ddraw.h>

// shall to reduce internation between class
//class ExternQuickKey;
//---------------------------------------------------------------------------

enum TAProcessEnum
{
	TALobby= 1,
	TALoading,
	TAInGame,
	TAExiting
};

#define TADRCONFIGREGNAME ("Moo")


//#define DEBUG_INFO

class AlliesWhiteboard;
class CIncome;
class CTAHook ;
class CWarp ;
class CMapRect;
class CIdleUnits;
class Dialog;
class CChangeQueue;
class CDDDTA;


#define DDRAW_INIT_STRUCT(ddstruct) { memset(&ddstruct,0,sizeof(ddstruct)); ddstruct.dwSize=sizeof(ddstruct); }

extern short MouseX;
extern short MouseY;

typedef struct DataShare_
{
	char Chat[100];
	int NewData;
	int DeathStatus[10];
	int TAProgress;
	char PlayerNames[10][20];
	float incomeM[10];
	float incomeE[10];
	float totalM[10];
	float totalE[10];
	int PlayingDemo;
	int allies[10];
	int yehaplayground[10];
	float storedM[10];
	float storedE[10];
	float storageM[10];
	float storageE[10];
	int IsRunning;
	int ehaOff;
	char ToAllies[100];
	int ToAlliesLength;
	char  FromAllies[100];
	int FromAlliesLength;
	int MapX;
	int MapY;
	int OtherMapX[10];
	int OtherMapY[10];
	int F1Disable;
	int CommanderWarp;
	char MapName[100];
	int myCheats;
	int PlayerColors[10];
	int LockOn;
	int MaxUnits;
	int ta3d;
	//int IsWatch;
	int LosViewOn;
	int PlayerDotColors[10];
	unsigned int IniCRC;
}*DataSharePTR;

extern DataShare_* DataShare;
extern void *pMapView;
bool SetupFileMap();

typedef struct LocalShare_
{
	int XPos;
	int YPos;
	WNDPROC TAWndProc;
	int Width;
	int Height;
	int ScreenWidth;
	int ScreenHeight;
	bool CompatibleVersion;
	LPVOID DDrawSurfClass;
	LPVOID TADirectDraw;
	LPVOID DDrawClass;
	LPVOID TADirectDrawFrontSurface;
	LPVOID TADirectDrawBackSurface;
	LPVOID Dialog;
	LPVOID Income;
	LPVOID TAHook;
	LPVOID Whiteboard;
	LPVOID CommanderWarp;
	LPVOID MapRect;
	LPVOID UnitRotate;
	LPVOID ChangeQueue;
	LPVOID DDDTA;

	UINT LocalPlayerID;
	UINT OrgLocalPlayerID;

	LPSTR ModRegName;
	//extern for unicode font;
	//LPVOID TAUnicodeSupport;
}*LocalSharePTR;
extern LocalShare_* LocalShare;

extern HINSTANCE HInstance;

LRESULT CALLBACK WinProc(HWND winprocwnd, UINT msg, WPARAM wparam, LPARAM lparam);

class IDDrawSurface : public IDirectDrawSurface
{
private:
	bool lpBackLockOn;
	bool FrontSurf;
	bool Windowed;
	LPVOID SurfaceMemory;
	int lPitch;
	int dwWidth;
	int dwHeight;
	AlliesWhiteboard * WhiteBoard;
	CIncome * Income;
	CTAHook * TAHook;
	CWarp * CommanderWarp;
	CMapRect * SharedRect;
	Dialog * SettingsDialog;
	CChangeQueue * ChangeQueue;
	CDDDTA * DDDTA;
	
	bool VidMem;
	bool VerticalSync;
	bool PlayingMovie;
	bool DisableDeInterlace;
	LPDIRECTDRAWPALETTE Palette;
	void DeInterlace();
	void CreateFileName(char *Buff, int Num);

	void AddtionInit (void);
	void AddtionRelease (void);

public:
	LPDIRECTDRAWSURFACE lpBack;
	LPDIRECTDRAWSURFACE lpFront;
	LPDIRECTDRAWCLIPPER lpDDClipper;
	LPRGNDATA ScreenRegion;
	LPRGNDATA BattleFieldRegion;
	void CreateClipplist();
	void ScreenShot();
	void CreateDir(char *Dir);
	void CorrectName(char *Name);

	static void OutptTxt(char *string);
	static void OutptInt(int Int_I);
	void Set(bool EnableVSync);
	void FrontSurface (LPDIRECTDRAWSURFACE lpTASurf);
	int ScreenWidth;
	int ScreenHeight;
	IDDrawSurface::IDDrawSurface(LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpTAddsc, LPDIRECTDRAWSURFACE FAR *arg2, IUnknown FAR *arg3,  HRESULT * rtn_p, 
		bool iWindowed, int iScreenWidth, int iScreenHeight);
	/*** IUnknown methods ***/
	HRESULT __stdcall QueryInterface(REFIID riid, LPVOID FAR * ppvObj);
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();
	/*** IDirectDrawSurface methods ***/
	HRESULT __stdcall AddAttachedSurface(LPDIRECTDRAWSURFACE arg1);
	HRESULT __stdcall AddOverlayDirtyRect(LPRECT arg1);
	HRESULT __stdcall Blt(LPRECT arg1, LPDIRECTDRAWSURFACE arg2, LPRECT arg3, DWORD arg4, LPDDBLTFX arg5);
	HRESULT __stdcall BltBatch(LPDDBLTBATCH arg1, DWORD arg2, DWORD arg3);
	HRESULT __stdcall BltFast(DWORD arg1, DWORD arg2, LPDIRECTDRAWSURFACE arg3, LPRECT arg4, DWORD arg5);
	HRESULT __stdcall DeleteAttachedSurface(DWORD arg1, LPDIRECTDRAWSURFACE arg2);
	HRESULT __stdcall EnumAttachedSurfaces(LPVOID arg1, LPDDENUMSURFACESCALLBACK arg2);
	HRESULT __stdcall EnumOverlayZOrders(DWORD arg1, LPVOID arg2, LPDDENUMSURFACESCALLBACK arg3);
	HRESULT __stdcall Flip(LPDIRECTDRAWSURFACE arg1, DWORD arg2);
	HRESULT __stdcall GetAttachedSurface(LPDDSCAPS arg1, LPDIRECTDRAWSURFACE FAR *arg2);
	HRESULT __stdcall GetBltStatus(DWORD arg1);
	HRESULT __stdcall GetCaps(LPDDSCAPS arg1);
	HRESULT __stdcall GetClipper(LPDIRECTDRAWCLIPPER FAR* arg1);
	HRESULT __stdcall GetColorKey(DWORD arg1, LPDDCOLORKEY arg2);
	HRESULT __stdcall GetDC(HDC FAR *arg1);
	HRESULT __stdcall GetFlipStatus(DWORD arg1);
	HRESULT __stdcall GetOverlayPosition(LPLONG arg1, LPLONG arg2);
	HRESULT __stdcall GetPalette(LPDIRECTDRAWPALETTE FAR* arg1);
	HRESULT __stdcall GetPixelFormat(LPDDPIXELFORMAT arg1);
	HRESULT __stdcall GetSurfaceDesc(LPDDSURFACEDESC arg1);
	HRESULT __stdcall Initialize(LPDIRECTDRAW arg1, LPDDSURFACEDESC arg2);
	HRESULT __stdcall IsLost();
	HRESULT __stdcall Lock(LPRECT arg1, LPDDSURFACEDESC arg2, DWORD arg3, HANDLE arg4);
	HRESULT __stdcall ReleaseDC(HDC arg1);
	HRESULT __stdcall Restore();
	HRESULT __stdcall SetClipper(LPDIRECTDRAWCLIPPER arg1);
	HRESULT __stdcall SetColorKey(DWORD arg1, LPDDCOLORKEY arg2);
	HRESULT __stdcall SetOverlayPosition(LONG arg1, LONG arg2);
	HRESULT __stdcall SetPalette(LPDIRECTDRAWPALETTE arg1);
	HRESULT __stdcall Unlock(LPVOID arg1);
	HRESULT __stdcall UpdateOverlay(LPRECT arg1, LPDIRECTDRAWSURFACE arg2, LPRECT arg3, DWORD arg4, LPDDOVERLAYFX arg5);
	HRESULT __stdcall UpdateOverlayDisplay(DWORD arg1);
	HRESULT __stdcall UpdateOverlayZOrder(DWORD arg1, LPDIRECTDRAWSURFACE arg2);
};
//---------------------------------------------------------------------------
#endif

