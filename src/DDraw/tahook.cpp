#include "oddraw.h"
#include "tamem.h"
#include "tafunctions.h"
#include "iddrawsurface.h"
#include "whiteboard.h"
//#include "font.h"              
#include <algorithm>
#include <stdio.h>
#include <sstream>
#include "Gaf.h"
#include "pcxread.h"
#include "rings.h"
#include "hook\etc.h"
#include "hook\hook.h"
#include "tahook.h"
#include "TAConfig.h"

#include "fullscreenminimap.h"
#include "GUIExpand.h"

#include "megamaptastuff.h"
#include "MegamapControl.h"
#include "HackableOptions.h"

#include <algorithm>
#include <list>
#include <tuple>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#ifdef WM_MOUSEWHEEL//vs2010
#undef WM_MOUSEWHEEL
#endif
#define WM_MOUSEWHEEL 522
#define MAX_SPACING 10

CTAHook* TAHook;

unsigned int SuppressShowReclaimCursorAddr = 0x4992a2;
int __stdcall SuppressShowReclaimCursorProc(PInlineX86StackBuffer X86StrackBuffer)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	GUIInfo* topGuiInfo = &taPtr->desktopGUI;
	unsigned currentOrderTYpe = X86StrackBuffer->Eax;

	if (TAHook->IsWreckSnapping() && ((GAFSequence*)topGuiInfo->Cursor_3 != taPtr->cursor_ary[cursornormal] || taPtr->CurrentCursora_Index != cursorreclamate)) {
		taPtr->CurrentCursora_Index = cursorreclamate;;
		// SetUiCursor(normal)
		X86StrackBuffer->Eax = (DWORD) taPtr->cursor_ary[cursornormal];
		X86StrackBuffer->Ecx = (DWORD) topGuiInfo;
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x4992c6;
		return X86STRACKBUFFERCHANGE;
	}
	else if (!TAHook->IsWreckSnapping() && taPtr->CurrentCursora_Index != currentOrderTYpe) {
		// SetUiCursor(currentOrderType)
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x4992ad;
		return X86STRACKBUFFERCHANGE;
	}
	else {
		// do nothing
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x4992cd;
		return X86STRACKBUFFERCHANGE;
	}
}

unsigned int ReduceCancelOrderToleranceAddr = 0x43b006;
int __stdcall ReduceCancelOrderToleranceProc(PInlineX86StackBuffer X86StrackBuffer)
{
	if (TAHook->IsWreckSnapping()) {
		const UnitOrdersStruct* orders = (UnitOrdersStruct*)(X86StrackBuffer->Esi);
		const Position_Dword* clickPosition = (Position_Dword*)(X86StrackBuffer->Eax);
		const unsigned short tolerance = 8u;

		if (clickPosition->X - orders->Pos.X + tolerance < 2u * tolerance &&
			clickPosition->Y - orders->Pos.Y + tolerance < 2u * tolerance) {
			// cancel old order
			X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x43b034;
			return X86STRACKBUFFERCHANGE;
		}
		else {
			// append new order
			X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x43b02b;
			return X86STRACKBUFFERCHANGE;
		}

	}
	else {
		return 0;
	}
}

