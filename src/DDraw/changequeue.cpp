#include "oddraw.h"
#include "changequeue.h"
#include "iddrawsurface.h"
#include <stdio.h>
#include "tamem.h"

TAdynmemStruct *TAdynmem;

bool pressed = false;
HWND GWinProchWnd;

CChangeQueue::CChangeQueue()
{
	LocalShare->ChangeQueue = this;

	int *PTR = (int*)0x00511de8;
	TAdynmem = (TAdynmemStruct*)(*PTR);

	IDDrawSurface::OutptTxt ( "New CChangeQueue");
}

CChangeQueue::~CChangeQueue()
{

}

void Write(HWND WinProchWnd)
{
	/*char Mtmp[100];
	wsprintf(Mtmp, "IsUnit: %i", &TAdynmem->BeginUnitsArray_p [0].IsUnit);
	OutptTxt(Mtmp);
	wsprintf(Mtmp, "Weapon1: %i", &TAdynmem->BeginUnitsArray_p [0].Weapon1);
	OutptTxt(Mtmp);
	wsprintf(Mtmp, "Weapon2: %i", &TAdynmem->BeginUnitsArray_p [0].Weapon2);
	OutptTxt(Mtmp);
	wsprintf(Mtmp, "Weapon3: %i", &TAdynmem->BeginUnitsArray_p [0].Weapon3);
	OutptTxt(Mtmp);

	wsprintf(Mtmp, "Xpos: %i", &TAdynmem->BeginUnitsArray_p [0].XPos);
	OutptTxt(Mtmp);

	wsprintf(Mtmp, "Orders: %i", TAdynmem->BeginUnitsArray_p [0].UnitOrders);
	OutptTxt(Mtmp);
	wsprintf(Mtmp, "UnitSelected: %i", TAdynmem->BeginUnitsArray_p [0].UnitSelected);
	OutptTxt(Mtmp);*/


	UnitOrdersStruct *LastOrder = TAdynmem->BeginUnitsArray_p [0].UnitOrders;
	UnitOrdersStruct *NewLastOrder;
	while(LastOrder->NextOrder)
	{
		NewLastOrder = LastOrder;
		LastOrder = LastOrder->NextOrder;

	}

	LastOrder->NextOrder = TAdynmem->BeginUnitsArray_p [0].UnitOrders;
	TAdynmem->BeginUnitsArray_p [0].UnitOrders = LastOrder;

	NewLastOrder->NextOrder = NULL; 


}

void CChangeQueue::Blit(LPDIRECTDRAWSURFACE DestSurf)
{

}

bool CChangeQueue::Message(HWND WinProchWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_KEYDOWN:
		if(wParam == 67) //c
		{
			TAdynmem->BeginUnitsArray_p [0].UnitOrders->Order_State |= 0x10;
			Write(WinProchWnd);
			//*((int*)NULL) = 1;
			return true;
		}
		break;
	case WM_KEYUP:
		if(wParam == 67) //c
		{
			TAdynmem->BeginUnitsArray_p [0].UnitOrders->Order_State &= !0x10;
			return true;
		}
		break;
	}

	return false;
}

