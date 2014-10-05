#include "oddraw.h"

#include "hook/etc.h"
#include "hook/hook.h"
#include "dialog.h"
#include "tamem.h"
#include "tafunctions.h"
#include "PCX.H"
#include "gameredrawer.h"
#include "UnitDrawer.h"
#include "MegamapControl.h"
#include "fullscreenminimap.h"
#include "dialog.h"
#include "cincome.h"
#include "gaf.h"
#include "iddrawsurface.h"
#include "ExternQuickKey.h "
#include "tahook.h"

#include <math.h>
#include <vector>
using namespace std;

#ifdef USEMEGAMAP

using namespace ordertype;

int  __stdcall FindMouseUnitRounte (PInlineX86StackBuffer X86StrackBuffer)
{
	if (((MegaMapControl *)(X86StrackBuffer->myInlineHookClass_Pish->ParamOfHook))->IsInControl ())
	{
		X86StrackBuffer->Eax= (*TAmainStruct_PtrPtr)->MouseOverUnit;
		X86StrackBuffer->rtnAddr_Pvoid= (LPVOID)0x48CDDC ;
		return X86STRACKBUFFERCHANGE;
	}

	return 0;
}

int  __stdcall  GetPosition_DwordRounte (PInlineX86StackBuffer X86StrackBuffer)
{
	if (((MegaMapControl *)(X86StrackBuffer->myInlineHookClass_Pish->ParamOfHook))->IsInControl ())
	{
		
		X86StrackBuffer->rtnAddr_Pvoid= (LPVOID)0x484CD4 ;
		return X86STRACKBUFFERCHANGE;
	}

	return 0;
}

int  __stdcall  GetGridPosFeatureRounte (PInlineX86StackBuffer X86StrackBuffer)
{
	if (((MegaMapControl *)(X86StrackBuffer->myInlineHookClass_Pish->ParamOfHook))->IsInControl ())
	{
		X86StrackBuffer->Eax= 0xffff;
		X86StrackBuffer->rtnAddr_Pvoid= (LPVOID)0x421EA0 ;
		return X86STRACKBUFFERCHANGE;
	}

	return 0;
}

MegaMapControl::MegaMapControl (FullScreenMinimap * parent_p, RECT * MegaMapScreen_p, RECT * TAMap_p, RECT * GameScreen_p,
	int MaxIconWidth, int MaxIconHeight, int MegaMapVirtulKey_arg, BOOL WheelMoveMegaMap_v, BOOL DoubleClickMoveMegamap_v, BOOL WheelZoom_v)
{
	FindMouseUnitHook= new InlineSingleHook ( (unsigned int)FindMouseUnit, 5, INLINE_5BYTESLAGGERJMP, FindMouseUnitRounte);
	FindMouseUnitHook->SetParamOfHook ( (LPVOID)this);


	GetPosition_DwordHook= new InlineSingleHook ( (unsigned int)GetPosition_Dword, 5, INLINE_5BYTESLAGGERJMP, GetPosition_DwordRounte);
	GetPosition_DwordHook->SetParamOfHook ( (LPVOID)this);


	GetGridPosFeatureHook= new InlineSingleHook ( (unsigned int)GetGridPosFeature, 5, INLINE_5BYTESLAGGERJMP, GetGridPosFeatureRounte);
	GetGridPosFeatureHook->SetParamOfHook ( (LPVOID)this);

	
	for (int i= 0; i<0x15; ++i)
	{
		Cursor_Surfc= NULL;
	}

	Init ( parent_p, MegaMapScreen_p, TAMap_p, GameScreen_p, MaxIconWidth, MaxIconHeight, MegaMapVirtulKey_arg, WheelMoveMegaMap_v, DoubleClickMoveMegamap_v, WheelZoom_v);
}


MegaMapControl::~MegaMapControl()
{
	if (FindMouseUnitHook)
	{
		delete FindMouseUnitHook;
	}
	if (GetPosition_DwordHook)
	{
		delete GetPosition_DwordHook;
	}

	if (GetGridPosFeatureHook)
	{
		delete GetGridPosFeatureHook;
	}
	return ;
}

