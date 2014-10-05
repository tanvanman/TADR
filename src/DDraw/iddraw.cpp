//---------------------------------------------------------------------------
#pragma hdrstop

#include "iddraw.h"           
#include "iddrawsurface.h"
#include <stdio.h>
//---------------------------------------------------------------------------
//#pragma package(smart_init)

#define OutptTxt IDDrawSurface::OutptTxt

IDDraw::IDDraw(LPDIRECTDRAW lpTADD, bool CreateFullScreen)
{
	lpDD = lpTADD;
	Windowed = CreateFullScreen;
	OutptTxt("IDDraw Created");

	LocalShare->DDrawClass = this;
	LocalShare->TADirectDraw = lpTADD;
	//unicode

	//
	//check if version is 3.1 standar
	if((*((unsigned char*)0x4ad494))==0x00 && (*((unsigned char*)0x4ad495))==0x55 && (*((unsigned char*)0x4ad496))==0xe8)
		LocalShare->CompatibleVersion = true;
	else
	{
		LocalShare->CompatibleVersion = false;
		DataShare->ehaOff = 1; //set the ehaofvariable
	}
}

HRESULT __stdcall IDDraw::QueryInterface(REFIID riid , LPVOID FAR * ppvObj)
{
	OutptTxt("QueryInterface");
	return lpDD->QueryInterface(riid, ppvObj);
}

ULONG __stdcall IDDraw::AddRef()
{
	OutptTxt("AddRef");
	return lpDD->AddRef();
}

ULONG __stdcall IDDraw::Release()
{
	OutptTxt("DDRAW::Release");
	ULONG result = lpDD->Release();
	delete this;
	return result;
}

HRESULT __stdcall IDDraw::Compact()
{
	OutptTxt("Compact");
	return lpDD->Compact();
}

HRESULT __stdcall IDDraw::CreateClipper(DWORD arg1, LPDIRECTDRAWCLIPPER FAR* arg2, IUnknown FAR * arg3)
{
	OutptTxt("CreateClipper");
	return lpDD->CreateClipper(arg1, arg2, arg3);
}

HRESULT __stdcall IDDraw::CreatePalette(DWORD arg1, LPPALETTEENTRY arg2, LPDIRECTDRAWPALETTE FAR* arg3, IUnknown FAR * arg4)
{
	OutptTxt("CreatePalette");
	return lpDD->CreatePalette(arg1, arg2, arg3, arg4);
}

HRESULT __stdcall IDDraw::CreateSurface(LPDDSURFACEDESC arg1, LPDIRECTDRAWSURFACE FAR *arg2, IUnknown FAR *arg3)
{
	OutptTxt("CreateSurface");

	arg1->dwBackBufferCount = 2;

	HRESULT result;// = lpDD->CreateSurface(arg1, arg2, arg3);
	
	IDDrawSurface *SClass = new IDDrawSurface ( lpDD, arg1, arg2, arg3, &result, 
		Windowed, ScreenWidth, ScreenHeight);
	//SClass->FrontSurface ( *arg2);
	//*arg2 = (IDirectDrawSurface*)SClass;
	return result;

}

HRESULT __stdcall IDDraw::DuplicateSurface(LPDIRECTDRAWSURFACE arg1, LPDIRECTDRAWSURFACE FAR * arg2)
{
	OutptTxt("DuplicateSurface");
	return lpDD->DuplicateSurface(arg1, arg2);
}

HRESULT __stdcall IDDraw::EnumDisplayModes(DWORD arg1, LPDDSURFACEDESC arg2, LPVOID arg3, LPDDENUMMODESCALLBACK arg4)
{
	OutptTxt("EnumDisplayModes");
	return lpDD->EnumDisplayModes(arg1, arg2, arg3, arg4);
}

HRESULT __stdcall IDDraw::EnumSurfaces(DWORD arg1, LPDDSURFACEDESC arg2, LPVOID arg3,LPDDENUMSURFACESCALLBACK arg4)
{
	OutptTxt("EnumSurfaces");
	return lpDD->EnumSurfaces(arg1, arg2, arg3, arg4);
}

HRESULT __stdcall IDDraw::FlipToGDISurface()
{
	OutptTxt("FlipToGDISurface");
	return lpDD->FlipToGDISurface();
}

HRESULT __stdcall IDDraw::GetCaps(LPDDCAPS arg1, LPDDCAPS arg2)
{
	OutptTxt("GetCaps");
	return lpDD->GetCaps(arg1, arg2);
}

HRESULT __stdcall IDDraw::GetDisplayMode(LPDDSURFACEDESC arg1)
{
	OutptTxt("GetDisplayMode");
	return lpDD->GetDisplayMode(arg1);
}

HRESULT __stdcall IDDraw::GetFourCCCodes(LPDWORD arg1, LPDWORD arg2)
{
	OutptTxt("GetFourCCCodes");
	return lpDD->GetFourCCCodes(arg1, arg2);
}

HRESULT __stdcall IDDraw::GetGDISurface(LPDIRECTDRAWSURFACE FAR *arg1)
{
	OutptTxt("GetGDISurface");
	return lpDD->GetGDISurface(arg1);
}

HRESULT __stdcall IDDraw::GetMonitorFrequency(LPDWORD arg1)
{
	OutptTxt("GetMonitorFrequency");
	return lpDD->GetMonitorFrequency(arg1);
}

HRESULT __stdcall IDDraw::GetScanLine(LPDWORD arg1)
{
	OutptTxt("GetScanLine");
	return lpDD->GetScanLine(arg1);
}

HRESULT __stdcall IDDraw::GetVerticalBlankStatus(LPBOOL arg1)
{
	OutptTxt("GetVerticalBlankStatus");
	return lpDD->GetVerticalBlankStatus(arg1);
}

HRESULT __stdcall IDDraw::Initialize(GUID FAR *arg1)
{
	OutptTxt("Initialize");
	return lpDD->Initialize(arg1);
}

HRESULT __stdcall IDDraw::RestoreDisplayMode()
{
	OutptTxt("RestoreDisplayMode");
	return lpDD->RestoreDisplayMode();
}

HRESULT __stdcall IDDraw::SetCooperativeLevel(HWND arg1, DWORD arg2)
{
	OutptTxt("SetCooperativeLevel");
	if(Windowed == false)
		return lpDD->SetCooperativeLevel(arg1, arg2);
	else
	{
		return lpDD->SetCooperativeLevel(arg1, DDSCL_NORMAL);
	}
}

HRESULT __stdcall IDDraw::SetDisplayMode(DWORD arg1, DWORD arg2,DWORD arg3)
{
	OutptTxt("SetDisplayMode");

	ScreenWidth = arg1;
	ScreenHeight = arg2;
	LocalShare->ScreenWidth = ScreenWidth;
	LocalShare->ScreenHeight = ScreenHeight;

	return lpDD->SetDisplayMode(arg1, arg2, arg3);

}

HRESULT __stdcall IDDraw::WaitForVerticalBlank(DWORD arg1, HANDLE arg2)
{
	OutptTxt("WaitForVerticalBlank");
	return lpDD->WaitForVerticalBlank(arg1, arg2);
}


