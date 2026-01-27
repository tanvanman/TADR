//---------------------------------------------------------------------------
#pragma hdrstop

#include "config.h"
#include "oddraw.h"
#include <mutex>
#include <vector>
#include <memoryapi.h>
using namespace std;

#include "tamem.h"
#include "tafunctions.h"

#include "ChallengeResponse.h"
#include "ConstructionKickout.h"
#include "TenPlayerReplay.h"
#include "whiteboard.h"
#include "MinimapHandler.h"
#include "dddta.h"
#include "cincome.h"
#include "dialog.h"
#include "tahook.h"
#include "commanderwarp.h"
#include "maprect.h"

#include "unitrotate.h"
#include "changequeue.h"
#include "ExternQuickKey.h"
#include "TAbugfix.h"
#include "fullscreenminimap.h"
#include "GUIExpand.h"

#include "iddrawsurface.h"


#include <stdio.h>
#include "font.h"
#include <time.h>
//#include <conio.h>
#include "pcx.h"
#include "hook/etc.h"
#include "hook/hook.h"
#include "UnicodeSupport.h"

#include "LimitCrack.h"
#include "TAConfig.h"

//---------------------------------------------------------------------------
//#pragma package(smart_init)

extern HINSTANCE HInstance;
short MouseX,MouseY;
bool StartedInRect;

// uncomment to synchronise some IDDrawSurface functions (mainly IDdrawSurface::lock) with the windows message queue WinProc
//#define SYNCHRONISE_THREADS
#ifdef SYNCHRONISE_THREADS
std::recursive_mutex WinProcMutex;
#endif

HANDLE IDDrawSurface::TDrawLogFile = 0;