void MegaMapControl::Init (FullScreenMinimap * parent_p, RECT * MegaMapScreen_p, RECT * TAMap_p, RECT * GameScreen_p,
	int MaxIconWidth, int MaxIconHeight, int MegaMapVirtulKey_arg, BOOL WheelMoveMegaMap_v, BOOL DoubleClickMoveMegamap_v, BOOL WheelZoom_v)
{
	parent= parent_p;
	
	MegaMapVirtulKey= MegaMapVirtulKey_arg;

	TAmainStruct_Ptr= *TAmainStruct_PtrPtr;

	SelectedCount= 0;

	WheelMoveMegaMap= WheelMoveMegaMap_v;
	DoubleClickMoveMegamap= DoubleClickMoveMegamap_v;
	WheelZoom= WheelZoom_v;
	//OrderType= 
	memcpy ( &MegaMapScreen, MegaMapScreen_p, sizeof(RECT));
	MegaMapWidth= MegaMapScreen.right- MegaMapScreen.left;
	MegaMapHeight= MegaMapScreen.bottom- MegaMapScreen.top;

	memcpy ( &TAMap, TAMap_p, sizeof(RECT));
	TAMapWidth= TAMap.right- TAMap.left;
	TAMapHeight= TAMap.bottom- TAMap.top;

	Screen2MapWidthScale= static_cast<float>(MegaMapWidth)/ static_cast<float>(TAMapWidth);
	Screen2MapHeightScale= static_cast<float>(MegaMapHeight)/ static_cast<float>(TAMapHeight);

	memcpy ( &TAGameScreen, GameScreen_p, sizeof(RECT));

	Position_Dword temp;

	ScreenPos2TAPos ( &temp, MaxIconWidth, MaxIconHeight);

	HalfMaxIconWidth_TAPos= temp.X/ 2;
	HalfMaxIconHeight_TAPos= temp.Y/ 2;

	InControl= FALSE;
	InMap= FALSE;

	LastDblXPos= -1;
	LastDblYpos= -1;
}

/*

void MegaMapControl::InitSurface ( LPDIRECTDRAW TADD)
{
	if (!TADD)
	{
		return;// invalid
	}
	ReleaseSurface ( );

	LastCursor_GAFp= (*TAProgramStruct_PtrPtr)->Cursor;

	if (NULL==LastCursor_GAFp)
	{
		LastCursor_GAFp= TAmainStruct_Ptr->cursor_ary[cursornormal]->PtrFrameAry[0].PtrFrame;
	}

	if (LastCursor_GAFp)
	{

		POINT GafSize={0, 0};
		int Width= 0x30;
		int Height= 0x30;

		DDSURFACEDESC ddsd;
		DDRAW_INIT_STRUCT(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;

		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;

		ddsd.dwWidth = Width;
		ddsd.dwHeight = Height;

		TADD->CreateSurface ( &ddsd, &Cursor_Surfc, NULL);

		DDRAW_INIT_STRUCT(ddsd);

		Cursor_Surfc->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);


		GafSize.x= ddsd.lPitch;
		GafSize.y= ddsd.dwHeight;


		memset ( (ddsd.lpSurface), LastCursor_GAFp->Background, ddsd.lPitch* ddsd.dwHeight);
		CopyGafToBits ( (LPBYTE)(ddsd.lpSurface), &GafSize, 0, 0, LastCursor_GAFp);

		Cursor_Surfc->Unlock(NULL);
	}
}

void MegaMapControl::ReleaseSurface (void) 
{

	if (NULL!=Cursor_Surfc)
	{
		Cursor_Surfc->Release ( );
		Cursor_Surfc= NULL;

	}
	
}*/


POINT * MegaMapControl::TAPos2ScreenPos (POINT * ScreenPos, unsigned int TAX, unsigned int TAY, unsigned int TAZ)
{
	if (NULL==ScreenPos)
	{
		return NULL;
	}
	int TAx= TAX; 
	int TAy= TAY- TAZ/ 2;


	ScreenPos->x= static_cast<int>(static_cast<float>(TAx)* Screen2MapWidthScale);
	ScreenPos->y= static_cast<int>(static_cast<float>(TAy)* Screen2MapHeightScale);

	return ScreenPos;
}

Position_Dword * MegaMapControl::ScreenPos2TAPos (Position_Dword * TAPos, int x, int y, BOOL UseTAHeight)
{
	if (NULL==TAPos)
	{
		return NULL;
	}
	TAPos->X= static_cast<int>(static_cast<float>(x)/ Screen2MapWidthScale);
	TAPos->Y= static_cast<int>(static_cast<float>(y)/ Screen2MapHeightScale);
	if (UseTAHeight&&
		TAmainStruct_Ptr->Features)
	{
		TAPos->Z= GetPosHeight ( TAPos);
	}
	else
	{
		TAPos->Z= TAmainStruct_Ptr->SeaLevel;
	}


	//GetPosition_Dword ( TAPos->X, TAPos->Y, TAPos);
	return TAPos;
}


