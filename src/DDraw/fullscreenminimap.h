#pragma once

#define USEMEGAMAP

#define MEGAMAPFPS (30)

#define MAXCURSORWIDTH (0x30)
#define MAXCURSORHEIGHT (0x30)

struct tagTNTHeaderStruct;
class TNTtoMiniMap;
class TAGameAreaReDrawer;
class InlineSingleHook;
class SingleHook;

class UnitsMinimap;
class MappedMap;
class ProjectileMap;
class MegaMapControl;
class MegamapTAStuff;

struct tagInlineX86StackBuffer;
typedef struct tagInlineX86StackBuffer * PInlineX86StackBuffer;

class FullScreenMinimap
{
public:
	TAGameAreaReDrawer * GameDrawer;
	UnitsMinimap * UnitsMap;
	TNTtoMiniMap * MyMinimap_p;
	MappedMap* Mapped_p;
	ProjectileMap* ProjectilesMap_p;
	MegaMapControl * Controler;
	MegamapTAStuff* TAStuff;

	int MegamapWidth;
	int MegamapHeight;
	RECT MegaMapInscren;
	RECT MegamapRect;

	RECT TAMAPTAPos;
public:
	FullScreenMinimap (BOOL Doit);
	~FullScreenMinimap (void);

	void InitMinimap (tagTNTHeaderStruct * TNTPtr, RECT *  GameScreen= NULL);

	void Blit(LPDIRECTDRAWSURFACE DestSurf);

	void LockBlit (LPVOID lpSurfaceMem, int dwWidth,int dwHeight, int lPitch);


	void LockBlit_TA (LPVOID lpSurfaceMem, int dwWidth, int dwHeight, int lPitch);
	void LockBlit_MEGA (LPVOID lpSurfaceMem, int dwWidth, int dwHeight, int lPitch);
	void BlitSurfaceCursor (LPDIRECTDRAWSURFACE DestSurf);

	void InitSurface (LPDIRECTDRAW TADD, BOOL VidMem);
	void InitSurface (LPDIRECTDRAW TADD);
	void ReleaseSurface (void);

	bool Message(HWND WinProcWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	BOOL IsBliting ( );
	void EnterMegaMap ();
	void QuitMegaMap ( );

	void Set (int VirtualKey);
	void SetVid (BOOL VidMem_a);

	void DischargeGUIState ( );
	void BlockGUIState ( );
private:
	InlineSingleHook * LoadMap_hook;
	BOOL Blit_b;
	BOOL Flipping;
	
	BOOL Do_b;



	int MegamapVirtualKey;

	int MaxIconWidth;
	int MaxIconHeight;


	BOOL DrawBackground;
	BOOL DrawMapped;
	BOOL DrawProjectile;
	BOOL DrawUnits;
	BOOL DrawMegamapRect;
	BOOL DrawMegamapBlit;
	BOOL DrawSelectAndOrder;
	BOOL DrawMegamapCursor;
	BOOL DrawMegamapTAStuff;

	BOOL WheelZoom;
	BOOL WheelMoveMegaMap;
	BOOL DoubleClickMoveMegamap;

	BOOL UseSurfaceCursor;

	InlineSingleHook * DrawTAScreen_hok;
	InlineSingleHook * DrawTAScreenBlit_hok;
	InlineSingleHook * DrawTAScreenEnd_hok;

	SingleHook * KeepActive;
	SingleHook * KeepActive1;
	BOOL VidMem;
};



int __stdcall BlockTADraw (PInlineX86StackBuffer X86StrackBuffer);
int __stdcall DischargeTADraw (PInlineX86StackBuffer X86StrackBuffer);


int __stdcall ForceTADrawBlit (PInlineX86StackBuffer X86StrackBuffer);