IDDrawSurface::IDDrawSurface(LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpTAddsc, LPDIRECTDRAWSURFACE FAR *arg2, IUnknown FAR *arg3,  HRESULT * rtn_p, 
	bool iWindowed, int iScreenWidth, int iScreenHeight)
{
	HRESULT result;

	HKEY hKey;
	DWORD dwDisposition;
	DWORD Size;

	lpFront = NULL;
	lpBack= NULL;
	lpDDClipper= NULL;

	Windowed = iWindowed;
	ScreenWidth = iScreenWidth;
	ScreenHeight = iScreenHeight;
	LocalShare->ScreenWidth = iScreenWidth;
	LocalShare->ScreenHeight = iScreenHeight;

	LocalShare->DDrawSurfClass = this;
	LocalShare->TADirectDrawFrontSurface = lpFront;

	//check if version is 3.1 standar
	if((*((unsigned char*)0x4ad494))==0x00 && (*((unsigned char*)0x4ad495))==0x55 && (*((unsigned char*)0x4ad496))==0xe8)
	{
		LocalShare->CompatibleVersion = true;
	}
	else
	{
		LocalShare->CompatibleVersion = false;
		DataShare->ehaOff = 1; //set the ehaofvariable
	}

	PlayingMovie = false;
	DisableDeInterlace = false;

	// TADRREGPATH
	std::string SubKey = CompanyName_CCSTR;
	SubKey += "\\Eye";

	RegCreateKeyEx(HKEY_CURRENT_USER, SubKey.c_str(), NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
	Size = sizeof(bool);
	if(RegQueryValueEx(hKey, "DisableDeInterlaceMovie", NULL, NULL, (unsigned char*)&DisableDeInterlace, &Size) != ERROR_SUCCESS)
	{
		//value does not exist.. create it
		DisableDeInterlace = true;
		RegSetValueEx(hKey, "DisableDeInterlaceMovie", NULL, REG_BINARY, (unsigned char*)&DisableDeInterlace, sizeof(bool));
	}
	RegCloseKey(hKey);

	LocalShare->OrgLocalPlayerID= (*TAmainStruct_PtrPtr)->LocalHumanPlayer_PlayerID;


	//---------
// 	RegCreateKeyEx(HKEY_CURRENT_USER, TADRREGPATH, NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
// 	Size = sizeof(bool);
// 	if(RegQueryValueEx(hKey, "VSync", NULL, NULL, (unsigned char*)&VerticalSync, &Size) != ERROR_SUCCESS)
// 	{
// 		VerticalSync = true;
// 	}
	VidMem= MyConfig->GetIniBool ( "UseVideoMemory", TRUE);


	if (VidMem)
	{
		lpTAddsc->ddsCaps.dwCaps&= ~(DDSCAPS_SYSTEMMEMORY);
		lpTAddsc->ddsCaps.dwCaps|= DDSCAPS_VIDEOMEMORY;
	}
	else
	{
		lpTAddsc->ddsCaps.dwCaps&= ~(DDSCAPS_VIDEOMEMORY);
		lpTAddsc->ddsCaps.dwCaps|= DDSCAPS_SYSTEMMEMORY;
	}

	result= lpDD->CreateSurface ( lpTAddsc, arg2, arg3);
	if (result!=DD_OK)
	{
		VidMem= ! VidMem;

		MyConfig->SetIniBool ( "UseVideoMemory", VidMem);

		if (VidMem)
		{
			lpTAddsc->ddsCaps.dwCaps&= ~(DDSCAPS_SYSTEMMEMORY);
			lpTAddsc->ddsCaps.dwCaps|= DDSCAPS_VIDEOMEMORY;
		}
		else
		{
			lpTAddsc->ddsCaps.dwCaps&= ~(DDSCAPS_VIDEOMEMORY);
			lpTAddsc->ddsCaps.dwCaps|= DDSCAPS_SYSTEMMEMORY;
		}
		result= lpDD->CreateSurface ( lpTAddsc, arg2, arg3);
	}
	*rtn_p= result;

	lpFront = *arg2;
	LocalShare->TADirectDrawFrontSurface = lpFront;


	SettingsDialog= new Dialog ( VidMem);
	WhiteBoard= new AlliesWhiteboard ( VidMem);
	Income= new CIncome ( VidMem);
	TAHook = nullptr;
#if TA_HOOK_ENABLE
	TAHook= new CTAHook ( VidMem);
#endif
	CommanderWarp= new CWarp ( VidMem);
	SharedRect= new CMapRect ( VidMem) ;
	ChangeQueue= new CChangeQueue ;
	DDDTA= new CDDDTA ;

#if CONSTRUCTION_KICKOUT_ENABLE
	ConstructionKickout::GetInstance();
#endif

#if USEMEGAMAP

	if (GUIExpander
		&&(GUIExpander->myMinimap))
	{
		GUIExpander->myMinimap->InitSurface ( reinterpret_cast<LPDIRECTDRAW>(LocalShare->TADirectDraw), VidMem);
	}
#endif


	DataShare->IsRunning = 10;
	SettingsDialog->SetAll  ( );

#if USEMEGAMAP
	if (GUIExpander
		&&GUIExpander->myMinimap)
	{
		GUIExpander->myMinimap->Set ( VidMem);
	}
#endif

	if (NowSupportUnicode)
	{
		NowSupportUnicode->Set ( VidMem);
	}



	lpBackLockOn= false;

	*arg2 = (IDirectDrawSurface*)this;

	OutptTxt("IDDrawSurface Created");
}

HRESULT __stdcall IDDrawSurface::QueryInterface(REFIID riid, LPVOID FAR * ppvObj)
{
	OutptTxt("QueryInterface");
	return lpFront->QueryInterface(riid, ppvObj);
}

ULONG __stdcall IDDrawSurface::AddRef()
{
	OutptTxt("AddRef");
	return lpFront->AddRef();
}

ULONG __stdcall IDDrawSurface::Release()
{
	OutptTxt("[DDrawSurface::Release] ...");
	if(lpDDClipper)
	{
		lpDDClipper->Release();
		lpDDClipper = NULL;
	}
	ULONG result; 
	if (lpFront)
	{
		result= lpFront->Release();
	}
	
#ifdef SYNCHRONISE_THREADS
    std::lock_guard<std::recursive_mutex> lock(WinProcMutex);
#endif

	if (ScreenRegion)
	{
		delete ScreenRegion;
		ScreenRegion= NULL;
	}
	if (BattleFieldRegion)
	{
		delete BattleFieldRegion;
		BattleFieldRegion= NULL;
	}
	


	delete WhiteBoard;
	WhiteBoard= NULL;
	delete Income;
	Income= NULL;
#if TA_HOOK_ENABLE
	delete TAHook;
	TAHook= NULL;
#endif
	delete CommanderWarp;
	CommanderWarp= NULL;
	delete SharedRect;
	SharedRect= NULL;
	delete SettingsDialog;
	SettingsDialog= NULL;
	delete ChangeQueue;
	ChangeQueue= NULL;
	delete DDDTA;
	DDDTA= NULL;
#if USEMEGAMAP
	if (GUIExpander
		&&(GUIExpander->myMinimap))
	{
		GUIExpander->myMinimap->ReleaseSurface ( );
	}
#endif
	delete this;
	return result;
}

HRESULT __stdcall IDDrawSurface::AddOverlayDirtyRect(LPRECT arg1)
{
	OutptTxt("AddOverlayDirtyRect");
	return lpFront->AddOverlayDirtyRect(arg1);
}

HRESULT __stdcall IDDrawSurface::AddAttachedSurface(LPDIRECTDRAWSURFACE arg1)
{
	OutptTxt("AddAttachedSurface");
	return lpFront->AddAttachedSurface(arg1);
}

HRESULT __stdcall IDDrawSurface::Blt(LPRECT arg1, LPDIRECTDRAWSURFACE arg2, LPRECT arg3, DWORD arg4, LPDDBLTFX arg5)
{
	OutptTxt("Blt");
	PlayingMovie = true;
	return lpFront->Blt(arg1, arg2, arg3, arg4, arg5);
}

HRESULT __stdcall IDDrawSurface::BltBatch(LPDDBLTBATCH arg1, DWORD arg2, DWORD arg3)
{
	OutptTxt("BltBatch");
	return lpFront->BltBatch(arg1, arg2, arg3);
}

HRESULT __stdcall IDDrawSurface::BltFast(DWORD arg1, DWORD arg2, LPDIRECTDRAWSURFACE arg3, LPRECT arg4, DWORD arg5)
{
	OutptTxt("BltFast");
	return lpFront->BltFast(arg1, arg2, arg3, arg4, arg5);
}

HRESULT __stdcall IDDrawSurface::DeleteAttachedSurface(DWORD arg1, LPDIRECTDRAWSURFACE arg2)
{
	OutptTxt("DeleteAttachedSurface");
	return lpFront->DeleteAttachedSurface(arg1, arg2);
}

HRESULT __stdcall IDDrawSurface::EnumAttachedSurfaces(LPVOID arg1, LPDDENUMSURFACESCALLBACK arg2)
{
	OutptTxt("EnumAttachedSurfaces");
	return lpFront->EnumAttachedSurfaces(arg1, arg2);
}

HRESULT __stdcall IDDrawSurface::EnumOverlayZOrders(DWORD arg1, LPVOID arg2, LPDDENUMSURFACESCALLBACK arg3)
{
	OutptTxt("EnumOverlayZOrders");
	return lpFront->EnumOverlayZOrders(arg1, arg2, arg3);
}

HRESULT __stdcall IDDrawSurface::Flip(LPDIRECTDRAWSURFACE arg1, DWORD arg2)
{
	OutptTxt("Flip");
	return lpFront->Flip(arg1, arg2);
}

HRESULT __stdcall IDDrawSurface::GetAttachedSurface(LPDDSCAPS arg1, LPDIRECTDRAWSURFACE FAR *arg2)
{
	OutptTxt("GetAttachedSurface");
	HRESULT result = lpFront->GetAttachedSurface(arg1, arg2);

#ifdef SYNCHRONISE_THREADS
    std::lock_guard<std::recursive_mutex> lock(WinProcMutex);
#endif
    lpBack = *arg2;
	LocalShare->TADirectDrawBackSurface = *arg2;
	CreateClipplist ( );
	
	return result;
}

HRESULT __stdcall IDDrawSurface::GetBltStatus(DWORD arg1)
{
	OutptTxt("GetBltStatus");
	return lpFront->GetBltStatus(arg1);
}

HRESULT __stdcall IDDrawSurface::GetCaps(LPDDSCAPS arg1)
{
	OutptTxt("GetCaps");
	return lpFront->GetCaps(arg1);
}

HRESULT __stdcall IDDrawSurface::GetClipper(LPDIRECTDRAWCLIPPER FAR* arg1)
{
	OutptTxt("GetClipper");
	return lpFront->GetClipper(arg1);
}

HRESULT __stdcall IDDrawSurface::GetColorKey(DWORD arg1, LPDDCOLORKEY arg2)
{
	OutptTxt("GetColorKey");
	return lpFront->GetColorKey(arg1, arg2);
}

HRESULT __stdcall IDDrawSurface::GetDC(HDC FAR *arg1)
{
	OutptTxt("GetDC");
	return lpFront->GetDC(arg1);
}

HRESULT __stdcall IDDrawSurface::GetFlipStatus(DWORD arg1)
{
	OutptTxt("GetFlipStatus");
	return lpFront->GetFlipStatus(arg1);
}

HRESULT __stdcall IDDrawSurface::GetOverlayPosition(LPLONG arg1, LPLONG arg2)
{
	OutptTxt("GetOverlayPosition");
	return lpFront->GetOverlayPosition(arg1, arg2);
}

HRESULT __stdcall IDDrawSurface::GetPalette(LPDIRECTDRAWPALETTE FAR* arg1)
{
	OutptTxt("GetPalette");
	return lpFront->GetPalette(arg1);
}

HRESULT __stdcall IDDrawSurface::GetPixelFormat(LPDDPIXELFORMAT arg1)
{
	OutptTxt("GetPixelFormat");
	return lpFront->GetPixelFormat(arg1);
}

HRESULT __stdcall IDDrawSurface::GetSurfaceDesc(LPDDSURFACEDESC arg1)
{
	OutptTxt("GetSurfaceDesc");
	return lpFront->GetSurfaceDesc(arg1);
}

HRESULT __stdcall IDDrawSurface::Initialize(LPDIRECTDRAW arg1, LPDDSURFACEDESC arg2)
{
	OutptTxt("Initialize");
	return lpFront->Initialize(arg1, arg2);
}

HRESULT __stdcall IDDrawSurface::IsLost()
{
	OutptTxt("IsLost");

	return lpFront->IsLost();
}

HRESULT __stdcall IDDrawSurface::Lock(LPRECT arg1, LPDDSURFACEDESC arg2, DWORD arg3, HANDLE arg4)
{
#ifdef XPOYDEBG
	HRESULT result = lpFront->Lock(arg1, arg2, arg3, arg4);

	return result;
#endif

	
#ifndef XPOYDEBG

	lpBackLockOn= true;

	HRESULT result = lpBack->Lock ( arg1, arg2, arg3, arg4);

#ifdef SYNCHRONISE_THREADS
    std::lock_guard<std::recursive_mutex> lock(WinProcMutex);
#endif

#if TA_HOOK_ENABLE
    if (GetCurrentThreadId() == LocalShare->GuiThreadId) {
        TAHook->TABlit();
    }
#endif

	if(result == DD_OK)
		SurfaceMemory = arg2->lpSurface;
	else
		SurfaceMemory = NULL;

	lPitch = arg2->lPitch;
	dwHeight= arg2->dwHeight;
	dwWidth= arg2->dwWidth;
	return result;
#endif

}

HRESULT __stdcall IDDrawSurface::ReleaseDC(HDC arg1)
{
	OutptTxt("ReleaseDC");
	return lpFront->ReleaseDC(arg1);
}

HRESULT __stdcall IDDrawSurface::Restore()
{
#ifndef XPOYDEBG
	((CDDDTA*)LocalShare->DDDTA)->FrameUpdate();
#endif
	return lpFront->Restore();
}

HRESULT __stdcall IDDrawSurface::SetClipper(LPDIRECTDRAWCLIPPER arg1)
{
	OutptTxt("SetClipper");
	//lpDDClipper = arg1;
	return lpFront->SetClipper(arg1);
}

HRESULT __stdcall IDDrawSurface::SetColorKey(DWORD arg1, LPDDCOLORKEY arg2)
{
	OutptTxt("SetColorKey");
	return lpFront->SetColorKey(arg1, arg2);
}

HRESULT __stdcall IDDrawSurface::SetOverlayPosition(LONG arg1, LONG arg2)
{
	OutptTxt("SetOverlayPosition");
	return lpFront->SetOverlayPosition(arg1, arg2);
}

HRESULT __stdcall IDDrawSurface::SetPalette(LPDIRECTDRAWPALETTE arg1)
{
	OutptTxt("SetPalette");
	Palette = arg1;
	return lpFront->SetPalette(arg1);
}

HRESULT __stdcall IDDrawSurface::Unlock(LPVOID arg1)
{
#ifdef SYNCHRONISE_THREADS
    std::lock_guard<std::recursive_mutex> lock(WinProcMutex);
#endif

	HRESULT result;
	if (GetCurrentThreadId() != LocalShare->GuiThreadId)
	{
		// mouse update thread.  just unlock back surface and return.
		//
		// If we're not the "GuiThread", we don't want to access any resources
		// here that are also accessed by our WinProc.
		// At this point, TA should have already synchronised access to the backsurface,
		// but all our stuff (notably WhiteBoardMarker and its ElementHandler,
		// which is definitely accessed by our WinProc) are not synchronised!
		// In the extreme case we could enable the SYNCHRONISE_THREADS #define.
		// But if we can get away without it, so much the better.
		//
		// So therefore: just unlock back surface and return!
		lpDDClipper->SetClipList(ScreenRegion, 0);
		result = lpBack->Unlock(arg1);
		lpBackLockOn = false;
		return result;
	}

	UpdateTAProcess ( );
	if (GetCurrentThreadId() == LocalShare->GuiThreadId && DataShare->PlayingDemo)
	{
		TenPlayerReplay::GetInstance();
	}

	GameingState * GameingState_P= (*TAmainStruct_PtrPtr)->GameingState_Ptr;

	if(VerticalSync)
	{
		if(PlayingMovie) //deinterlace and flip directly
		{
			if(!DisableDeInterlace)
			{
				DeInterlace();
				result = lpBack->Unlock(arg1);

				if(lpFront->Blt(NULL, lpBack, NULL, DDBLT_ASYNC, NULL)!=DD_OK)
				{
					lpFront->Blt(NULL, lpBack, NULL, DDBLT_WAIT, NULL);
					OutptTxt("lpBack to lpFront Blit failed");
				}
				lpBackLockOn= false;
				return result;
			}
			else
				PlayingMovie = false;
		}

		if(DataShare->ehaOff == 1 && !DataShare->PlayingDemo) //disable everything
		{//just unlock flip and return
			lpDDClipper->SetClipList ( ScreenRegion,0);
			result = lpBack->Unlock ( arg1);
			if(result!=DD_OK)
			{
				lpBackLockOn= false;
				return result;
			}

			if (lpFront->Flip(NULL, DDFLIP_DONOTWAIT) != DD_OK)
			{
				lpFront->Flip(NULL, DDFLIP_WAIT);
			}

			if (lpBack->Blt(NULL, lpFront, NULL, DDBLT_ASYNC, NULL) != DD_OK)
			{
				lpBack->Blt(NULL, lpFront, NULL, DDBLT_WAIT, NULL);
				OutptTxt("lpFront to lpBack Blit failed");
			}
		}
		else
		{
			if(SurfaceMemory!=NULL)
			{
				WhiteBoard->LockBlit ( (char*)SurfaceMemory, lPitch);

				if (GameingState_P
					&&(gameingstate::MULTI==GameingState_P->State))
				{
					SharedRect->LockBlit ( (char*)SurfaceMemory, lPitch);
				}
#if USEMEGAMAP
				if ((GUIExpander)
					&&(GUIExpander->myMinimap))
				{
					GUIExpander->myMinimap->LockBlit ( (char*)SurfaceMemory, dwWidth, dwHeight, lPitch);
				}
#endif
			}

			result = lpBack->Unlock ( arg1);
			if(result!=DD_OK)
			{
				lpBackLockOn= false;
				return result;
			}
			DDDTA->Blit(lpBack);

			lpDDClipper->SetClipList ( BattleFieldRegion,0);
			WhiteBoard->Blit(lpBack);


			if (GameingState_P
				&&(gameingstate::MULTI==GameingState_P->State))
			{
				CommanderWarp->Blit(lpBack);
			}

			
#if USEMEGAMAP
			if ((GUIExpander)
				&&(GUIExpander->myMinimap))
			{
				GUIExpander->myMinimap->Blit ( lpBack);
			}

#endif

			lpDDClipper->SetClipList(ScreenRegion,0);
			if (GameingState_P
				&&(gameingstate::MULTI==GameingState_P->State))
			{
				Income->BlitIncome(lpBack);
			}

#if WEATHER_REPORT
			Income->BlitWeatherReport((char*)SurfaceMemory, dwWidth, dwHeight, lPitch);
#endif

			SettingsDialog->BlitDialog(lpBack);
#if TA_HOOK_ENABLE
			TAHook->Blit(lpBack);
#endif
			if (GetCurrentThreadId() == LocalShare->GuiThreadId) {
				ChallengeResponse::GetInstance()->Blit((char*)SurfaceMemory, dwWidth, dwHeight, lPitch);
			}

			//////////////////////////////////////////////////////////////////////////
			//unicode
			if (NULL!=NowSupportUnicode)
			{
				NowSupportUnicode->Blt ( lpBack);
			}

			if(lpFront->Blt(NULL, lpBack, NULL, DDBLT_ASYNC, NULL)!=DD_OK)
			{
				lpFront->Blt(NULL, lpBack, NULL, DDBLT_WAIT, NULL);
				OutptTxt("lpBack to lpFront Blit failed");
			}
		}
		lpBackLockOn= false;
		return result;
	}
	else
	{
		if(PlayingMovie) //deinterlace and flip directly
		{
			if(!DisableDeInterlace)
			{
				DeInterlace();
				HRESULT result = lpBack->Unlock(arg1);
				if(lpFront->Flip(NULL, DDFLIP_DONOTWAIT | DDFLIP_NOVSYNC) != DD_OK)
				{
					if(lpFront->Flip(NULL, DDFLIP_NOVSYNC) != DD_OK)
						lpFront->Flip(NULL, DDFLIP_WAIT);
				}
				lpBackLockOn= false;
				return result;
			}
			else
				PlayingMovie = false;
		}

		
		if(DataShare->ehaOff == 1 && !DataShare->PlayingDemo) //disable everything
		{//just unlock flip and return
			lpDDClipper->SetClipList ( ScreenRegion,0);
			result = lpBack->Unlock ( arg1);
			if(result!=DD_OK)
			{
				lpBackLockOn= false;
				return result;
			}
		}
		else
		{
			if(SurfaceMemory!=NULL)
			{
				WhiteBoard->LockBlit ( (char*)SurfaceMemory, lPitch);

				if (GameingState_P
					&&(gameingstate::MULTI==GameingState_P->State))
				{
					SharedRect->LockBlit ( (char*)SurfaceMemory, lPitch);
				}

#if USEMEGAMAP
				if ((GUIExpander)
					&&(GUIExpander->myMinimap))
				{
					GUIExpander->myMinimap->LockBlit ( (char*)SurfaceMemory, dwWidth, dwHeight, lPitch);
				}
#endif

			}

			result = lpBack->Unlock ( arg1);
			if(result!=DD_OK)
			{
				lpBackLockOn= false;
				return result;
			}

			DDDTA->Blit(lpBack);

			lpDDClipper->SetClipList ( BattleFieldRegion,0);
			WhiteBoard->Blit(lpBack);



			if (GameingState_P
				&&(gameingstate::MULTI==GameingState_P->State))
			{
				CommanderWarp->Blit(lpBack);
			}


#if USEMEGAMAP
			if ((GUIExpander)
				&&(GUIExpander->myMinimap))
			{
				GUIExpander->myMinimap->Blit ( lpBack);
			}
#endif

			lpDDClipper->SetClipList(ScreenRegion,0);

			if (GameingState_P
				&&(gameingstate::MULTI==GameingState_P->State))
			{
				Income->BlitIncome(lpBack);
			}

#if WEATHER_REPORT
			Income->BlitWeatherReport((char*)SurfaceMemory, dwWidth, dwHeight, lPitch);
#endif

			SettingsDialog->BlitDialog(lpBack);
#if TA_HOOK_ENABLE
			TAHook->Blit(lpBack);
#endif
			if (GetCurrentThreadId() == LocalShare->GuiThreadId) {
				ChallengeResponse::GetInstance()->Blit((char*)SurfaceMemory, dwWidth, dwHeight, lPitch);
			}

			//////////////////////////////////////////////////////////////////////////
			//unicode
			if (NULL!=NowSupportUnicode)
			{
				NowSupportUnicode->Blt ( lpBack);
			}
		}

		if(lpFront->Flip(NULL, DDFLIP_DONOTWAIT) != DD_OK)
		{
			lpFront->Flip(NULL, DDFLIP_WAIT);
		}


		if(lpBack->Blt(NULL, lpFront, NULL, DDBLT_ASYNC, NULL)!=DD_OK)
		{
			lpBack->Blt(NULL, lpFront, NULL, DDBLT_WAIT, NULL);
			OutptTxt("lpFront to lpBack Blit failed");
		}
	}

	lpBackLockOn= false;
	return result;
}


HRESULT __stdcall IDDrawSurface::UpdateOverlay(LPRECT arg1, LPDIRECTDRAWSURFACE arg2, LPRECT arg3, DWORD arg4, LPDDOVERLAYFX arg5)
{
	OutptTxt("UpdateOverlay");
	return lpFront->UpdateOverlay(arg1, arg2, arg3, arg4, arg5);
}

HRESULT __stdcall IDDrawSurface::UpdateOverlayDisplay(DWORD arg1)
{
	OutptTxt("UpdateOverlayDisplay");
	return lpFront->UpdateOverlayDisplay(arg1);
}

HRESULT __stdcall IDDrawSurface::UpdateOverlayZOrder(DWORD arg1, LPDIRECTDRAWSURFACE arg2)
{
	OutptTxt("UpdateOverlayZOrder");
	return lpFront->UpdateOverlayZOrder(arg1, arg2);
}

void IDDrawSurface::OutptFmtTxt(const char* format, ...)
{
	if (!format) {
		OutptTxt("");
		return;
	}

	char buffer[16384] = { 0 };

#if defined(DEBUG_INFO) || defined(DEBUG_INFO_2) || defined(_DEBUG)

	va_list args;
	va_start(args, format);

	// Try formatting — this version NEVER fastfails.
	int result = vsnprintf(buffer, sizeof(buffer), format, args);

	va_end(args);

	// If formatting failed (negative return), fall back to raw text.
	// Common reason: mismatched format tags, e.g., "%s" with no args.
	if (result < 0) {
		// Copy raw string safely into buffer
		strncpy(buffer, format, sizeof(buffer) - 1);
		buffer[sizeof(buffer) - 1] = '\0';
	}

#else
	// In non-debug builds, just output raw text as-is.
	strncpy(buffer, format, sizeof(buffer) - 1);
	buffer[sizeof(buffer) - 1] = '\0';
#endif

	OutptTxt(buffer);
}

void IDDrawSurface::OutptTxt(const char* buffer, bool newline)
{
#if defined(DEBUG_INFO_2) || defined(_DEBUG)
	OutputDebugStringA(buffer);
#endif

#ifdef DEBUG_INFO
	if (TDrawLogFile == 0) {
		TDrawLogFile = CreateFileA("tdrawlog.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	}
	if (TDrawLogFile != INVALID_HANDLE_VALUE) {

		time_t rawtime;
		struct tm* timeinfo;
		char datetimestr[80];

		time(&rawtime);
		timeinfo = std::localtime(&rawtime);

		std::strftime(datetimestr, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);

		std::string s = std::string(datetimestr) + " " + std::to_string(GetCurrentThreadId()) + " --- " + buffer;
		DWORD tempWritten;
		WriteFile(TDrawLogFile, s.c_str(), s.size(), &tempWritten, NULL);
		if (newline)
		{
			WriteFile(TDrawLogFile, "\r\n", 2, &tempWritten, NULL);
		}
	}
#endif //DEBUG
}


void IDDrawSurface::OutptInt(int Int_I)
{
#ifdef DEBUG_INFO
	char Buffer_Int[0x10];
	wsprintf ( Buffer_Int, "%x", Int_I);
	OutptTxt( Buffer_Int);
#endif //DEBUG
}
void IDDrawSurface::FrontSurface (LPDIRECTDRAWSURFACE lpTASurf)
{
	lpFront= lpTASurf;
}

void IDDrawSurface::Set(bool EnableVSync)
{
	VerticalSync = EnableVSync;
}

void IDDrawSurface::CreateClipplist()
{
	LPDIRECTDRAW TADD = (IDirectDraw*)LocalShare->TADirectDraw;
	if (lpBack)
	{
		if (lpDDClipper)
		{
			lpDDClipper->Release ( );
		}
		TADD->CreateClipper ( 0,&lpDDClipper,NULL);

		ScreenRegion = (LPRGNDATA)new char[sizeof(RGNDATAHEADER)+sizeof(RECT)];
		BattleFieldRegion = (LPRGNDATA)new char[sizeof(RGNDATAHEADER)+sizeof(RECT)];

		RECT ScreenRect = {0,0,ScreenWidth,ScreenHeight};
		memcpy(ScreenRegion->Buffer, &ScreenRect,sizeof(RECT));
		ScreenRegion->rdh.dwSize = sizeof(RGNDATAHEADER);
		ScreenRegion->rdh.iType = RDH_RECTANGLES;
		ScreenRegion->rdh.nCount = 1;
		ScreenRegion->rdh.nRgnSize = sizeof(RECT);
		ScreenRegion->rdh.rcBound.left = 0;
		ScreenRegion->rdh.rcBound.top = 0;
		ScreenRegion->rdh.rcBound.right = ScreenWidth;
		ScreenRegion->rdh.rcBound.bottom = ScreenHeight;

		RECT BattleFieldRect = {128,32,ScreenWidth,ScreenHeight-32};
		memcpy(BattleFieldRegion->Buffer, &BattleFieldRect,sizeof(RECT));
		BattleFieldRegion->rdh.dwSize = sizeof(RGNDATAHEADER);
		BattleFieldRegion->rdh.iType = RDH_RECTANGLES;
		BattleFieldRegion->rdh.nCount = 1;
		BattleFieldRegion->rdh.nRgnSize = sizeof(RECT);
		BattleFieldRegion->rdh.rcBound.left = 128;
		BattleFieldRegion->rdh.rcBound.top = 32;
		BattleFieldRegion->rdh.rcBound.right = ScreenWidth;
		BattleFieldRegion->rdh.rcBound.bottom = ScreenHeight-32;


		lpBack->SetClipper ( lpDDClipper);
	}
}

void IDDrawSurface::ScreenShot()
{
	//create the creenshot
	char ScreenShotName[MAX_PATH*2];

	int i = 0;
	CreateFileName ( ScreenShotName, i);

	WIN32_FIND_DATA fs;
	HANDLE File = FindFirstFile ( ScreenShotName, &fs);
	while(File!=INVALID_HANDLE_VALUE)
	{
		i++;
		CreateFileName(ScreenShotName, i);
		FindClose(File);
		File = FindFirstFile(ScreenShotName, &fs);
	}
	//FindClose(Handle);

	OutptTxt(ScreenShotName);

	while (lpBackLockOn)
	{
		Sleep ( 1);
	}
	PCX PCXScreen;

	DDSURFACEDESC ddsd;
	DDRAW_INIT_STRUCT(ddsd);
	if(lpBack->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL)==DD_OK)
	{
		char *Buff = new char[LocalShare->ScreenWidth*LocalShare->ScreenHeight];
		for(int j=0; j<LocalShare->ScreenHeight; j++)
		{
			memcpy(Buff+j*LocalShare->ScreenWidth, ((char*)ddsd.lpSurface)+j*ddsd.lPitch, LocalShare->ScreenWidth);
		}

		PCXScreen.NewBuffer(LocalShare->ScreenWidth, LocalShare->ScreenHeight);
		PCXScreen.SetBuffer((UCHAR*)Buff);
		PCXScreen.CopyPalette ( TAPalette);
		if(PCXScreen.Save(ScreenShotName) == false)
			OutptTxt("error writing screenshot");

		
		//PCXScreen.SetBuffer ( NULL);
		//delete Buff;
		lpBack->Unlock(NULL);
	}
	

}

void IDDrawSurface::CreateFileName(char *Buff, int Num)
{
	int *PTR = (int*)0x00511de8;
	char *RootDir = (char*)(*PTR + 0x38A53);
	lstrcpyA(Buff, RootDir);
	lstrcatA(Buff, "\\screenshots\\");
	CreateDir ( Buff ); //creates the dir so it exist
	char CNum[10];
	wsprintf ( CNum, "%.4i", Num);

	char Date[100];
	struct tm temp_time_now;
	struct tm *time_now= &temp_time_now;
	time_t timer;
	timer = time(NULL);
	localtime_s(&temp_time_now, &timer);
	strftime(Date, 100,"%x - ", time_now);
	CorrectName(Date);
	lstrcatA(Buff, Date);

	if(DataShare->TAProgress != TAInGame)
	{
		lstrcatA(Buff, "SHOT");
		lstrcatA(Buff, CNum);
		lstrcatA(Buff, ".pcx");
		return;
	}

	char TempBuff[100];
	lstrcpyA ( TempBuff, DataShare->MapName);
	CorrectName(TempBuff);
	lstrcatA(Buff, TempBuff);
	lstrcatA(Buff, " - ");
	lstrcpyA(TempBuff, DataShare->PlayerNames[0]);
	CorrectName(TempBuff);
	lstrcatA(Buff, TempBuff);
	for(int i=1; i<10; i++)
	{
		if(strlen(DataShare->PlayerNames[i])>0)
		{
			lstrcatA(Buff, ", ");
			lstrcpyA(TempBuff, DataShare->PlayerNames[i]);
			CorrectName(TempBuff);
			lstrcatA(Buff, TempBuff);
		}
	}
	lstrcatA(Buff, " ");
	lstrcatA(Buff, CNum);
	lstrcatA(Buff, ".pcx");
}

void IDDrawSurface::CreateDir(char *Dir)
{
	char *ptr;
	ptr = strstr(Dir, "\\");
	ptr++;
	/*ptr = strstr(Dir, "\\");
	if(ptr!=NULL)
	ptr++;*/
	while(ptr!=NULL)
	{
		char CDir[MAX_PATH];
		memcpy(CDir, Dir, ptr-Dir);
		CDir[(ptr-Dir)] = '\0';
		CreateDirectory(CDir, NULL);
		ptr = strstr(ptr, "\\");
		if(ptr!=NULL)
			ptr++;
	}
	CreateDirectory(Dir, NULL);
}

void IDDrawSurface::CorrectName(char *Name)
{
	for(size_t i=0; i<strlen(Name); i++)
	{
		char C = Name[i];
		/*    if((C<'a' || C>'z') && (C<'A' || C>'Z') && (C<'1' || C>'0') && (C<'!' || C>'&') && C!='(' && C!=')'
		&& C!='[' && C!=']' && C!='\0')
		Name[i] = '_';        */
		if( strchr( "\\/:*?\"<>|", C ) )
			Name[i] = '_';
	}
}

void IDDrawSurface::DeInterlace()
{
	int PaletteEntry = 0;

	Palette->GetEntries(NULL, 1, 1, (PALETTEENTRY*)&PaletteEntry);
	if(PaletteEntry != 83886208)
	{
		if(SurfaceMemory != NULL)
		{
			char *SurfMem = (char*)SurfaceMemory;
			for(int i=1; i<(LocalShare->ScreenHeight); i+=2)
			{
				memcpy(&SurfMem[i*lPitch], &SurfMem[(i+1)*lPitch], LocalShare->ScreenWidth);
			}
		}  
	}
	else
		PlayingMovie = false;
}

LRESULT CALLBACK _WinProc(HWND WinProcWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    __try
	{
		UpdateTAProcess ( );
		if (NULL!=FixTABug)
		{
			FixTABug->AntiCheat ( );
		}

		if(DataShare->ehaOff == 1 && !DataShare->PlayingDemo)
		{
			return LocalShare->TAWndProc ( WinProcWnd, Msg, wParam, lParam);
		}

		//////////////////////////////////////////////////////////////////////////
		if (NULL!=NowCrackLimit)
		{
			if(myExternQuickKey->Message ( WinProcWnd, Msg, wParam, lParam))
				return 0;

		}
		if ((NULL!=NowSupportUnicode)
			&&NowSupportUnicode->Message ( WinProcWnd, Msg, wParam, lParam))
		{
			return 0;
		}

		if((Msg == WM_KEYUP)||(WM_KEYDOWN==Msg))
		{
			if((wParam == 120&&(GetAsyncKeyState ( 17) &0x8000)>0)
				||wParam == VK_SNAPSHOT)
			{
				if (Msg == WM_KEYUP)
				{
					((IDDrawSurface*)LocalShare->DDrawSurfClass)->ScreenShot();
				}

				return 0;
			}
		}
		//////////////////////////////////////////////////////////////////////////

		if (NULL != (LocalShare->Dialog))
		{
			if (((Dialog*)LocalShare->Dialog)->Message(WinProcWnd, Msg, wParam, lParam))
				return 0;  //message handled by the dialog
		}

		if((NULL!=LocalShare->Whiteboard)
			&&(((AlliesWhiteboard*)LocalShare->Whiteboard)->Message(WinProcWnd, Msg, wParam, lParam)))
			return 0;

		if((NULL!=LocalShare->Income)
			&&(((CIncome*)LocalShare->Income)->Message(WinProcWnd, Msg, wParam, lParam)))
			return 0;  //message handled by the income class

		if((LocalShare->CommanderWarp)
			&&(((CWarp*)LocalShare->CommanderWarp)->Message(WinProcWnd, Msg, wParam, lParam)))
			return 0;

#if CONSTRUCTION_KICKOUT_ENABLE
		if (ConstructionKickout::GetInstance()->Message(WinProcWnd, Msg, wParam, lParam))
		{
			return 0;
		}
#endif

#if TA_HOOK_ENABLE
		if((NULL!=LocalShare->TAHook)
			&&(((CTAHook*)LocalShare->TAHook)->Message(WinProcWnd, Msg, wParam, lParam)))
			return 0;  //message handled by tahook class
#endif

		//   if(((CChangeQueue*)LocalShare->ChangeQueue)->Message(WinProcWnd, Msg, wParam, lParam))
		//     return 0;

		if((NULL!=LocalShare->DDDTA)
			&&(((CDDDTA*)LocalShare->DDDTA)->Message(WinProcWnd, Msg, wParam, lParam)))
			return 0;

		//   if(((CUnitRotate*)LocalShare->UnitRotate)->Message(WinProcWnd, Msg, wParam, lParam))
		//     return 0;

		if(DataShare->F1Disable)
			if(Msg == WM_KEYDOWN && wParam == 112)
				return 0;
#if USEMEGAMAP
		if (GUIExpander
			&&(GUIExpander->myMinimap))
		{
			if (GUIExpander->myMinimap->Message ( WinProcWnd, Msg, wParam, lParam))
			{
				return 0;
			}
		}
#endif
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		;// return LocalShare->TAWndProc(WinProcWnd, Msg, wParam, lParam);
	}
	return LocalShare->TAWndProc(WinProcWnd, Msg, wParam, lParam);
}

LRESULT CALLBACK WinProc(HWND WinProcWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
#ifdef SYNCHRONISE_THREADS
    std::lock_guard<std::recursive_mutex> lock(WinProcMutex);
#endif
    return _WinProc(WinProcWnd, Msg, wParam, lParam);
}