void MegaMapControl::EnterMegaMap ()
{
	PlaySound_Effect ( "Options", 0);

	

	// init those megamap control's data
	InControl= FALSE;
	InMap= FALSE;

	SelectState= selectbuttom::none;
	SelectedCount= CountSelectedUnits ( );
	TAmainStruct_Ptr->MouseOverUnit= 0;
	LastDblXPos= 0;
	LastDblYpos= 0;
	TAmainStruct_Ptr->BuildSpotState&= !CIRCLESELECTING;

	POINT Pos;
	PubCursorX= -1;
	PubCursorY= -1;
	if (GetCursorPos ( &Pos))
	{
		int xPos = Pos.x;  // horizontal position of cursor 
		int yPos = Pos.y;  // vertical position of cursor 

		if (IsDrawCursor ( xPos, yPos))
		{
			PubCursorX= xPos;
			PubCursorY= yPos;
		}
	}
	UpdateSelectUnitEffect ();
	ApplySelectUnitMenu_Wapper ( );


	parent->EnterMegaMap ( );
}
                                    
void MegaMapControl::QuitMegaMap ( )
{
	PlaySound_Effect ( "Previous", 0);
	parent->QuitMegaMap ( );
	InControl= FALSE;
	InMap= FALSE;
	SelectState= selectbuttom::none;
	TAmainStruct_Ptr->MouseOverUnit= 0;

	PubCursorX= -1;
	PubCursorY= -1;

	UpdateSelectUnitEffect ();
	ApplySelectUnitMenu_Wapper ( );

}

void MegaMapControl::Set (int VirtualKey)
{
	MegaMapVirtulKey= VirtualKey;
}

bool MegaMapControl::Message(HWND WinProcWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	IDDrawSurface::OutptTxt( "MapControl Message");

	bool LBMD;
	int zDelta;
	int xPos;
	int yPos;
	bool shift;
	switch(Msg)
	{
	case WM_KEYUP:
	case WM_KEYDOWN:
		if(wParam == MegaMapVirtulKey)
		{
			if (WM_KEYUP==Msg)
			{
				if (IsBliting())
				{
					QuitMegaMap ( );
				}
				else
				{
					EnterMegaMap ();
				}
			}
			return true;
		}

		break;
	case WM_MOUSEWHEEL:
		
		zDelta= (short) HIWORD(wParam);    // wheel rotation

		xPos= LOWORD(lParam);  // horizontal position of cursor 
		
		yPos= HIWORD(lParam);  // vertical position of cursor 
		if (zDelta<0)
		{
			WheelBack ( xPos, yPos);
		}
		else if (0<zDelta)
		{//
			WheelFont ( xPos, yPos);
		}
		break;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK: 
	case WM_MOUSEMOVE:

		if ( IsBliting ())
		{
			 xPos = LOWORD(lParam);  // horizontal position of cursor 
			 yPos = HIWORD(lParam);  // vertical position of cursor 

			 if (IsDrawCursor ( xPos, yPos))
			 {
				 PubCursorX= xPos;
				 PubCursorY= yPos;
			 }
			 else
			 {
				 PubCursorX= -1;
				 PubCursorY= -1;
			 }
			 

			 LBMD= MK_LBUTTON==(MK_LBUTTON& wParam);
			 shift= MK_SHIFT==(MK_SHIFT& wParam);

			if (CheckInControl ( xPos, yPos))
			{
				InControl= TRUE;
			}
			else
			{
				InControl= FALSE;
			}

			if (CheckInMap ( xPos, yPos))
			{// in map
				InMap= TRUE;
			}
			else
			{
				InMap= FALSE;
			}

			if (InControl
				&&InMap)
			{
				xPos= xPos- MegaMapScreen.left;
				yPos= yPos- MegaMapScreen.top;

				switch (Msg)
				{
				case WM_LBUTTONDOWN:
					if (! SelectDown ( xPos, yPos, false))
					{
						LeftDown ( xPos, yPos, shift);
					}
						
						
					break;
				case WM_LBUTTONDBLCLK:
					DoubleClick ( xPos, yPos, shift);
					break;
				case WM_LBUTTONUP:
					SelectedCount= CountSelectedUnits ( );
					if (!SelectUp ( xPos, yPos, false, shift))
					{
						LeftUp ( xPos, yPos, shift);
					}
						
					break;
				case WM_RBUTTONDOWN:
					RightDown( xPos, yPos, shift);
					break;
				case WM_RBUTTONUP:
					SelectedCount= CountSelectedUnits ( );
					RightUp ( xPos, yPos, shift);
					break;
				case WM_RBUTTONDBLCLK: 
					//RightDoubleClick ( xPos, yPos, shift);
					break;
				case WM_MOUSEMOVE:
					if (! SelectMove ( xPos, yPos, false, LBMD))
					{
						MouseMove ( xPos, yPos);
					}
						
					break;
				}
				
			}
			else
			{// out of map
				xPos= xPos- MegaMapScreen.left;
				yPos= yPos- MegaMapScreen.top;
				switch (Msg)
				{
					case WM_LBUTTONDOWN:
						 SelectDown ( xPos, yPos, true);
						break;
					case WM_LBUTTONUP:
						 SelectedCount= CountSelectedUnits ( );
						 SelectUp ( xPos, yPos, true, shift);
						break;
					case WM_MOUSEMOVE:
						 SelectMove ( xPos, yPos, true, LBMD);
						break;
				}
				
			}

			if (IsInControl ())
			{
				return TRUE;
			}
		}
		break;
	}

	

	return FALSE;
}