CTAHook::CTAHook(BOOL VidMem)
{
	lpRectSurf = NULL;

	if(DataShare->ehaOff == 1)
		return;

	HKEY hKey;
	HKEY hKey1;
	DWORD dwDisposition;
	RegCreateKeyEx(HKEY_CURRENT_USER, CompanyName_CCSTR, NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey1, &dwDisposition);

	RegCreateKeyEx(hKey1, "TAHook", NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
	//write tahook ver string for the .hookreport function in recorder
	char VerString [] = "Swedish Eye ver 0.8";
	RegSetValueEx(hKey, "Ver", NULL, REG_SZ, (unsigned char*)VerString, strlen(VerString));
	RegCloseKey(hKey);
	RegCloseKey(hKey1);


	LocalShare->TAHook = this;
	TAHook = this;

	HWND TopWindow;
	TopWindow = GetForegroundWindow();
	char Text[255];
	GetClassName(TopWindow,Text,254);
	if(strcmp(Text,"Total Annihilation Class")==0)
		TAhWnd = TopWindow;

	QueuePos = 0;
	Delay = 5;
	MexSnapRadius = CTAHook::GetDefaultMexSnapRadius();
	WreckSnapRadius = CTAHook::GetDefaultWreckSnapRadius();
	ClickSnapOverrideKey = VK_LMENU;
	WriteLine = false;
	FootPrintX = FootPrintY = 2;
	ScrollEnabled = LocalShare->CompatibleVersion;
	RingWrite = false;
	Spacing = 0;
	DraggingUnitOrders = NULL;
	DraggingUnitOrdersState = DraggingOrderStateEnum::IDLE;
	ClickSnapPreviewBuild = false;
	ClickSnapPreviewWreck = false;

	lpRectSurf = CreateSurfPCXResource(50, VidMem);

	InterpretCommand = (void (__stdcall *)(char *Command, int Unk1))0x417B50;
	TAMapClick = (void (__stdcall *)(void *msgstruct))0x498F70;
	TestBuildSpot = (void (__stdcall *)(void))0x4197D0;
	TADrawRect = (void (__stdcall *)(tagRECT *unk, tagRECT *rect, int color))0x4BF8C0;
	FindMouseUnit = (unsigned short (__stdcall *)(void))0x48CD80;
	SendText = (int (__stdcall *)(char*, int))0x46bc70;
	ShowText = (void (__stdcall *)(PlayerStruct *Player, char *Text, int Unk1, int Unk2))0x463E50;

	int *PTR = (int*)0x00511de8;
	TAdynmem = (TAdynmemStruct*)(*PTR);

	m_hooks.push_back(std::make_unique<InlineSingleHook>(SuppressShowReclaimCursorAddr, 5, INLINE_5BYTESLAGGERJMP, SuppressShowReclaimCursorProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(ReduceCancelOrderToleranceAddr, 5, INLINE_5BYTESLAGGERJMP, ReduceCancelOrderToleranceProc));

	IDDrawSurface::OutptTxt ( "New CTAHook");
}


CTAHook::~CTAHook()
{
	if(lpRectSurf)
		lpRectSurf->Release();

	lpRectSurf= NULL;
	if(DataShare->ehaOff == 1)
		return;
}

struct CountFeetExceedingSurfaceMetalAdapter
{
	static int count(int x, int y, int footX, int footY)
	{
		TAdynmemStruct* Ptr = *(TAdynmemStruct**)0x00511de8;
		if (!Ptr || !Ptr->GameingState_Ptr) {
			return 0;
		}

		const int threshold = Ptr->GameingState_Ptr->surfaceMetal;

		const int dxMin = -footX / 2;
		const int dxMax = footX % 2 ? footX / 2 : footX / 2 - 1;
		const int dyMin = -footY / 2;
		const int dyMax = footY % 2 ? footY / 2 : footY / 2 - 1;

		int count = 0;
		for (int dx = dxMin; dx <= dxMax; ++dx) {
			for (int dy = dyMin; dy <= dyMax; ++dy) {
				if (x + dx < 0 || y + dy < 0 || x + dx >= Ptr->FeatureMapSizeX || y + dy >= Ptr->FeatureMapSizeY) {
					continue;
				}
				const int idx = (x + dx) + (y + dy) * Ptr->FeatureMapSizeX;
				int metal = (int)Ptr->FeatureMap[idx].MetalValue;
				if (metal > threshold)
				{
					++count;
				}
			}
		}
		return count;
	}
};

struct TestCanBuildAdapter
{
	static int count(int x, int y, int footX, int footY)
	{
		TAdynmemStruct* Ptr = *(TAdynmemStruct**)0x00511de8;
		if (!Ptr) {
			return 0;
		}

		UnitDefStruct* unit = &Ptr->UnitDef[Ptr->BuildUnitID];

		// For completeness, this is exacatly how TA calculates the coordinate to test.
		// It seems to differ a pixel or two from just using Ptr->BuildPosX,Y
		// Note the use of index [2] for the y coordinate
		// unsigned mousePositionX = ((0x80000 + Ptr->MouseMapPos[0] - (footX << 19)) >> 20) & 0xffff;
		// unsigned mousePositionY = ((0x80000 + Ptr->MouseMapPos[2] - (footY << 19)) >> 20) & 0xffff;
		// unsigned packedMousePositionXY = (mousePositionY << 16) | mousePositionX;

		const unsigned mousePositionX = (x - footX / 2) & 0xffff;
		const unsigned mousePositionY = (y - footY / 2) & 0xffff;
		const unsigned packedMousePositionXY = (mousePositionY << 16) | mousePositionX;
		return TestGridSpot(unit, packedMousePositionXY, 0, &Ptr->Players[Ptr->LocalHumanPlayer_PlayerID]);
	}
};

struct CountReclaimableAdapter
{
	static int count(int x, int y, int footX, int footY)
	{
		TAdynmemStruct* Ptr = *(TAdynmemStruct**)0x00511de8;
		if (!Ptr) {
			return 0;
		}

		if (x < 0 || y < 0 || x >= Ptr->FeatureMapSizeX || y >= Ptr->FeatureMapSizeY) {
			return 0;
		}
		FeatureStruct* f = &Ptr->FeatureMap[x + y * Ptr->FeatureMapSizeX];
		unsigned idx = GetGridPosFeature(f);
		if (idx < Ptr->NumFeatureDefs) {
			return (Ptr->FeatureDef[idx].Metal > 0 || Ptr->FeatureDef[idx].Energy > 0) &&
				(Ptr->FeatureDef[idx].FeatureMask & (unsigned)FeatureMasks::reclaimable);
		}
		else {
			return 0;
		}
	}
};

static void FindFeatureCentre(const int posXY[2], double centreXY[2])
{
	TAdynmemStruct* Ptr = *(TAdynmemStruct**)0x00511de8;
	if (!Ptr) {
		return;
	}

	const int x = posXY[0];
	const int y = posXY[1];
	centreXY[0] = x;
	centreXY[1] = y;

	if (x < 0 || y < 0 || x >= Ptr->FeatureMapSizeX || y >= Ptr->FeatureMapSizeY) {
		return;
	}

	FeatureStruct* f = &Ptr->FeatureMap[x + y * Ptr->FeatureMapSizeX];

	unsigned idx = GetGridPosFeature(f);
	if (idx < Ptr->NumFeatureDefs) {
		int left = x;
		int top = y;
		if (f->FeatureDefIndex == 0xfffe) {
			left -= f->FeatureDefDx;
			top -= f->FeatureDefDy;
		}
		int right = left + Ptr->FeatureDef[idx].FootprintX - 1;
		int bottom = top + Ptr->FeatureDef[idx].FootprintZ - 1;
		centreXY[0] = double(left + right) / 2.0;
		centreXY[1] = double(top + bottom) / 2.0;
	}
}


template<typename PositionTestFunctor>
int SnapToNear(int xyPos[2], int footX, int footY, int R, double mouseMapPosX, double mouseMapPosY)
{
	std::vector<std::tuple<int, int, int, double> > sums;
	sums.reserve((1 + R) * (1 + R));
	for (int dx = -R; dx <= R; ++dx) {
		for (int dy = -R; dy <= R; ++dy) {
			int count = PositionTestFunctor::count(xyPos[0] + dx, xyPos[1] + dy, footX, footY);
			double distx = xyPos[0] + dx - mouseMapPosX/16.0 + 0.5;
			double disty = xyPos[1] + dy - mouseMapPosY/16.0 + 0.5;
			double distsq = distx * distx + disty * disty;
			if (count > 0) {
				sums.push_back(std::make_tuple(dx, dy, count, distsq));
			}
		}
	}

	if (sums.empty()) {
		return 0;
	}

	auto itMaxSums = std::max_element(sums.begin(), sums.end(),
		[](const std::tuple<int, int, int, double>& a, const std::tuple<int, int, int, double>& b) { 
		return std::get<2>(a) < std::get<2>(b);
	});

	std::vector<std::tuple<int, int, int, double> > maxima;
	std::copy_if(sums.begin(), sums.end(), std::back_inserter(maxima), [itMaxSums](const std::tuple<int, int, int, double>& x) {
		return std::get<2>(x) == std::get<2>(*itMaxSums);
	});

	auto itClosestMax = std::min_element(maxima.begin(), maxima.end(),
		[](const std::tuple<int, int, int, double>& a, const std::tuple<int, int, int, double>& b) { 
		return std::get<3>(a) < std::get<3>(b);
	});

	xyPos[0] += std::get<0>(*itClosestMax);
	xyPos[1] += std::get<1>(*itClosestMax);
	return std::get<2>(*itClosestMax);
}

bool CTAHook::Message(HWND WinProcWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	__try
	{

		if (TAInGame==DataShare->TAProgress)
		{
			switch(Msg)
			{
			case WM_KEYDOWN:
				switch((int)wParam)
				{
				case VK_F9:
					if ((*TAmainStruct_PtrPtr)->SoftwareDebugMode & softwaredebugmode::CheatsEnabled)
					{
						/*
						if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
							// cycle through map debug modes
							TAdynmem->mapDebugMode = (TAdynmem->mapDebugMode + 1) % 8;
						}
						else {
							// cycle through which element of TAdynmem->FeatureMap to annotate mapDebugMode with
							static char field = 0;
							char radix = 16;
							WriteProcessMemory(GetCurrentProcess(), (void*)(0x418A7D + 2), &field, 1, NULL);
							WriteProcessMemory(GetCurrentProcess(), (void*)(0x418A81 + 1), &radix, 1, NULL);
							field = (field + 1) % 13;
						}
						*/
					}
					break;
				case VK_F11:
					WriteShareMacro();
					return true;

				case VK_PRIOR: //pageup
					if((GetAsyncKeyState(VirtualKeyCode)&0x8000)>0)
					{
						Spacing++;
						if(Spacing>MAX_SPACING)
							Spacing = MAX_SPACING;
						UpdateSpacing();
						return true;
					}

				case VK_NEXT:  //pagedown
					if((GetAsyncKeyState(VirtualKeyCode)&0x8000)>0)
					{
						Spacing--;
						if(Spacing<0)
							Spacing = 0;
						UpdateSpacing();
						return true;
					}

				}
				break;

			case WM_KEYUP:
				if(wParam == VirtualKeyCode)
				{
					WriteLine = false;
					RingWrite = false;
					EnableTABuildRect();
				}
				break;


			case WM_CHAR:
				if((GetAsyncKeyState(VirtualKeyCode)&0x8000)>0 && WriteLine==true)
					return true;
				break;

			case WM_LBUTTONDOWN:
				if(RingWrite)
				{
					WriteDTLine();
					return true;
				}
				else if(WriteLine==true && (GetAsyncKeyState(VirtualKeyCode)&0x8000)>0)
				{
					FootPrintX = GetFootX();
					if(FootPrintX == 0)
						FootPrintX = 1;
					FootPrintY = GetFootY();
					if(FootPrintY == 0)
						FootPrintY = 1;

					EndX= TAdynmem->MouseMapPos.X;
					EndY= TAdynmem->MouseMapPos.Y;
					//EndX = (LOWORD(lParam)-128) + TAdynmem->EyeBallMapXPos;
					//EndY = (HIWORD(lParam)) + TAdynmem->EyeBallMapYPos - 32 + TAdynmem->CircleSelect_Pos1TAz/2;
					/*if(ScrollEnabled)
					WriteScrollDTLine();
					else
					WriteDTLine();*/
					WriteDTLine();


					StartX= ClickSnapPreviewBuild ? this->ClickSnapPreviewPosXY[0] * 16 : TAdynmem->MouseMapPos.X;
					StartY= ClickSnapPreviewBuild ? this->ClickSnapPreviewPosXY[1] * 16 : TAdynmem->MouseMapPos.Y;
					//StartX = (LOWORD(lParam)-128) + TAdynmem->EyeBallMapXPos;
					//StartY = (HIWORD(lParam)) + TAdynmem->EyeBallMapYPos - 32 + TAdynmem->CircleSelect_Pos1TAz/2;
					/*if(ScrollEnabled)
					{
					StartMapX = *MapX;
					StartMapY = *MapY;
					}*/
					XMatrix[0]=-1;
					YMatrix[0]=-1;
					return true;
				}
				else if((ordertype::BUILD==TAdynmem->PrepareOrder_Type)
					&&(GetAsyncKeyState(VirtualKeyCode)&0x8000)>0 && LOWORD(lParam)>127)
				{
					FootPrintX = GetFootX();
					if(FootPrintX == 0)
						FootPrintX = 1;
					FootPrintY = GetFootY();
					if(FootPrintY == 0)
						FootPrintY = 1;

					WriteLine = true;

					DisableTABuildRect();

					StartX = ClickSnapPreviewBuild ? this->ClickSnapPreviewPosXY[0] * 16 : TAdynmem->MouseMapPos.X;
					StartY = ClickSnapPreviewBuild ? this->ClickSnapPreviewPosXY[1] * 16 : TAdynmem->MouseMapPos.Y;
					EndX= TAdynmem->MouseMapPos.X;
					EndY= TAdynmem->MouseMapPos.Y;
					//StartX = (LOWORD(lParam)-128) + TAdynmem->EyeBallMapXPos;
					//StartY = (HIWORD(lParam)) + TAdynmem->EyeBallMapYPos - 32 + TAdynmem->CircleSelect_Pos1TAz/2;
					//EndX = (LOWORD(lParam)-128) + TAdynmem->EyeBallMapXPos;
				//	EndY = (HIWORD(lParam)) + TAdynmem->EyeBallMapYPos - 32 + TAdynmem->CircleSelect_Pos1TAz/2;
					XMatrix[0]=-1;
					YMatrix[0]=-1;

					CalculateLine();
					/*if(ScrollEnabled)
					{
					StartMapX = *MapX;
					StartMapY = *MapY;
					}*/
					return true;
				}
				if ((GetAsyncKeyState(ClickSnapOverrideKey) & 0x8000) == 0) // hold down assigned VK to temporarily disable
				{
					RECT& gameScreenRect = (*TAmainStruct_PtrPtr)->GameSreen_Rect;
					int mouseScreenX = LOWORD(lParam);
					int mouseScreenY = HIWORD(lParam);
					if (mouseScreenX >= gameScreenRect.left && mouseScreenX < gameScreenRect.right)
					{
						if (ClickSnapPreviewBuild)
						{
							ClickBuilding(ClickSnapPreviewPosXY[0] * 16, ClickSnapPreviewPosXY[1] * 16, (GetAsyncKeyState(VK_SHIFT) & 0x8000));
							return true;
						}
						else if (ClickSnapPreviewWreck)
						{
							unsigned idx = ClickSnapPreviewPosXY[0] + ClickSnapPreviewPosXY[1] * TAdynmem->FeatureMapSizeX;
							if (idx >= TAdynmem->FeatureMapSizeX * TAdynmem->FeatureMapSizeY) {
								return false;
							}

							FeatureStruct* f = &TAdynmem->FeatureMap[idx];

							TAdynmem->PrepareOrder_Type = ordertype::RECLAIM;
							TAdynmem->MouseMapPos.X = 8 + int(WreckSnapPreviewMouseMapPosXY[0] * 16.0 + 0.5);
							TAdynmem->MouseMapPos.Y = 8 + int(WreckSnapPreviewMouseMapPosXY[1] * 16.0 + 0.5);
							TAdynmem->MouseMapPos.Z = (f->maxHeight2x2 + f->minHeight2x2) / 2;
							msgstruct mu;
							mu.shiftstatus = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 5 : 0;
							TAMapClick(&mu);

							// fiddle the position of the reclaim order to match the centre of the mouse click capture region
							// (which is offset for underwater reclaim)
							for (int i = 0; i < TAdynmem->PlayerUnitsNumber_Skim; ++i) {
								UnitStruct* u = &TAdynmem->Players[TAdynmem->LocalHumanPlayer_PlayerID].Units[i];
								if (u->UnitID > 0 && u->UnitSelected & 0x10 && u->UnitOrders) {
									UnitOrdersStruct* uo = u->UnitOrders;
									while (uo->NextOrder != NULL) {
										uo = uo->NextOrder;
									}
									unsigned cursorIndex = (*COBSciptHandler_Begin)[uo->COBHandler_index].cursorIndex;
									if (cursorIndex == cursorreclamate &&
										uo->Pos.X == TAdynmem->MouseMapPos.X && uo->Pos.Y == TAdynmem->MouseMapPos.Y && uo->Pos.Z == TAdynmem->MouseMapPos.Z &&
										uo->Pos.Z < TAdynmem->SeaLevel)
									{
										uo->Pos.Z = TAdynmem->SeaLevel;
									}
								}
							}

							return true;
						}
						else if ((ordertype::STOP == TAdynmem->PrepareOrder_Type) && 
							(GetAsyncKeyState(VK_SHIFT) & 0x8000) &&
#ifdef USEMEGAMAP
							!GUIExpander->myMinimap->Controler->IsBliting() &&
#endif
							DraggingUnitOrdersState == DraggingOrderStateEnum::IDLE)
						{
							DraggingUnitOrders = FindUnitOrdersUnderMouse();
							DraggingUnitOrdersState = DraggingOrderStateEnum::PRIMED_TO_DRAG;
							return DraggingUnitOrders != NULL;
						}
					}
				}
				break;

			case WM_LBUTTONUP:
				if (DraggingUnitOrders != NULL && DraggingUnitOrdersState == DraggingOrderStateEnum::PRIMED_TO_DRAG)
				{
					DraggingUnitOrdersState = DraggingOrderStateEnum::CLICK_NOT_DRAG;
					PostMessage(WinProcWnd, WM_LBUTTONDOWN, wParam, lParam);
					PostMessage(WinProcWnd, WM_LBUTTONUP, wParam, lParam);
					return true;
				}
				DraggingUnitOrders = NULL;
				DraggingUnitOrdersState = DraggingOrderStateEnum::IDLE;
				if(RingWrite)
				{
					return true;
				}
				if((GetAsyncKeyState(VirtualKeyCode)&0x8000)==0)
				{
					WriteLine = false;
					EnableTABuildRect();
				}
				break;

			case WM_RBUTTONDOWN:
				if((GetAsyncKeyState(VirtualKeyCode)&0x8000)>0)
				{
					//RingWrite = true;
					return true;
				}
				break;

			case WM_RBUTTONUP:
				RingWrite = false;
				break;

			case WM_MOUSEMOVE:
				//MouseX = LOWORD(lParam);
				//MouseY = HIWORD(lParam);
				ClickSnapPreviewBuild = false;
				ClickSnapPreviewWreck = false;

				if (DraggingUnitOrders &&
					ordertype::STOP == TAdynmem->PrepareOrder_Type &&
					GetAsyncKeyState(VK_SHIFT) & 0x8000 &&
					DraggingUnitOrders->AttackTargat == NULL &&
					IsAnOrder(DraggingUnitOrders->Unit_ptr->UnitOrders, DraggingUnitOrders)
#ifdef USEMEGAMAP
					&& !GUIExpander->myMinimap->Controler->IsBliting()
#endif
					)
				{
					DraggingUnitOrdersState = DraggingOrderStateEnum::DRAG_COMMENCED;
					DragUnitOrders(DraggingUnitOrders);
					return true;
				}
				else if(WriteLine)
				{
					//EndX = LOWORD(lParam);
					//EndY = HIWORD(lParam);
					//if(VisualizeDTRows)

					EndX= TAdynmem->MouseMapPos.X;
					EndY= TAdynmem->MouseMapPos.Y;
					//EndX = (LOWORD(lParam)-128) + TAdynmem->EyeBallMapXPos;
					//EndY = (HIWORD(lParam)) + TAdynmem->EyeBallMapYPos - 32 + TAdynmem->CircleSelect_Pos1TAz/2;
					CalculateLine();
				}
				else if((ordertype::BUILD==TAdynmem->PrepareOrder_Type)
					&&(GetAsyncKeyState(VirtualKeyCode)&0x8000)>0)
				{
					MouseOverUnit = FindMouseUnit ( );
					if(MouseOverUnit)
					{
						CalculateRing();
						RingWrite = true;
					}
					else
					{
						RingWrite = false;
						if(!WriteLine)
							EnableTABuildRect();
					}
				}
				else
				{
					RingWrite = false;
					DraggingUnitOrders = NULL;
					DraggingUnitOrdersState = DraggingOrderStateEnum::IDLE;
					DraggingUnitOrdersBuildRectangleColor = -1;
					ClickSnapPreviewBuild = false;

					if ((GetAsyncKeyState(ClickSnapOverrideKey) & 0x8000) == 0) // hold down assigned VK to temporarily disable
					{
						RECT& gameScreenRect = (*TAmainStruct_PtrPtr)->GameSreen_Rect;
						int mouseScreenX = LOWORD(lParam);
						int mouseScreenY = HIWORD(lParam);
						if (mouseScreenX >= gameScreenRect.left && mouseScreenX < gameScreenRect.right)
						{
							if (MexSnapRadius > 0 && (ordertype::BUILD == TAdynmem->PrepareOrder_Type))
							{
								if (GetBuildUnit().extractsmetal) {
									ClickSnapPreviewPosXY[0] = TAdynmem->BuildPosX;
									ClickSnapPreviewPosXY[1] = TAdynmem->BuildPosY;
									ClickSnapPreviewFootXY[0] = GetFootX();
									ClickSnapPreviewFootXY[1] = GetFootY();
									SnapToNear<CountFeetExceedingSurfaceMetalAdapter>(ClickSnapPreviewPosXY, GetFootX(), GetFootY(), MexSnapRadius,
										TAdynmem->MouseMapPos.X, TAdynmem->MouseMapPos.Y);
									if (ClickSnapPreviewPosXY[0] != TAdynmem->BuildPosX || ClickSnapPreviewPosXY[1] != TAdynmem->BuildPosY) {
										int testFurtherSnapPosXY[2] = { ClickSnapPreviewPosXY[0], ClickSnapPreviewPosXY[1] };
										SnapToNear<CountFeetExceedingSurfaceMetalAdapter>(testFurtherSnapPosXY, GetFootX(), GetFootY(), std::max(GetFootX(), GetFootY()),
											TAdynmem->MouseMapPos.X, TAdynmem->MouseMapPos.Y);
										if (testFurtherSnapPosXY[0] == ClickSnapPreviewPosXY[0] && testFurtherSnapPosXY[1] == ClickSnapPreviewPosXY[1]) {
											// proceed with snap only if the first snap was centred (didn't require further snapping)
											ClickSnapPreviewBuild = true;
										}
									}
								}

								char yardMap[65] = { 0 };
								if (GetBuildUnit().YardMap) {
									int N = std::min(64, GetFootX() * GetFootY());
									std::memcpy(yardMap, GetBuildUnit().YardMap, N);
									yardMap[N] = 0;
								}
								// 0x8f in yardmap, diagnostic of geothermals (I hope)
								if (strchr(yardMap, 0x8f))
								{
									ClickSnapPreviewPosXY[0] = TAdynmem->BuildPosX;
									ClickSnapPreviewPosXY[1] = TAdynmem->BuildPosY;
									ClickSnapPreviewFootXY[0] = GetFootX();
									ClickSnapPreviewFootXY[1] = GetFootY();
									SnapToNear<TestCanBuildAdapter>(ClickSnapPreviewPosXY, GetFootX(), GetFootY(), MexSnapRadius,
										TAdynmem->MouseMapPos.X, TAdynmem->MouseMapPos.Y);
									ClickSnapPreviewBuild = ClickSnapPreviewPosXY[0] != TAdynmem->BuildPosX || ClickSnapPreviewPosXY[1] != TAdynmem->BuildPosY;
								}
							}
							else if (WreckSnapRadius > 0 && ordertype::RECLAIM == TAdynmem->PrepareOrder_Type)
							{
								unsigned idx = TAdynmem->BuildPosX + TAdynmem->BuildPosY * TAdynmem->FeatureMapSizeX;
								if (idx < TAdynmem->FeatureMapSizeX * TAdynmem->FeatureMapSizeY) {
									FeatureStruct* f = &TAdynmem->FeatureMap[idx];
									if (f->occupyingUnitNumber > 0) {
										return false;
									}
								}

								ClickSnapPreviewPosXY[0] = TAdynmem->BuildPosX;
								ClickSnapPreviewPosXY[1] = TAdynmem->BuildPosY;
								ClickSnapPreviewFootXY[0] = 1;
								ClickSnapPreviewFootXY[1] = 1;
								ClickSnapPreviewWreck = 0 < SnapToNear<CountReclaimableAdapter>(ClickSnapPreviewPosXY, 1, 1, WreckSnapRadius,
									TAdynmem->MouseMapPos.X, TAdynmem->MouseMapPos.Y);
								if (ClickSnapPreviewWreck) {
									FindFeatureCentre(ClickSnapPreviewPosXY, WreckSnapPreviewMouseMapPosXY);
								}
							}
						}
					}
					if (ClickSnapPreviewBuild) {
						DisableTABuildRect();
					}
					else {
						EnableTABuildRect();
					}
				}
				break;
			case WM_MOUSEWHEEL:
				/*FootPrint += ((short)HIWORD(wParam))/120;
				if(FootPrint>8)
				FootPrint = 8;
				if(FootPrint<2)
				FootPrint = 2;
				OutputFootPrint();
				return true;*/

				if(HIWORD(wParam)>120)
				{
					if((GetAsyncKeyState(VirtualKeyCode)&0x8000)>0)
					{
						Spacing--;
						if(Spacing<0)
							Spacing = 0;
						UpdateSpacing();
						return true;
					}
				}
				else if(HIWORD(wParam)<=120)
				{
					if((GetAsyncKeyState(VirtualKeyCode)&0x8000)>0)
					{
						Spacing++;
						if(Spacing>MAX_SPACING)
							Spacing = MAX_SPACING;
						UpdateSpacing();
						return true;
					}
				}   
				break;
			}
		}


	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		;// return LocalShare->TAWndProc(WinProcWnd, Msg, wParam, lParam);
	}
	return false;
}

void CTAHook::Set(int KeyCodei, const char *ChatMacroi, bool OptimizeRowsi, bool FullRingsi, int iDelay, int iMexSnapRadius, int iWreckSnapRadius, int iClickSnapOverrideKey)
{
	VirtualKeyCode = KeyCodei;
	lstrcpyA(ShareText, ChatMacroi);
	OptimizeRows = OptimizeRowsi;
	FullRingsEnabled = FullRingsi;
	Delay = iDelay;

	if (iMexSnapRadius < 0) {
		MexSnapRadius = CTAHook::GetDefaultMexSnapRadius();
	}
	else if (iMexSnapRadius <= CTAHook::GetMaxMexSnapRadius()) {
		MexSnapRadius = iMexSnapRadius;
	}
	else {
		MexSnapRadius = CTAHook::GetDefaultMexSnapRadius();
	}

	if (iWreckSnapRadius < 0) {
		WreckSnapRadius = CTAHook::GetDefaultWreckSnapRadius();
	}
	else if (iWreckSnapRadius <= CTAHook::GetMaxWreckSnapRadius()) {
		WreckSnapRadius = iWreckSnapRadius;
	}
	else {
		iWreckSnapRadius = CTAHook::GetDefaultWreckSnapRadius();
	}
	ClickSnapOverrideKey = iClickSnapOverrideKey;
}

void CTAHook::WriteShareMacro()
{
    if (ShareText != NULL) {
        std::stringstream ss(ShareText);
        std::string line;
        while (std::getline(ss, line, '\r')) {
            ChatText(line.c_str());
        }
    }
}

void CTAHook::Blit(LPDIRECTDRAWSURFACE DestSurf)
{
	VisualizeWreckSnapPreview(DestSurf);

	/*if(QueueStatus == ScrolledDTLine)
	{
	int slask = Delay/5;
	if(slask<1)
	slask = 1;
	if(QueuePos>0 && (SendMessage%slask)==0)
	SendQueued();
	}
	else if(QueuePos>0 && (SendMessage%Delay)==0)
	SendQueued();*/

	/*  if(lpRectSurf->IsLost() != DD_OK)
	{
	lpRectSurf->Restore();
	RestoreFromPCX(50, lpRectSurf);
	}

	SendMessage++;

	if(WriteLine && ScrollEnabled)
	{
	bool Calc = false;
	if(StartMapX!=*MapX || StartMapY!=*MapY)
	Calc = true;
	StartX -= *MapX - StartMapX;
	StartY -= *MapY - StartMapY;
	StartMapX = *MapX;
	StartMapY = *MapY;
	if(Calc)
	CalculateLine();
	}

	if(VisualizeDTRows && WriteLine)
	VisualizeRow(DestSurf);

	if(VisualizeDTRows && RingWrite)
	VisualizeRing(DestSurf);*/
}

BOOL CTAHook::IsLineBuilding (void)
{
	return WriteLine || RingWrite;
}

void CTAHook::TABlit()
{
	if(IsLineBuilding ( ))
	{
		PaintMinimapRect();
#ifdef USEMEGAMAP
		if (GUIExpander
			&&GUIExpander->myMinimap
			&&GUIExpander->myMinimap->Controler
			&&( GUIExpander->myMinimap->Controler->IsBliting ( )))
		{
			EnableTABuildRect();
		}
		else
#endif
		{
			EnableTABuildRect();
			VisualizeRow();
			DisableTABuildRect();
		}
	}

	VisualizeDraggingBuildRectangle();
	VisualizeMexSnapPreview();
}
/*

void CTAHook::QueueMessage(UINT M, WPARAM W, LPARAM L)
{
	if(QueuePos==999)
		return;
	MessageQueue[QueuePos].Message = M;
	MessageQueue[QueuePos].wParam = W;
	MessageQueue[QueuePos].lParam = L;
	QueuePos++;
	QueueLength++;
}

void CTAHook::SendQueued()
{
	int i;

	switch(QueueStatus)
	{
	case ShareMacro:
		//post 20 messages each frame
		for(i=0; i<20; i++)
		{
			if(QueuePos==0)
				return;
			if(MessageQueue[QueueLength-QueuePos].Message==WM_CHAR && MessageQueue[QueueLength-QueuePos].wParam==13 && i!=0)
				return;
			LocalShare->TAWndProc(TAhWnd, MessageQueue[QueueLength-QueuePos].Message, MessageQueue[QueueLength-QueuePos].wParam, MessageQueue[QueueLength-QueuePos].lParam);
			QueuePos--;
		}
		break;

	case DTLine:
	case DTRing:
		for(i=0; i<5; i++)
		{
			if(QueuePos==0)
				return;
			LocalShare->TAWndProc(TAhWnd, MessageQueue[QueueLength-QueuePos].Message, MessageQueue[QueueLength-QueuePos].wParam, MessageQueue[QueueLength-QueuePos].lParam);

			QueuePos--;
		}
		break;

	case ScrolledDTLine:
		if(MessageQueue[QueueLength-QueuePos].Message == SCROLL)
			((AlliesWhiteboard*)LocalShare->Whiteboard)->InstantScrollTo(MessageQueue[QueueLength-QueuePos].wParam, MessageQueue[QueueLength-QueuePos].lParam);
		else
			LocalShare->TAWndProc(TAhWnd, MessageQueue[QueueLength-QueuePos].Message, MessageQueue[QueueLength-QueuePos].wParam, MessageQueue[QueueLength-QueuePos].lParam);
		QueuePos--;
		break;
	}

}
*/

void CTAHook::WriteDTLine()
{

	if(OptimizeRows && FootPrintX==2 && FootPrintY==2)
		OptimizeDTRows();

	int i=0;
	while(XMatrix[i] != -1 && YMatrix[i]!=-1)
	{
		ClickBuilding(XMatrix[i], YMatrix[i]);

		i++;	
	}
}

void CTAHook::CalculateLine()
{
	int dx = (EndX - StartX)/16;
	int dy = (EndY - StartY)/16;
	int x_inc = 16;
	int y_inc = 16;

	int error,index;

	int x = StartX;
	int y = StartY;

	int footprintx = Spacing + FootPrintX;
	int footprinty = Spacing + FootPrintY;

	if(dx<0)
	{
		x_inc = -x_inc;
		dx = -dx;
	}

	if(dy<0)
	{
		y_inc = -y_inc;
		dy = -dy;
	}

	if(dx > dy)
	{
		dx = dx/footprintx;
		x_inc *= footprintx;

		if(dx>999)
			return; //argh..

		YMatrix[dx+1]=-1;
		XMatrix[dx+1]=-1;

		MatrixLength = dx;
		Direction = 1;

		error = (dy + dy) - dx;

		for(index=0; index <= dx; index++)
		{
			XMatrix[index]=(short)x;
			YMatrix[index]=(short)y;

			while((error >= 0) && (dx != 0))
			{
				error -= (dx + dx);
				y += y_inc;
			}

			error += (dy + dy);
			x += x_inc;

		} // end for

	} // end if dx > dy
	else
	{
		dy = dy/footprinty;
		y_inc *= footprinty;

		if(dy>999)
			return;

		YMatrix[dy+1]=-1;
		XMatrix[dy+1]=-1;

		MatrixLength = dy;
		Direction = 2;

		error = (dx + dx) - dy;

		for(index=0; index <= dy; index++)
		{
			XMatrix[index]=(short)x;
			YMatrix[index]=(short)y;

			while((error >= 0) && (dy != 0))
			{
				error -= (dy + dy);
				x += x_inc;
			}

			error += (dx + dx);
			y += y_inc;
		} // end for
	} // end else if dx > dy
}

void CTAHook::UpdateSpacing()
{
	/*char CFootPrint[20];
	wsprintf(CFootPrint, "%i", Spacing);

	char OutString[80];
	lstrcpyA(OutString, "Spacing set to ");
	lstrcatA(OutString, CFootPrint);

	SendText(OutString, 0);*/

	if(WriteLine)
		CalculateLine();

	if(RingWrite)
		CalculateRing();
}

void CTAHook::OptimizeDTRows()
{
	short tx,ty;

	switch(Direction)
	{
	case 1:
		if(MatrixLength>2)
		{
			for(int index=1; index <= MatrixLength-2; index++)
			{
				if(YMatrix[index]==YMatrix[index+2])
				{
					tx = XMatrix[index+2];
					ty = YMatrix[index+2];
					XMatrix[index+2] = XMatrix[index];
					YMatrix[index+2] = YMatrix[index];
					XMatrix[index] = tx;
					YMatrix[index] = ty;
					index+=2;
				}
				else if(YMatrix[index]==YMatrix[index+1])
				{
					tx = XMatrix[index+1];
					ty = YMatrix[index+1];
					XMatrix[index+1] = XMatrix[index];
					YMatrix[index+1] = YMatrix[index];
					XMatrix[index] = tx;
					YMatrix[index] = ty;
					index++;
				}
			}//end for
		}//end if
		break;

	case 2:
		if(MatrixLength>2)
		{
			for(int index=1; index <= MatrixLength-2; index++)
			{
				if(XMatrix[index]==XMatrix[index+2])
				{
					tx = XMatrix[index+2];
					ty = YMatrix[index+2];
					XMatrix[index+2] = XMatrix[index];
					YMatrix[index+2] = YMatrix[index];
					XMatrix[index] = tx;
					YMatrix[index] = ty;
					index+=2;
				}
				else if(XMatrix[index]==XMatrix[index+1])
				{
					tx = XMatrix[index+1];
					ty = YMatrix[index+1];
					XMatrix[index+1] = XMatrix[index];
					YMatrix[index+1] = YMatrix[index];
					XMatrix[index] = tx;
					YMatrix[index] = ty;
					index+=1;
				}
			}//end for
		}//end if
		break;
	}
}

void CTAHook::VisualizeRow()
{
	int i=0;

	while(XMatrix[i] != -1 && YMatrix[i]!=-1)
	{
		int BakX= TAdynmem->MouseMapPos.X;
		int BakY= TAdynmem->MouseMapPos.Y;

		TAdynmem->MouseMapPos.X = XMatrix[i];
		TAdynmem->MouseMapPos.Y = YMatrix[i];

		TAdynmem->BuildSpotState=70;

		TestBuildSpot ( );

		int color = TAdynmem->BuildSpotState==70 ? 234 : 214; 

		DrawBuildRect( (TAdynmem->CircleSelect_Pos1TAx - TAdynmem->EyeBallMapXPos) + 128,
			(TAdynmem->CircleSelect_Pos1TAy - TAdynmem->EyeBallMapYPos) + 32 - (TAdynmem->CircleSelect_Pos1TAz/2),
			GetFootX()*16,
			GetFootY()*16,
			color);
		i++;
		TAdynmem->MouseMapPos.X= BakX;
		TAdynmem->MouseMapPos.Y= BakY;
	}

	/*  DDBLTFX ddbltfx;
	DDRAW_INIT_STRUCT(ddbltfx);
	ddbltfx.ddckSrcColorkey.dwColorSpaceLowValue = 102;
	ddbltfx.ddckSrcColorkey.dwColorSpaceHighValue = 102;

	int XCenter;
	int YCenter;
	int iMapX, iMapY;
	if(ScrollEnabled)
	{
	iMapX = ((AlliesWhiteboard*)LocalShare->Whiteboard)->GetMapX();
	iMapY = ((AlliesWhiteboard*)LocalShare->Whiteboard)->GetMapY();
	}
	else
	{
	iMapX = 0;
	iMapY = 0;
	}
	int i=0;
	while(XMatrix[i] != -1 && YMatrix[i]!=-1)
	{
	XCenter = (XMatrix[i])+8-((XMatrix[i])+iMapX+8 + ((FootPrintX%2)*8))%16;
	YCenter = (YMatrix[i])+8-((YMatrix[i])+iMapY+2 + ((FootPrintY%2)*8))%16;
	RECT Dest;
	Dest.left = XCenter - FootPrintX*8;
	Dest.top = YCenter - FootPrintY*8;
	Dest.right = XCenter + FootPrintX*8;
	Dest.bottom = YCenter + FootPrintY*8;
	if(DestSurf->Blt(&Dest, lpRectSurf, NULL, DDBLT_ASYNC | DDBLT_KEYSRCOVERRIDE, &ddbltfx)!=DD_OK)
	{
	DestSurf->Blt(&Dest, lpRectSurf, NULL, DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &ddbltfx);
	}

	i++;
	}*/

}
#ifdef USEMEGAMAP

void CTAHook::VisualizeRow_ForME_megamap (OFFSCREEN * argc)
{
	//EnableTABuildRect ( );
	int i=0;

	while(XMatrix[i] != -1 && YMatrix[i]!=-1)
	{
		int BakX= TAdynmem->MouseMapPos.X;
		int BakY= TAdynmem->MouseMapPos.Y;

		TAdynmem->MouseMapPos.X = XMatrix[i];
		TAdynmem->MouseMapPos.Y = YMatrix[i];

		TAdynmem->BuildSpotState=70;

		TestBuildSpot ( );

		int color = TAdynmem->BuildSpotState==70 ? 234 : 214; 

		GUIExpander->myMinimap->TAStuff->TADrawRect ( (OFFSCREEN *)argc, TAdynmem->CircleSelect_Pos1TAx, TAdynmem->CircleSelect_Pos1TAy, TAdynmem->CircleSelect_Pos1TAz, 
			TAdynmem->CircleSelect_Pos2TAx, TAdynmem->CircleSelect_Pos2TAy, TAdynmem->CircleSelect_Pos2TAz, color);
		i++;

		TAdynmem->MouseMapPos.X= BakX;
		TAdynmem->MouseMapPos.Y= BakY;
	}

	//DisableTABuildRect ( );
}
#endif

void CTAHook::WriteScrollDTLine()
{
	/*  if(QueuePos>0)
	return;

	QueueLength = 0;
	QueuePos = 0;


	CalculateLine();
	if(OptimizeRows && FootPrintX==2 && FootPrintY==2)
	OptimizeDTRows();

	if(XMatrix[0]>128 && XMatrix[0]<LocalShare->ScreenWidth && YMatrix[0]>32 && YMatrix[0]<LocalShare->ScreenHeight-32
	&& XMatrix[MatrixLength]>128 && XMatrix[MatrixLength]<LocalShare->ScreenWidth && YMatrix[MatrixLength]>32 && YMatrix[MatrixLength]<LocalShare->ScreenHeight-32)
	{
	//all DTs inside screen standar clicking
	QueueStatus = DTLine;
	//delete the clicked DT
	long Pos;
	Pos = EndY << 16;
	Pos += EndX;
	QueueMessage(WM_LBUTTONDOWN, MK_LBUTTON | MK_SHIFT, Pos);
	QueueMessage(WM_LBUTTONUP, MK_LBUTTON | MK_SHIFT, Pos);
	Pos = StartY << 16;
	Pos += StartX;
	QueueMessage(WM_LBUTTONDOWN, MK_LBUTTON | MK_SHIFT, Pos);
	QueueMessage(WM_LBUTTONUP, MK_LBUTTON | MK_SHIFT, Pos);
	int i=0;
	while(XMatrix[i] != -1 && YMatrix[i]!=-1)
	{
	short x = XMatrix[i];
	short y = YMatrix[i];
	long Pos;
	Pos = y << 16;
	Pos += x;
	QueueMessage(WM_LBUTTONDOWN, MK_LBUTTON | MK_SHIFT, Pos);
	QueueMessage(WM_LBUTTONUP, MK_LBUTTON | MK_SHIFT, Pos);
	i++;
	}
	}
	else
	{
	QueueStatus = ScrolledDTLine;
	WriteLine = false;
	int OldMapX = *MapX;
	int OldMapY = *MapY;

	int ScrolledToX = *MapX;
	int ScrolledToY = *MapY;
	int i=0;
	int Foot8 = FootPrintX*8;

	long Pos;

	ScrolledToX = (StartMapX + StartX)-(LocalShare->ScreenWidth-128)/2;
	if(ScrolledToX<0)
	ScrolledToX = 0;
	if(ScrolledToX>GetMaxScrollX())
	ScrolledToX = GetMaxScrollX();
	ScrolledToY = (StartMapY + StartY)-(LocalShare->ScreenHeight-128)/2;
	if(ScrolledToY<0)
	ScrolledToY = 0;
	if(ScrolledToY>GetMaxScrollY())
	ScrolledToY = GetMaxScrollY();
	QueueMessage(SCROLL, ScrolledToX, ScrolledToY);
	Pos = ((StartMapY + StartY) - ScrolledToY) << 16;
	Pos += (StartMapX + StartX) - ScrolledToX;
	QueueMessage(WM_LBUTTONDOWN, MK_LBUTTON | MK_SHIFT, Pos);
	QueueMessage(WM_LBUTTONUP, MK_LBUTTON | MK_SHIFT, Pos);

	ScrolledToX = (StartMapX + EndX)-(LocalShare->ScreenWidth-128)/2;
	if(ScrolledToX<0)
	ScrolledToX = 0;
	if(ScrolledToX>GetMaxScrollX())
	ScrolledToX = GetMaxScrollX();
	ScrolledToY = (StartMapY + EndY)-(LocalShare->ScreenHeight-128)/2;
	if(ScrolledToY<0)
	ScrolledToY = 0;
	if(ScrolledToY>GetMaxScrollY())
	ScrolledToY = GetMaxScrollY();
	QueueMessage(SCROLL, ScrolledToX, ScrolledToY);
	Pos = ((StartMapY + EndY) - ScrolledToY) << 16;
	Pos += (StartMapX + EndX) - ScrolledToX;
	QueueMessage(WM_LBUTTONDOWN, MK_LBUTTON | MK_SHIFT, Pos);
	QueueMessage(WM_LBUTTONUP, MK_LBUTTON | MK_SHIFT, Pos);


	while(XMatrix[i] != -1 && YMatrix[i]!=-1)
	{
	int RealX = StartMapX + XMatrix[i];
	int RealY = StartMapY + YMatrix[i];

	//scroll so DT is in middle of screen
	ScrolledToX = RealX-(LocalShare->ScreenWidth-128)/2;
	if(ScrolledToX<0)
	ScrolledToX = 0;
	if(ScrolledToX>GetMaxScrollX())
	ScrolledToX = GetMaxScrollX();
	ScrolledToY = RealY-(LocalShare->ScreenHeight-32)/2;
	if(ScrolledToY<0)
	ScrolledToY = 0;
	if(ScrolledToY>GetMaxScrollY())
	ScrolledToY = GetMaxScrollY();
	QueueMessage(SCROLL, ScrolledToX, ScrolledToY);

	short x = RealX - ScrolledToX;
	short y = RealY - ScrolledToY;
	long Pos;
	Pos = y << 16;
	Pos += x;
	QueueMessage(WM_LBUTTONDOWN, MK_LBUTTON | MK_SHIFT, Pos);
	QueueMessage(WM_LBUTTONUP, MK_LBUTTON | MK_SHIFT, Pos);

	i++;
	}

	QueueMessage(SCROLL, OldMapX, OldMapY);
	}

	QueueLength = QueuePos;
	*/
}



void CTAHook::CalculateRing()
{
	//char *unittested = new char [0xffff];
	//memset(unittested, 0, 0xffff);

	int footx = TAdynmem->BeginUnitsArray_p [MouseOverUnit].UnitType->FootX + Spacing*2;
	int footy = TAdynmem->BeginUnitsArray_p [MouseOverUnit].UnitType->FootY + Spacing*2;
	int posx = TAdynmem->BeginUnitsArray_p [MouseOverUnit].XGridPos*16 - Spacing*16;
	int posy =  TAdynmem->BeginUnitsArray_p [MouseOverUnit].YGridPos*16 - Spacing*16;

	//int x1 = posx;
	//int y1 = posy;
	//int x2 = posx + footx*16;
	//int y2 = posy + footy*16;

	//unittested[MouseOverUnit-1] = 1;

	//FindConnectedSquare(x1, y1, x2, y2, unittested);

	CalculateRing(posx, posy, footx, footy);
}

/*
void CTAHook::FindConnectedSquare(int &x1, int &y1, int &x2, int &y2, char *unittested)
{
	CalculateRing(x1, y1, (x2-x1)/16, (y2-y1)/16);

	int i=0;
	while(XMatrix[i]!=-1 && YMatrix[i]!=-1)
	{
		TAdynmem->MouseMapPos.X = XMatrix[i];
		TAdynmem->MouseMapPos.Y = YMatrix[i];

		int building = FindMouseUnit();
		if(unittested[building-1]==0)
		{
			int newx1 = TAdynmem->BeginUnitsArray_p [building].XGridPos*16;
			int newy1 = TAdynmem->BeginUnitsArray_p [building].YGridPos*16;
			int newx2 = newx1 + TAdynmem->BeginUnitsArray_p [building].UnitType->FootX*16;
			int newy2 = newy1 + TAdynmem->BeginUnitsArray_p [building].UnitType->FootY*16;

			if(newx1<x1)
				x1 = newx1;
			if(newy1<y1)
				y1 = newy1;
			if(newx2>x2)
				x2 = newx2;
			if(newy2>y2)
				y2 = newy2;

			unittested[building-1] = 1;

			FindConnectedSquare(x1, y1, x2, y2, unittested);
		}
		i++;
	}
}
*/

void CTAHook::CalculateRing(int posx, int posy, int footx, int footy)
{
	int pos = 0;

	int add = 0;
	add = (footx%GetFootX()&&FullRingsEnabled&&GetFootX()<3&&GetFootY()<3) ? 1 : 0;
	int linelengthx = (footx/GetFootX()) + add + 1;
	add = (footy%GetFootY()&&FullRingsEnabled&&GetFootX()<3&&GetFootY()<3) ? 1 : 0;
	int linelengthy = (footy/GetFootY()) + add + 1;

	for(int i=0; i<linelengthx; i++)
	{
		XMatrix[pos] = posx + (GetFootX()*16)/2 + i*GetFootX()*16;
		YMatrix[pos] = posy - (GetFootY()*16)/2;
		pos++;
	}
	for(int i=0; i<linelengthy; i++)
	{
		XMatrix[pos] = posx + footx*16 + (GetFootX()*16)/2;
		YMatrix[pos] = posy + (GetFootY()*16)/2 + i*GetFootY()*16;
		pos++;
	}
	for(int i=0; i<linelengthx; i++)
	{
		XMatrix[pos] = posx + footx*16 - (GetFootX()*16)/2 - GetFootX()*16*i;
		YMatrix[pos] = posy + footy*16 + (GetFootY()*16)/2;
		pos++;
	}
	for(int i=0; i<linelengthy; i++)
	{
		XMatrix[pos] = posx - (GetFootX()*16)/2;
		YMatrix[pos] = posy + footy*16 - (GetFootY()*16)/2 - GetFootY()*16*i;
		pos++;
	}

	XMatrix[pos] = -1;
	YMatrix[pos] = -1;
}
/*

void CTAHook::VisualizeRing(LPDIRECTDRAWSURFACE DestSurf)
{
	DDBLTFX ddbltfx;
	DDRAW_INIT_STRUCT(ddbltfx);
	ddbltfx.ddckSrcColorkey.dwColorSpaceLowValue = 102;
	ddbltfx.ddckSrcColorkey.dwColorSpaceHighValue = 102;

	int iMapX, iMapY;
	if(ScrollEnabled)
	{
		iMapX = ((AlliesWhiteboard*)LocalShare->Whiteboard)->GetMapX();
		iMapY = ((AlliesWhiteboard*)LocalShare->Whiteboard)->GetMapY();
	}

	for(int i=0; i<Length[FootPrintX-2]; i++)
	{
		int XCenter = ((EndX) + Rings[FootPrintX-2][i].x)+8-(((EndX) + Rings[FootPrintX-2][i].x)+iMapX+8)%16;
		int YCenter = ((EndY) + Rings[FootPrintY-2][i].y)+8-(((EndY) + Rings[FootPrintY-2][i].y)+iMapY+2)%16;
		RECT Dest;
		Dest.left = XCenter - 16;
		Dest.top = YCenter - 16;
		Dest.right = XCenter + 16;
		Dest.bottom = YCenter + 16;
		if(DestSurf->Blt(&Dest, lpRectSurf, NULL, DDBLT_ASYNC | DDBLT_KEYSRCOVERRIDE, &ddbltfx)!=DD_OK)
		{
			DestSurf->Blt(&Dest, lpRectSurf, NULL, DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &ddbltfx);
		}
	}
}
*/

void CTAHook::ClickBuilding(int Xpos, int Ypos, bool shiftBuild)
{
	int BakX= TAdynmem->MouseMapPos.X;
	int BakY= TAdynmem->MouseMapPos.Y;

	msgstruct mu;
	mu.shiftstatus = shiftBuild ? 5 : 0;

	TAdynmem->MouseMapPos.X = (short)Xpos;
	TAdynmem->MouseMapPos.Y = (short)Ypos;

	TestBuildSpot();

	TAMapClick(&mu);

	TAdynmem->MouseMapPos.X= BakX;
	TAdynmem->MouseMapPos.Y= BakY;
}

short CTAHook::GetFootX()  //get footprint of selected BeginUnitsArray_p  to build
{
	return TAdynmem->UnitDef[TAdynmem->BuildUnitID].FootX;
}

short CTAHook::GetFootY()  //get footprint of selected BeginUnitsArray_p  to build
{
	return TAdynmem->UnitDef[TAdynmem->BuildUnitID].FootY;
}

const UnitDefStruct& CTAHook::GetBuildUnit() const
{
	return TAdynmem->UnitDef[TAdynmem->BuildUnitID];
}

void CTAHook::DrawBuildRect(int posx, int posy, int sizex, int sizey, int color)
{
	tagRECT rect = {posx, posy, posx+sizex, posy+sizey};

	if(rect.top<32)
		rect.top = 32;
	if(rect.top>LocalShare->ScreenHeight-33)
		rect.top = LocalShare->ScreenHeight-33;
	if(rect.bottom<32)
		rect.bottom = 32;
	if(rect.bottom>LocalShare->ScreenHeight-33)
		rect.bottom = LocalShare->ScreenHeight-33;

	if(rect.left<128)
		rect.left = 128;
	if(rect.left>LocalShare->ScreenWidth)
		rect.left = LocalShare->ScreenWidth;
	if(rect.right<128)
		rect.right = 128;
	if(rect.right>LocalShare->ScreenWidth)
		rect.right = LocalShare->ScreenWidth;

	TADrawRect(NULL, &rect, color);

	rect.top++;
	rect.left++;
	rect.bottom++;
	rect.right++;

	if(rect.top<32)
		rect.top = 32;
	if(rect.top>LocalShare->ScreenHeight-33)
		rect.top = LocalShare->ScreenHeight-33;
	if(rect.bottom<32)
		rect.bottom = 32;
	if(rect.bottom>LocalShare->ScreenHeight-33)
		rect.bottom = LocalShare->ScreenHeight-33;

	if(rect.left<128)
		rect.left = 128;
	if(rect.left>LocalShare->ScreenWidth)
		rect.left = LocalShare->ScreenWidth;
	if(rect.right<128)
		rect.right = 128;
	if(rect.right>LocalShare->ScreenWidth)
		rect.right = LocalShare->ScreenWidth;

	TADrawRect(NULL, &rect, color);
}

void CTAHook::EnableTABuildRect()
{
    // Patch DrawGameScreen
    std::uint8_t ops = 0x85;    // test eax, eax ; a genuine test whether or not to DrawTranspRectangle
    WriteProcessMemory(GetCurrentProcess(), (void*)0x469e0b, &ops, 1, NULL);
}

void CTAHook::DisableTABuildRect()
{
    // Patch DrawGameScreen
    std::uint8_t ops = 0x31;    // xor eax, eax ; never DrawTranspRectangle
    WriteProcessMemory(GetCurrentProcess(), (void*)0x469e0b, &ops, 1, NULL);
}

void CTAHook::PaintMinimapRect()
{
	TADrawRect ( NULL, &TAdynmem->MinimapRect, 0xC2);	
}

UnitOrdersStruct* CTAHook::FindUnitOrdersUnderMouse()
{
	PlayerStruct* me = &TAdynmem->Players[TAdynmem->LocalHumanPlayer_PlayerID];
	for (UnitStruct* unit = me->Units; unit != me->UnitsAry_End; ++unit)
	{
		UnitOrdersStruct* unitOrders = unit->UnitOrders;
		while (unit->UnitSelected & 0x10 && unitOrders != NULL) {
			unsigned OrderMask = (*COBSciptHandler_Begin)[unitOrders->COBHandler_index].COBScripMask;
			unsigned cursorIndex = (*COBSciptHandler_Begin)[unitOrders->COBHandler_index].cursorIndex;
			const char* cobTechnicalName = (*COBSciptHandler_Begin)[unitOrders->COBHandler_index].technicalName;
			if (unitOrders->AttackTargat == NULL &&
				(OrderMask & 1 || // build
					cursorIndex == cursorpatrol ||
					cursorIndex == cursorunload ||
					cursorIndex == cursormove
					))
			{
				const int orderX = unitOrders->Pos.X;
				const int orderY = int(unitOrders->Pos.Y) - int(unitOrders->Pos.Z)/2;
				const int footX = unitOrders->BuildUnitID ? TAdynmem->UnitDef[unitOrders->BuildUnitID].FootX : 1;
				const int footY = unitOrders->BuildUnitID ? TAdynmem->UnitDef[unitOrders->BuildUnitID].FootY : 1;
				const int mouseX = TAdynmem->MouseMapPos.X;
				const int mouseY = int(TAdynmem->MouseMapPos.Y) - int(TAdynmem->MouseMapPos.Z)/2;
				if (mouseX >= orderX - 8 * footX && mouseX < orderX + 8 * footX &&
					mouseY >= orderY - 8 * footY && mouseY < orderY + 8 * footY) {
					return unitOrders;
				}
			}
			unitOrders = unitOrders->NextOrder;
		}
	}
	return NULL;
}

bool CTAHook::IsAnOrder(UnitOrdersStruct* unitOrders, UnitOrdersStruct* order)
{
	while (unitOrders != NULL) {
		if (unitOrders == order)
		{
			return true;
		}
		unitOrders = unitOrders->NextOrder;
	}
	return false;
}

void CTAHook::VisualizeDraggingBuildRectangle()
{
	if (DraggingUnitOrders && DraggingUnitOrders->BuildUnitID && DraggingUnitOrdersBuildRectangleColor >= 0)
	{
		const int footX = DraggingUnitOrders->BuildUnitID ? TAdynmem->UnitDef[DraggingUnitOrders->BuildUnitID].FootX : 1;
		const int footY = DraggingUnitOrders->BuildUnitID ? TAdynmem->UnitDef[DraggingUnitOrders->BuildUnitID].FootY : 1;
		DrawBuildRect((TAdynmem->CircleSelect_Pos1TAx - TAdynmem->EyeBallMapXPos) + 128,
			(TAdynmem->CircleSelect_Pos1TAy - TAdynmem->EyeBallMapYPos) + 32 - (TAdynmem->CircleSelect_Pos1TAz / 2),
			GetFootX() * 16,
			GetFootY() * 16,
			DraggingUnitOrdersBuildRectangleColor);
	}
}

void CTAHook::VisualizeMexSnapPreview()
{
	if (ClickSnapPreviewBuild)
	{
		int BakX = TAdynmem->MouseMapPos.X;
		int BakY = TAdynmem->MouseMapPos.Y;
		TAdynmem->MouseMapPos.X = ClickSnapPreviewPosXY[0] * 16;
		TAdynmem->MouseMapPos.Y = ClickSnapPreviewPosXY[1] * 16;
		TAdynmem->BuildSpotState = 70;
		TestBuildSpot();

		int color = TAdynmem->BuildSpotState == 70 ? 234 : 214;
		DrawBuildRect((TAdynmem->CircleSelect_Pos1TAx - TAdynmem->EyeBallMapXPos) + 128,
			(TAdynmem->CircleSelect_Pos1TAy - TAdynmem->EyeBallMapYPos) + 32 - (TAdynmem->CircleSelect_Pos1TAz / 2),
			ClickSnapPreviewFootXY[0] * 16,
			ClickSnapPreviewFootXY[1] * 16,
			color);

		TAdynmem->MouseMapPos.X = BakX;
		TAdynmem->MouseMapPos.Y = BakY;
	}
}

void CTAHook::VisualizeWreckSnapPreview(LPDIRECTDRAWSURFACE DestSurf)
{
	if (!ClickSnapPreviewWreck) {
		return;
	}

	DDSURFACEDESC ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	if (DestSurf->GetSurfaceDesc(&ddsd) != DD_OK) {
		return;
	}

	OFFSCREEN OffScreen;
	memset(&OffScreen, 0, sizeof(OFFSCREEN));
	OffScreen.Height = ddsd.dwHeight;
	OffScreen.Width = ddsd.dwWidth;
	OffScreen.lPitch = ddsd.lPitch;
	OffScreen.lpSurface = ddsd.lpSurface;
	std::memcpy(&OffScreen.ScreenRect, &TAdynmem->GameSreen_Rect, sizeof(OffScreen.ScreenRect));

	PGAFSequence GafSeq_p = TAdynmem->cursor_ary[cursorreclamate];
	int annimationIndex = (TAdynmem->GameTime / GafSeq_p->PtrFrameAry->Animated) % GafSeq_p->Frames;
	PGAFFrame Gaf_p = GafSeq_p->PtrFrameAry[annimationIndex].PtrFrame;
	if (Gaf_p == NULL) {
		return;
	}

	unsigned idx = ClickSnapPreviewPosXY[0] + ClickSnapPreviewPosXY[1] * TAdynmem->FeatureMapSizeX;
	if (idx >= TAdynmem->FeatureMapSizeX * TAdynmem->FeatureMapSizeY) {
		return;
	}

	FeatureStruct* f = &TAdynmem->FeatureMap[idx];
	int z = (f->maxHeight2x2 + f->minHeight2x2) / 2;
	int x = 8 + int(WreckSnapPreviewMouseMapPosXY[0] * 16.0 + 0.5) - TAdynmem->EyeBallMapXPos + 128;
	int y = 8 + int(WreckSnapPreviewMouseMapPosXY[1] * 16.0 + 0.5) - TAdynmem->EyeBallMapYPos + 32 - z / 2;
	if (z < TAdynmem->SeaLevel) {
	 //show position of the click rather than the reclaim position
		y -= (TAdynmem->SeaLevel - z) / 2;
	}

	CopyGafToContext(&OffScreen, Gaf_p, x, y);
}

void CTAHook::DragUnitOrders(UnitOrdersStruct* order)
{
	int state = order->State;
	UnitStruct* unit = order->Unit_ptr;

	int XPosBack = order->Pos.X;
	int YPosBack = order->Pos.Y;
	int ZPosBack = order->Pos.Z;

	if (unit->UnitOrders == order) {
		order->Pos.X = unit->XPos;
		order->Pos.Y = unit->YPos;
		order->Pos.Z = unit->ZPos;
		Order_Move_Ground(unit, order, 0);
	}

	if (order->BuildUnitID) {
		const int footX = TAdynmem->UnitDef[order->BuildUnitID].FootX;
		const int footY = TAdynmem->UnitDef[order->BuildUnitID].FootY;
		int idx = TAdynmem->BuildPosX + TAdynmem->BuildPosY * TAdynmem->FeatureMapSizeX;
		TAdynmem->BuildUnitID = order->BuildUnitID;
		TAdynmem->MouseMapPos.X = 16 * TAdynmem->BuildPosX + (footX % 2 ? 8 : 0);
		TAdynmem->MouseMapPos.Y = 16 * TAdynmem->BuildPosY + (footY % 2 ? 8 : 0);
		TAdynmem->MouseMapPos.Z = TAdynmem->FeatureMap[idx].height;
		TAdynmem->BuildSpotState = 70;
		TestBuildSpot();
		if (TAdynmem->BuildSpotState == 70) {
			order->State = 0;
			order->Pos.X = TAdynmem->MouseMapPos.X;
			order->Pos.Y = TAdynmem->MouseMapPos.Y;
			order->Pos.Z = TAdynmem->MouseMapPos.Z;
			DraggingUnitOrdersBuildRectangleColor = -1;
		}
		else {
			DraggingUnitOrdersBuildRectangleColor = 214;
			order->Pos.X = XPosBack;
			order->Pos.Y = YPosBack;
			order->Pos.Z = ZPosBack;
		}
	}
	else {
		order->State = 0;
		order->Pos.X = TAdynmem->MouseMapPos.X;
		order->Pos.Y = TAdynmem->MouseMapPos.Y;
		order->Pos.Z = TAdynmem->MouseMapPos.Z;
	}
}

int CTAHook::GetDefaultMexSnapRadius()
{
	// dead space at the end of "Copyright 0000 Humongous Entertainment"
	// Is 0 unless exe is patched to make it something else
	int radius = *TA_HOOK_DEFAULT_MEX_SNAP_RADIUS;
	if (radius > 9) { 
		radius = 9;
	}

	int maxRadius = GetMaxMexSnapRadius();
	if (radius > maxRadius) {
		radius = maxRadius;
	}
	return radius;
}

int CTAHook::GetMaxMexSnapRadius()
{
	// dead space at the end of "Copyright 0000 Humongous Entertainment"
	// Is 0 unless exe is patched to make it something else
	int radius = *TA_HOOK_MAX_MEX_SNAP_RADIUS;
	if (radius > 9) {
		radius = 9;
	}
	return radius;
}

int CTAHook::GetDefaultWreckSnapRadius()
{
	// dead space at the end of "0000"
	// Is 0 unless exe is patched to make it something else
	int radius = *TA_HOOK_DEFAULT_WRECK_SNAP_RADIUS;
	if (radius > 9) {
		radius = 9;
	}

	int maxRadius = GetMaxWreckSnapRadius();
	if (radius > maxRadius) {
		radius = maxRadius;
	}
	return radius;
}

int CTAHook::GetMaxWreckSnapRadius()
{
	// dead space at the end of "0000"
	// Is 0 unless exe is patched to make it something else
	int radius = *TA_HOOK_MAX_WRECK_SNAP_RADIUS;
	if (radius > 9) {
		radius = 9;
	}
	return radius;
}

bool CTAHook::IsWreckSnapping()
{
	return ClickSnapPreviewWreck;
}

bool CTAHook::IsMexSnapping()
{
	return ClickSnapPreviewBuild;
}
