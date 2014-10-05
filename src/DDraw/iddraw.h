//---------------------------------------------------------------------------
#ifndef iddrawH
#define iddrawH
#include "oddraw.h"
#include <objbase.h>

#define DDRAW_INIT_STRUCT(ddstruct) { memset(&ddstruct,0,sizeof(ddstruct)); ddstruct.dwSize=sizeof(ddstruct); }

extern bool Log;
//---------------------------------------------------------------------------
//public TComInterface<IDirectDraw>, public TComInterfaceBase<IUnknown>
class IDDraw : public IDirectDraw
{
  private:
    
    bool Windowed;
    int ScreenWidth;
    int ScreenHeight;
  public:
    LPDIRECTDRAW lpDD;
    IDDraw(LPDIRECTDRAW lpTADD, bool CreateFullScreen);
    /*** IUnknown methods ***/
    HRESULT __stdcall QueryInterface(REFIID riid, LPVOID FAR * ppvObj);
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();
    // IDirectDraw methods
    HRESULT __stdcall Compact();
    HRESULT __stdcall CreateClipper(DWORD arg1, LPDIRECTDRAWCLIPPER FAR* arg2, IUnknown FAR * arg3);
    HRESULT __stdcall CreatePalette(DWORD arg1, LPPALETTEENTRY arg2, LPDIRECTDRAWPALETTE FAR* arg3, IUnknown FAR * arg4);
    HRESULT __stdcall CreateSurface(LPDDSURFACEDESC arg1, LPDIRECTDRAWSURFACE FAR *arg2, IUnknown FAR *arg3);
    HRESULT __stdcall DuplicateSurface(LPDIRECTDRAWSURFACE arg1, LPDIRECTDRAWSURFACE FAR * arg2);
    HRESULT __stdcall EnumDisplayModes(DWORD arg1, LPDDSURFACEDESC arg2, LPVOID arg3, LPDDENUMMODESCALLBACK arg4);
    HRESULT __stdcall EnumSurfaces(DWORD arg1, LPDDSURFACEDESC arg2, LPVOID arg3,LPDDENUMSURFACESCALLBACK arg4);
    HRESULT __stdcall FlipToGDISurface();
    HRESULT __stdcall GetCaps(LPDDCAPS arg1, LPDDCAPS arg2);
    HRESULT __stdcall GetDisplayMode(LPDDSURFACEDESC arg1);
    HRESULT __stdcall GetFourCCCodes(LPDWORD arg1, LPDWORD arg2);
    HRESULT __stdcall GetGDISurface(LPDIRECTDRAWSURFACE FAR *arg1);
    HRESULT __stdcall GetMonitorFrequency(LPDWORD arg1);
    HRESULT __stdcall GetScanLine(LPDWORD arg1);
    HRESULT __stdcall GetVerticalBlankStatus(LPBOOL arg1);
    HRESULT __stdcall Initialize(GUID FAR *arg1);
    HRESULT __stdcall RestoreDisplayMode();
    HRESULT __stdcall SetCooperativeLevel(HWND arg1, DWORD arg2);
    HRESULT __stdcall SetDisplayMode(DWORD arg1, DWORD arg2,DWORD arg3);
    HRESULT __stdcall WaitForVerticalBlank(DWORD arg1, HANDLE arg2);
};
//---------------------------------------------------------------------------
#endif