BOOL MegaMapControl::RightDown (int x, int y, bool shift)
{

	return TRUE;
}
BOOL MegaMapControl::RightUp (int x, int y, bool shift)
{


	BOOL Upt= FALSE;
	if (STOP!=TAmainStruct_Ptr->PrepareOrder_Type)
	{//
		TAmainStruct_Ptr->PrepareOrder_Type= STOP;
		Upt= TRUE;
	}
	else if (0!=TAmainStruct_Ptr->InterfaceType)
	{// R
		//STOP==PrepareOrder_Type
		if (0<SelectedCount)
		{//
			if (cursorselect==TAmainStruct_Ptr->CurrentCursora_Index)
			{// in cursorselect case will do guard 
				TAmainStruct_Ptr->PrepareOrder_Type= DEFEND;
				Upt= TRUE;
			}
			SendOrder ( TAmainStruct_Ptr->MouseMapPos.X, TAmainStruct_Ptr->MouseMapPos.Y, TAmainStruct_Ptr->MouseMapPos.Z, TAmainStruct_Ptr->PrepareOrder_Type, shift);
			Upt= TRUE;
		}
	}
	else
	{//L

// 		if (2& TAmainStruct_Ptr->BuildSpotState)
// 		{// we should back to unit GUI at this time.
// 			
// 			return TRUE;
// 		}
// 

		if (0<SelectedCount)// deselect unit when no prepare order 
		{
			DeselectUnits ();
			SelectedCount= 0;

			Upt= TRUE;
		}

		if (! IsGUIMem  ( &TAmainStruct_Ptr->desktopGUI, TAmainStruct_Ptr->CurtUnitGUIName))
		{
			Upt=TRUE;
		}
	}



	if (Upt)
	{

		if (0==SelectedCount)
		{
			IntoCurrentUnitGUI ( TRUE);
		}
		else
		{
			UpdateSelectUnitEffect ( ) ;
			ApplySelectUnitMenu_Wapper ( );
		}
	}

// 	else
// 	{
// 		TAmainStruct_Ptr->CameraToUnit= 0; 
// 		ScreenPos2TAPos ( &TAmainStruct_Ptr->MouseMapPos, x, y);
// 		MoveScreen ( TAmainStruct_Ptr->MouseMapPos.X, TAmainStruct_Ptr->MouseMapPos.Y, TAmainStruct_Ptr->MouseMapPos.Z);
// 		QuitMegaMap ( );
// 	}

	
	return TRUE;
}


BOOL MegaMapControl::LeftDown (int x, int y, bool shift)
{			

	return TRUE;
}

BOOL MegaMapControl::LeftUp (int x, int y, bool shift)
{
	BOOL Upt= FALSE;

	UnitStruct * Begin= TAmainStruct_Ptr->BeginUnitsArray_p ;
	int MouseUnit= TAmainStruct_Ptr->MouseOverUnit;
	if ((x==LastDblXPos)
		&&(y==LastDblYpos))
	{// no double clicked
		return TRUE;
	}
	LastDblXPos= -1;
	LastDblYpos= -1;

	if (cursorselect==TAmainStruct_Ptr->CurrentCursora_Index)
	{
		if (MouseUnit)
		{
			if (TAmainStruct_Ptr->Players[TAmainStruct_Ptr->LocalHumanPlayer_PlayerID].PlayerAryIndex==(TAmainStruct_Ptr->BeginUnitsArray_p [MouseUnit].Owner_PlayerPtr0->PlayerAryIndex))
			{
				if (! shift)
				{
					DeselectUnits ( );
					SelectedCount= 0;
				}
				
				if (0!=(0x20& Begin[MouseUnit].UnitSelected))
				{
					if (0.0F==(Begin[MouseUnit].Nanoframe))
					{
						if (shift)
						{
							if (0x10&Begin[MouseUnit].UnitSelected)
							{
								Begin[MouseUnit].UnitSelected&= ~0x10;
								--SelectedCount;
							}
							else
							{
								Begin[MouseUnit].UnitSelected|= 0x10;
								++SelectedCount;
							}
						}
						else
						{
							Begin[MouseUnit].UnitSelected|= 0x10;
							++SelectedCount;
						}

					}
				}

				Upt= TRUE;
			}
		}
	}
	else if (0<SelectedCount)
	{
		if (0==TAmainStruct_Ptr->InterfaceType)
		{// L
				if (cursorselect!=TAmainStruct_Ptr->CurrentCursora_Index)
				{
					if (BUILD!=TAmainStruct_Ptr->PrepareOrder_Type)
					{
						SendOrder ( TAmainStruct_Ptr->MouseMapPos.X, TAmainStruct_Ptr->MouseMapPos.Y, TAmainStruct_Ptr->MouseMapPos.Z, TAmainStruct_Ptr->PrepareOrder_Type, shift);
					}

					UpdateSelectUnitEffect ( );
					ApplySelectUnitMenu_Wapper  ( );
				}
		}
		else
		{// R 

			
			DeselectUnits ();
			SelectedCount= 0;

			
			Upt= TRUE;
			
		}
	}

	if (0!=TAmainStruct_Ptr->InterfaceType
		&&! IsGUIMem  ( &TAmainStruct_Ptr->desktopGUI, TAmainStruct_Ptr->CurtUnitGUIName))
	{
		Upt=TRUE;
	}

	if (Upt)
	{

		if (0==SelectedCount)
		{
			IntoCurrentUnitGUI ( TRUE);
		}
		else
		{
			UpdateSelectUnitEffect ( ) ;
			ApplySelectUnitMenu_Wapper ( );
		}
	}

	return TRUE;
}

BOOL MegaMapControl::DoubleClick (int x, int y, bool shift)
{
	int SelectedUnit= TAmainStruct_Ptr->MouseOverUnit;
	LastDblXPos= x;
	LastDblYpos= y;


	if (0!= SelectedUnit)
	{
		if (myExternQuickKey->DoubleClick)
		{
// 			LastDblXPos= x;
// 			LastDblYpos= y;

			UnitStruct * Begin= TAmainStruct_Ptr->BeginUnitsArray_p ;

			DeselectUnits ( );
			SelectedCount= 0;

			if (TAmainStruct_Ptr->Players[TAmainStruct_Ptr->LocalHumanPlayer_PlayerID].PlayerAryIndex==(TAmainStruct_Ptr->BeginUnitsArray_p [SelectedUnit].Owner_PlayerPtr0->PlayerAryIndex))
			{
				Begin[SelectedUnit].UnitSelected|= 0x10;
				SelectAllSelectedUnits ( );
				SelectedCount= CountSelectedUnits ( );

				UpdateSelectUnitEffect ( );
				ApplySelectUnitMenu_Wapper ( );

				return TRUE;
			}

			IntoCurrentUnitGUI ( TRUE);

			if (0==TAmainStruct_Ptr->InterfaceType)
			{//LEFT, in Left interface, double click on any units just  won't do the zoom thing.
				return TRUE;
			}
			// in right interface, double click in not local player unit, will zoom in.
		}

	} 
	
	if (DoubleClickMoveMegamap)
	{// move screen


		MegamapMoveSceen ( x+ MegaMapScreen.left, y+ MegaMapScreen.top);
		QuitMegaMap  ( );
	}

	return TRUE;
}



BOOL MegaMapControl::WheelFont (int xPos, int yPos)
{
	if (WheelZoom)
	{
		if (IsBliting())
		{
			TAmainStruct_Ptr->CameraToUnit= 0; 

			if (WheelMoveMegaMap)
			{
				MegamapMoveSceen ( xPos, yPos);
			}
			QuitMegaMap ( );
		}
	}

	return TRUE;
}

BOOL MegaMapControl::WheelBack (int xPos, int yPos)
{
	if (WheelZoom)
	{
		if (! IsBliting())
		{
			EnterMegaMap ();
		}
	}

	return TRUE;
}
BOOL MegaMapControl::IsBliting(void)
{
	return parent->IsBliting();
}

BOOL MegaMapControl::CheckInControl (int xPos, int yPos)
{
	//04CCC129  53 4F 55 4E 44 53 52 54 2E 47 55 49              
	//04CCC129  4D 55 53 49 43 52 54 2E 47 55 49                  
	//061E5AF1  53 50 45 45 44 53 52 54 2E 47 55 49              
	
	//061E5AF1  56 49 53 55 41 4C 52 54 2E 47 55 49              

	RECT topControlRect;

	if (TAmainStruct_Ptr->desktopGUI.TheActive_GUIMEM
		&&(NULL!=TAmainStruct_Ptr->desktopGUI.TheActive_GUIMEM->per_active)
		&&TAmainStruct_Ptr->desktopGUI.TheActive_GUIMEM->ControlsAry)
	{
		topControlRect.left=  TAmainStruct_Ptr->desktopGUI.TheActive_GUIMEM->ControlsAry->xpos;
		topControlRect.right= topControlRect.left+ TAmainStruct_Ptr->desktopGUI.TheActive_GUIMEM->ControlsAry->width;
		topControlRect.top=  TAmainStruct_Ptr->desktopGUI.TheActive_GUIMEM->ControlsAry->ypos;
		topControlRect.bottom= topControlRect.top+ TAmainStruct_Ptr->desktopGUI.TheActive_GUIMEM->ControlsAry->height;

		if ((topControlRect.left<=xPos)
			&&(xPos<=topControlRect.right)
			&&(topControlRect.top<=yPos)
			&&(yPos<=topControlRect.bottom))
		{
			return FALSE;
		}
	}
	return (xPos<TAGameScreen.right)
		&&(TAGameScreen.left<xPos)
		&&(yPos<TAGameScreen.bottom)
		&&(TAGameScreen.top<yPos);
}

BOOL MegaMapControl::CheckInMap (int xPos, int yPos)
{
	return (xPos<(MegaMapScreen.right))
		&&(MegaMapScreen.left<xPos)
		&&(yPos<(MegaMapScreen.bottom))
		&&(MegaMapScreen.top<yPos);
}

BOOL MegaMapControl::IsInControl(void)
{
	return InControl;
}

BOOL MegaMapControl::IsInMap(void)
{
	return InMap;
}



BOOL MegaMapControl::IsDrawCursor (int xPos, int yPos)
{
	RECT topControlRect;
	if (((Dialog *)LocalShare->Dialog)->IsShow ( &topControlRect))
	{
		if ((topControlRect.left<=xPos)
			&&(xPos<=topControlRect.right)
			&&(topControlRect.top<=yPos)
			&&(yPos<=topControlRect.bottom))
		{
			return FALSE;
		}
	}

	if (((CIncome *)LocalShare->Income)->IsShow ( &topControlRect))
	{
		if ((topControlRect.left<=xPos)
			&&(xPos<=topControlRect.right)
			&&(topControlRect.top<=yPos)
			&&(yPos<=topControlRect.bottom))
		{
			return FALSE;
		}
	}

	if (TAmainStruct_Ptr->desktopGUI.TheActive_GUIMEM
		&&(NULL!=TAmainStruct_Ptr->desktopGUI.TheActive_GUIMEM->per_active)
		&&TAmainStruct_Ptr->desktopGUI.TheActive_GUIMEM->ControlsAry)
	{

		topControlRect.left=  TAmainStruct_Ptr->desktopGUI.TheActive_GUIMEM->ControlsAry->xpos;
		topControlRect.right= topControlRect.left+ TAmainStruct_Ptr->desktopGUI.TheActive_GUIMEM->ControlsAry->width;
		topControlRect.top=  TAmainStruct_Ptr->desktopGUI.TheActive_GUIMEM->ControlsAry->ypos;
		topControlRect.bottom= topControlRect.top+ TAmainStruct_Ptr->desktopGUI.TheActive_GUIMEM->ControlsAry->height;

		PGAFSequence CursorSequence= (*TAmainStruct_PtrPtr)->cursor_ary[cursornormal];
		if (NULL!=CursorSequence)
		{
			PGAFFrame GafFrame= CursorSequence->PtrFrameAry[0].PtrFrame;
			topControlRect.left-= GafFrame->Width;
			topControlRect.top-= GafFrame->Height;
		}

		if ((topControlRect.left<=xPos)
			&&(xPos<=topControlRect.right)
			&&(topControlRect.top<=yPos)
			&&(yPos<=topControlRect.bottom))
		{
			return TRUE;
		}
	}

	return (xPos<TAGameScreen.right)
		&&((TAGameScreen.left- MAXCURSORWIDTH/ 2)<xPos)
		&&(yPos<TAGameScreen.bottom)
		&&((TAGameScreen.top- MAXCURSORHEIGHT/ 2)<yPos);
}

BOOL MegaMapControl::MouseMove (int x, int y)
{

	if (IsInMap ( ))
	{
		ScreenPos2TAPos ( &TAmainStruct_Ptr->MouseMapPos, x, y, TRUE);

		RECT MouseRect;
		MouseRect.left= TAmainStruct_Ptr->MouseMapPos.X- HalfMaxIconWidth_TAPos;
		MouseRect.right= TAmainStruct_Ptr->MouseMapPos.X+ HalfMaxIconWidth_TAPos;
		MouseRect.top= TAmainStruct_Ptr->MouseMapPos.Y- HalfMaxIconHeight_TAPos;
		MouseRect.bottom= TAmainStruct_Ptr->MouseMapPos.Y+ HalfMaxIconHeight_TAPos;

		int Count= TAmainStruct_Ptr->NumHotRadarUnits;

		RadarUnit_ * RadarUnits_v= (*TAmainStruct_PtrPtr)->RadarUnits;
		UnitStruct * Begin= TAmainStruct_Ptr->BeginUnitsArray_p ;

		UnitStruct * unitPtr;

		BOOL UnitUnderMouse= FALSE;
		for (int i= 0; i<Count; ++i)
		{
			unitPtr=  &Begin[RadarUnits_v[i].ID];
			int X= static_cast<int>(unitPtr->XPos+ unitPtr->UnitType->FootX/ 2);
			int Y= static_cast<int>(unitPtr->YPos+ unitPtr->UnitType->FootY/ 2- unitPtr->ZPos/ 2);
			if ((MouseRect.left<X)
				&&(X<MouseRect.right)
				&&(MouseRect.top<Y)
				&&(Y<MouseRect.bottom))
			{
				POINT GafAspect;
				parent->UnitsMap->UnitPicture ( unitPtr, TAmainStruct_Ptr->LocalHumanPlayer_PlayerID, NULL, &GafAspect);
				
				Position_Dword GafAspectTA;
				ScreenPos2TAPos ( &GafAspectTA, GafAspect.x, GafAspect.y);

				MouseRect.left= TAmainStruct_Ptr->MouseMapPos.X- GafAspectTA.X/ 2;
				MouseRect.right=  MouseRect.left+ GafAspectTA.X;
				MouseRect.top= TAmainStruct_Ptr->MouseMapPos.Y- GafAspectTA.Y/ 2;
				MouseRect.bottom=  MouseRect.top+ GafAspectTA.Y;
				if ((MouseRect.left<X)
					&&(X<MouseRect.right)
					&&(MouseRect.top<Y)
					&&(Y<MouseRect.bottom))
				{
					TAmainStruct_Ptr->MouseOverUnit= RadarUnits_v[i].ID;
					UnitUnderMouse= TRUE;
					break;
				}
			}
		}

		if (!UnitUnderMouse)
		{
			TAmainStruct_Ptr->MouseOverUnit= 0;
		}
/*
		if (0!=TAmainStruct_Ptr->MouseOverUnit)
		{
			if (STOP==TAmainStruct_Ptr->PrepareOrder_Type)
			{
				int NewCursorIndex= CorretCursor_InGame ( TAmainStruct_Ptr->PrepareOrder_Type);

				if (NewCursorIndex!=TAmainStruct_Ptr->CurrentCursora_Index)
				{
					TAmainStruct_Ptr->CurrentCursora_Index= NewCursorIndex;
					SetUICursor ( &(TAmainStruct_Ptr->desktopGUI), TAmainStruct_Ptr->cursor_ary[NewCursorIndex]);
				}
			}
		}*/
	}

	return TRUE;
}

BOOL MegaMapControl::SelectDown (int x, int y, bool out)
{
	if (false==out)
	{
		SelectState= selectbuttom::down;

		SelectScreenRect.left= x;
		SelectScreenRect.top= y;

		SelectTick= GetTickCount ( );
	}
	return FALSE;
}

BOOL MegaMapControl::SelectUp (int x, int y, bool out, bool shift)
{
	BOOL Rtn_b= FALSE;

	UnitStruct * Begin= TAmainStruct_Ptr->BeginUnitsArray_p ;


	if (selectbuttom::select==SelectState)
	{
		SelectState= selectbuttom::up;
		// do select
		LONG Tmp;
		Position_Dword TmpPos;
		if (SelectScreenRect.right<SelectScreenRect.left)
		{
			Tmp= SelectScreenRect.left;
			SelectScreenRect.left= SelectScreenRect.right;
			SelectScreenRect.right= Tmp;
		}

		if (SelectScreenRect.bottom<SelectScreenRect.top)
		{
			Tmp= SelectScreenRect.top;
			SelectScreenRect.top= SelectScreenRect.bottom;
			SelectScreenRect.bottom= Tmp;
		}


		if ((MINSELECTHEIGHT<abs ( SelectScreenRect.bottom- SelectScreenRect.top))
			&&(MINSELECTWIDTH<abs ( SelectScreenRect.right- SelectScreenRect.left))
			&&((SelectTick/ 1000)<= (GetTickCount ( )/ 1000)))
		{
			ScreenPos2TAPos ( &TmpPos, SelectScreenRect.left, SelectScreenRect.top);
			SelectScreenRect.left= TmpPos.X;
			SelectScreenRect.top= TmpPos.Y;

			ScreenPos2TAPos ( &TmpPos, SelectScreenRect.right, SelectScreenRect.bottom);
			SelectScreenRect.right= TmpPos.X;
			SelectScreenRect.bottom= TmpPos.Y;
			myExternQuickKey->SelectUnitInRect ( ALL, &SelectScreenRect, shift);
			SelectedCount= myExternQuickKey->FilterSelectedUnit ( );
			//
			UpdateSelectUnitEffect ();
			ApplySelectUnitMenu_Wapper ( );

			Rtn_b= TRUE;
		}
	}
	else if ((! out)
		&&(0<SelectedCount))
	{
		if (STOP!=TAmainStruct_Ptr->PrepareOrder_Type)
		{
			//
			MOUSEEVENT MEvent;
			memset ( &MEvent, 0, sizeof(MOUSEEVENT));
			MEvent.fwKeys= shift? 4: 0;

		//	ScreenPos2TAPos ( &TAmainStruct_Ptr->MouseMapPos, x, y, TRUE);

			TAMapClick ( &MEvent);

			Rtn_b= TRUE;
		}
	}


	SelectState= selectbuttom::none;
	return Rtn_b;
}
BOOL MegaMapControl::SelectMove (int x, int y, bool Out_b, bool LBMD)
{
	if (STOP==TAmainStruct_Ptr->PrepareOrder_Type)
	{
		if ((selectbuttom::down==SelectState
			||selectbuttom::select==SelectState)
			)
		{
			SelectState= selectbuttom::select;

			if (Out_b)
			{
				if (x<0)
				{
					x= 0;
				}
				if ((MegaMapWidth- 1)<x)
				{
					x= MegaMapWidth- 1;
				}

				if (y<0)
				{
					y= 0;
				}
				if ((MegaMapHeight- 1)<y)
				{
					y= MegaMapHeight- 1;
				}
			}
			SelectScreenRect.right= x;
			SelectScreenRect.bottom= y;
			return TRUE;
		}
	}
	else
	{
		SelectState= selectbuttom::none;
	}


	return FALSE;
}

selectbuttom::SELECTBUTTOM MegaMapControl::ReadSelectState (void)
{
	return SelectState;
}

RECT * MegaMapControl::ReadSelectRect (RECT * rect_p)
{
	if (rect_p)
	{
		memcpy ( rect_p, &SelectScreenRect, sizeof(RECT));
	}

	return &SelectScreenRect;
}

BOOL MegaMapControl::IsDrawOrder (void)
{
	return (0!= GetAsyncKeyState ( VK_SHIFT));
}

BOOL MegaMapControl::IsDrawRect (BOOL Build_b)
{
	if (Build_b)
	{
		return (BUILD==TAmainStruct_Ptr->PrepareOrder_Type);
	}
	return (selectbuttom::select==SelectState
		&&(MINSELECTHEIGHT<abs ( SelectScreenRect.bottom- SelectScreenRect.top))
		&&(MINSELECTWIDTH<abs ( SelectScreenRect.right- SelectScreenRect.left))
		&&((SelectTick/ 1000)<= (GetTickCount ( )/ 1000)))
		||(BUILD==TAmainStruct_Ptr->PrepareOrder_Type);
}


void MegaMapControl::MegamapMoveSceen (int xPos, int yPos)
{
	if (MegaMapScreen.right<xPos)
	{
		xPos=MegaMapScreen.right;
	}
	if (xPos<MegaMapScreen.left)
	{
		xPos= MegaMapScreen.left;
	}
	if (MegaMapScreen.bottom<yPos)
	{
		yPos=MegaMapScreen.bottom;
	}
	if (yPos<MegaMapScreen.top)
	{
		yPos= MegaMapScreen.top;
	}


	xPos= xPos- MegaMapScreen.left;
	yPos= yPos- MegaMapScreen.top;



	ScreenPos2TAPos ( &TAmainStruct_Ptr->MouseMapPos, xPos, yPos);


	MoveScreen ( TAmainStruct_Ptr->MouseMapPos.X+ (MegaMapWidth/ 2- xPos), TAmainStruct_Ptr->MouseMapPos.Y+ ( MegaMapHeight/ 2- yPos), TAmainStruct_Ptr->MouseMapPos.Z);

}

void MegaMapControl::MoveScreen ( int TAX,  int TAY,  int TAZ)
{
	//ScrollToCenter ( TAX, TAY- TAZ/ 2);

	int *PTR = (int*)TAmainStruct_PtrPtr;
	int *XPointer = (int*)(*PTR + 0x1431f);
	int *YPointer = (int*)(*PTR + 0x14323);
	
 	TAX -= (((*TAProgramStruct_PtrPtr)->ScreenWidth)-(*TAmainStruct_PtrPtr)->GameSreen_Rect.left)/2;
 	TAY -= (((*TAProgramStruct_PtrPtr)->ScreenHeight)-(*TAmainStruct_PtrPtr)->GameSreen_Rect.top)/2;

	if(TAX<0)
		TAX = 0;
	if(TAY<0)
		TAY = 0;
	if(TAX>(GetMaxScrollX()))
		TAX = (GetMaxScrollX());
	if(TAY>(GetMaxScrollY()))
		TAY = (GetMaxScrollY());

	(*TAmainStruct_PtrPtr)->EyeBallMapXPos= TAX;
	(*TAmainStruct_PtrPtr)->EyeBallMapYPos= TAY;
	(*TAmainStruct_PtrPtr)->MapXScrollingTo= TAX;
	(*TAmainStruct_PtrPtr)->MapYScrollingTo= TAY;

	UpdateLosState ( 0);
	//ScrollMinimap ( );
	//041C3C0
}

#endif