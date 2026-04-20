#include "config.h"
#include "Dialog.h"

#include "cincome.h"
#include "fullscreenminimap.h"
#include "gaf.h"
#include "GUIExpand.h"
#include "hook\hook.h"
#include "iddrawsurface.h"
#include "MegamapControl.h"
#include "whiteboard.h"
#include "pcxread.h"
#include "tahook.h"
#include "tafunctions.h"
#include "Widgets/Button.h"
#include "Widgets/IntegerField.h"
#include "Widgets/Label.h"
#include "Widgets/TextField.h"
#include "Widgets/VirtualKeyField.h"
#include "Widgets/Widget.h"

#include <stdio.h>

#define COL0 16
#define COL0b 22
#define COL0c 74
#define COL1 126
#define COL2 200
#define COL3 310
#define COL4 340

Dialog::Dialog(BOOL Vidmem_a)
    : DialogBase(Vidmem_a, DialogWidth, DialogHeight, ROW_HEIGHT)
{
	lpOKButton      = CreateSurfPCXResource(5,  Vidmem_a);
	lpStagedButton3 = CreateSurfPCXResource(6,  Vidmem_a);
	lpCheckBox      = CreateSurfPCXResource(10, Vidmem_a);
	lpStagedButton1 = CreateSurfPCXResource(11, Vidmem_a);
	lpStandardButton= CreateSurfPCXResource(12, Vidmem_a);

	// column 0, 1

#if MAX_MEX_SNAP_RADIUS || MAX_WRECK_SNAP_RADIUS
	m_widgets.push_back(std::make_shared<Label>(COL0, ROW(0), "Snap Override Key"));
	m_widgets.push_back(m_clickSnapOverrideVirtualKeyField = std::make_shared<VirtualKeyField>(COL1, ROW(0), 50, ROW_HEIGHT, VK_MENU, "ClickSnapOverrideKey"));
#endif

#if TA_HOOK_ENABLE
	m_widgets.push_back(std::make_shared<Label>(COL0, ROW(1), "Autoclick Key"));
	m_widgets.push_back(m_autoClickVirtualKeyField = std::make_shared<VirtualKeyField>(COL1, ROW(1), 50, ROW_HEIGHT, 88, "KeyCode"));
#endif

#if USEWHITEBOARD
	m_widgets.push_back(std::make_shared<Label>(COL0, ROW(2), "Whiteboard Key"));
	m_widgets.push_back(m_whiteboardVirtualKeyField = std::make_shared<VirtualKeyField>(COL1, ROW(2), 50, ROW_HEIGHT, VK_OEM_5, "WhiteboardKey"));
#endif

#if USEMEGAMAP
	m_widgets.push_back(std::make_shared<Label>(COL0, ROW(3), "Megamap Key"));
	m_widgets.push_back(m_megaMapVirtualKeyField = std::make_shared<VirtualKeyField>(COL1, ROW(3), 50, ROW_HEIGHT, VK_TAB, "MegamapKey"));
#endif

	m_widgets.push_back(std::make_shared<Label>(COL0, ROW(4), "PATROLLING CONSTRUCTION UNITS"));
#if PATROLING_CONS_RECLAIM_OR_ASSIST_ENABLE
	m_widgets.push_back(std::make_shared<Label>(COL0b, ROW(5), "Hold Pos"));
	m_widgets.push_back(std::make_shared<Label>(COL0b, ROW(6), "Maneuver"));
	m_widgets.push_back(std::make_shared<Label>(COL0b, ROW(7), "Roam"));
	static const std::vector<std::string> patrolButtonLabels({ "Reclaim Only", "Both", "Assist Only" });
	m_widgets.push_back(m_conUnitPatrolHoldPosButton = std::make_shared<Button>(COL0c, ROW(5), lpStagedButton3,
		RECLAIM_ONLY, 3, true, patrolButtonLabels, "ConUnitsPatrolHoldPosOption"));
	m_widgets.push_back(m_conUnitPatrolManeuverButton = std::make_shared<Button>(COL0c, ROW(6), lpStagedButton3,
		RECLAIM_AND_ASSIST, 3, true, patrolButtonLabels, "ConUnitsPatrolManeuverOption"));
	m_widgets.push_back(m_conUnitPatrolRoamButton = std::make_shared<Button>(COL0c, ROW(7), lpStagedButton3,
		RECLAIM_AND_ASSIST, 3, true, patrolButtonLabels, "ConUnitsPatrolRoamOption"));
#else
	m_widgets.push_back(std::make_shared<Label>(COL0b, ROW(5), "Options not available"));
#endif

	m_widgets.push_back(std::make_shared<Label>(COL0, ROW(8), "GUARDING CONSTRUCTION UNITS"));
#if FIXED_POSN_GUARDING_CONS_ENABLE
	m_widgets.push_back(std::make_shared<Label>(COL0b, ROW(9), "Hold Pos"));
	m_widgets.push_back(std::make_shared<Label>(COL0b, ROW(10), "Maneuver"));
	m_widgets.push_back(std::make_shared<Label>(COL0b, ROW(11), "Roam"));
	static const std::vector<std::string> guardButtonLabels({ "Stay", "Cavedog", "Scatter" });
	m_widgets.push_back(m_conUnitGuardHoldPosButton = std::make_shared<Button>(COL0c, ROW(9), lpStagedButton3,
		CAVEDOG, 3, true, guardButtonLabels, "ConUnitsGuardHoldPosOption"));
	m_widgets.push_back(m_conUnitGuardManeuverButton = std::make_shared<Button>(COL0c, ROW(10), lpStagedButton3,
		CAVEDOG, 3, true, guardButtonLabels, "ConUnitsGuardManeuverOption"));
	m_widgets.push_back(m_conUnitGuardRoamButton = std::make_shared<Button>(COL0c, ROW(11), lpStagedButton3,
		CAVEDOG, 3, true, guardButtonLabels, "ConUnitsGuardRoamOption"));
#else
	m_widgets.push_back(std::make_shared<Label>(COL0b, ROW(9), "Options not available"));
#endif

	// column 2, 3, 4
	m_widgets.push_back(std::make_shared<Label>(COL2, ROW(0), "Mex-Snap Radius"));
	m_widgets.push_back(m_mexSnapRadiusIntegerField = std::make_shared<IntegerField>(COL3, ROW(0), 20, ROW_HEIGHT,
		CTAHook::GetDefaultMexSnapRadius(), 0, CTAHook::GetMaxMexSnapRadius(), "MexSnapRadius2"));
	m_mexSnapRadiusIntegerField->m_disabled = CTAHook::GetMaxMexSnapRadius() == 0;
	if (!m_mexSnapRadiusIntegerField->m_disabled)
	{
		m_widgets.push_back(std::make_shared<Label>(COL4, ROW(0), "(0-" + std::to_string(CTAHook::GetMaxMexSnapRadius()) + ")"));
	}

	m_widgets.push_back(std::make_shared<Label>(COL2, ROW(1), "Wreck-Snap Radius"));
	m_widgets.push_back(m_wreckSnapRadiusIntegerField = std::make_shared<IntegerField>(COL3, ROW(1), 20, ROW_HEIGHT,
		CTAHook::GetDefaultWreckSnapRadius(), 0, CTAHook::GetMaxWreckSnapRadius(), "WreckSnapRadius2"));
	m_wreckSnapRadiusIntegerField->m_disabled = CTAHook::GetMaxWreckSnapRadius() == 0;
	if (!m_wreckSnapRadiusIntegerField->m_disabled)
	{
		m_widgets.push_back(std::make_shared<Label>(COL4, ROW(1), "(0-" + std::to_string(CTAHook::GetMaxWreckSnapRadius()) + ")"));
	}

	m_widgets.push_back(std::make_shared<Label>(COL2, ROW(2), "Chat Macro F11"));
	m_widgets.push_back(m_chatMacroTextField = std::make_shared<TextField>(COL2, ROW(3), 170, 5 * ROW_HEIGHT,
		"+setshareenergy 1000\x0d+setsharemetal 1000\x0d+shareall\x0d+shootall", "ShareText"));

	m_widgets.push_back(std::make_shared<Label>(COL2, ROW(8), "Resource Bar Background"));
	m_widgets.push_back(m_resourceBarBackgroundButton = std::make_shared<Button>(COL2, ROW(9), lpStagedButton3,
		1, 3, true, std::vector<std::string>({ "None", "Text", "Solid" }), "BackGround", std::function<void(int)>()));

#if TA_HOOK_ENABLE
	m_widgets.push_back(m_optimiseDtRowsButton = std::make_shared<Button>(COL2, ROW(10), lpCheckBox,
		1, 2, false, std::vector<std::string>(), "OptimizeDT", std::function<void(int)>()));
	m_widgets.push_back(std::make_shared<Label>(COL2 + m_optimiseDtRowsButton->m_width + 4, ROW(10), "Optimize DT Rows"));

	m_widgets.push_back(m_enableFullRingsButton = std::make_shared<Button>(COL2, ROW(11), lpCheckBox,
		1, 2, false, std::vector<std::string>(), "FullRings", std::function<void(int)>()));
	m_widgets.push_back(std::make_shared<Label>(COL2 + m_enableFullRingsButton->m_width + 4, ROW(11), "Enable FullRings"));
#endif

	m_widgets.push_back(m_vsyncButton = std::make_shared<Button>(COL2, ROW(12), lpCheckBox,
		0, 2, false, std::vector<std::string>(), "VSync"));
	m_widgets.push_back(std::make_shared<Label>(COL2 + m_vsyncButton->m_width + 4, ROW(12), "VSync"));

	m_widgets.push_back(m_okButton = std::make_shared<Button>(COL3-16, ROW(12), lpOKButton,
		0, 1, true, std::vector<std::string>(), "",
		[this](int)
	{
		SetAll();
		HideDialog();
	}));

	LocalShare->Dialog = this;
	FirstBlit = true;

	ReadPos();
	ReadSettings();

	EnterOption_hook= new InlineSingleHook ( EnterOption_Address, 5, INLINE_5BYTESLAGGERJMP, EnterOption);
	PressInOption_hook= new InlineSingleHook ( PressInOption_Address, 5, INLINE_5BYTESLAGGERJMP, PressInOption);
	
	IDDrawSurface::OutptTxt ( "New Dialog");
}

Dialog::~Dialog()
{
	// lpDialogSurf, background, fonts, and cursor are released by ~DialogBase().
	if(lpOKButton)
		lpOKButton->Release();
	if(lpStagedButton3)
		lpStagedButton3->Release();
	if(lpCheckBox)
		lpCheckBox->Release();
	if(lpStagedButton1)
		lpStagedButton1->Release();
	if(lpStandardButton)
		lpStandardButton->Release();
	if (EnterOption_hook)
	{
		delete EnterOption_hook;
	}
	if (PressInOption_hook)
	{
		delete PressInOption_hook;
	}
	
	WritePos();
	WriteSettings();
	LocalShare->Dialog = NULL;
}

void Dialog::ShowDialog()
{
	for (auto w : m_widgets)
	{
		w->m_focused = false;
	}

    posX = 1024 - DialogWidth;
    posY = 30;
	CorrectPos(); //make sure dialog is inside screen
	RestoreAll ( );
	RenderDialog();
	m_visible = true;
}

void Dialog::HideDialog()
{
	m_visible  = false;
	CursorPosX = -1;
	CursorPosY = -1;

	WritePos();
	WriteSettings();
}

void Dialog::BlitDialog(LPDIRECTDRAWSURFACE DestSurf)
{
	if(FirstBlit)
	{
		SetAll();
		FirstBlit = false;
	}
	DialogBase::BlitDialog(DestSurf);
}

void Dialog::RestoreAll()
{
	DialogBase::RestoreAll();  // shared surfaces + cursor

	lpOKButton->Restore();
	lpStagedButton3->Restore();
	lpCheckBox->Restore();
	lpStagedButton1->Restore();
	lpStandardButton->Restore();

	RestoreFromPCX(5,  lpOKButton);
	RestoreFromPCX(6,  lpStagedButton3);
	RestoreFromPCX(10, lpCheckBox);
	RestoreFromPCX(11, lpStagedButton1);
	RestoreFromPCX(12, lpStandardButton);

	RenderDialog();
}

bool Dialog::Message(HWND WinProchWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	// ctrl-F2: open dialog (Dialog-specific shortcut).
	if (!m_visible)
	{
		if (DataShare->TAProgress == TAInGame &&
			Msg == WM_KEYDOWN && wParam == 113 && (GetAsyncKeyState(17) & 0x8000) > 0 /*ctrl*/)
		{
			ShowDialog();
			return true;
		}
		return false;
	}

	__try
	{
		return DialogBase::Message(WinProchWnd, Msg, wParam, lParam);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		;
	}
	return false;
}

void Dialog::OnDragMoved()
{
	CorrectPos();
}

void Dialog::OnMouseInsideDialog(int /*mx*/, int /*my*/)
{
#if USEMEGAMAP
	if ((GUIExpander)
		&& (GUIExpander->myMinimap)
		&& (GUIExpander->myMinimap->Controler))
	{
		GUIExpander->myMinimap->Controler->PubCursorX = -1;
		GUIExpander->myMinimap->Controler->PubCursorY = -1;
	}
#endif
}

int Dialog::GetClickSnapOverrideKey()
{
	return m_clickSnapOverrideVirtualKeyField->m_vk;
}

void Dialog::SetAll()
{
	CIncome *Income = (CIncome*)LocalShare->Income;
	if (Income)
	{
		Income->Set(m_resourceBarBackgroundButton->GetState());
	}

	IDDrawSurface *SurfClass = (IDDrawSurface*)LocalShare->DDrawSurfClass;
	if (SurfClass)
	{
		SurfClass->Set (m_vsyncButton->GetState());
	}
	
#if TA_HOOK_ENABLE
	CTAHook *TAHook = (CTAHook*)LocalShare->TAHook;
	if (TAHook)
	{
		TAHook->Set(
			m_autoClickVirtualKeyField ? m_autoClickVirtualKeyField->m_vk : 0,
			m_chatMacroTextField ? m_chatMacroTextField->m_text.c_str() : "",
			m_optimiseDtRowsButton ? m_optimiseDtRowsButton->GetState() : 1,
			m_enableFullRingsButton ? m_enableFullRingsButton->GetState() : 1,
			10,
			m_mexSnapRadiusIntegerField ? m_mexSnapRadiusIntegerField->m_value : 0,
			m_wreckSnapRadiusIntegerField ? m_wreckSnapRadiusIntegerField->m_value : 0,
			m_clickSnapOverrideVirtualKeyField ? m_clickSnapOverrideVirtualKeyField->m_vk : 0
		);
	}
#endif

#if USEWHITEBOARD
	AlliesWhiteboard *WB = (AlliesWhiteboard*)LocalShare->Whiteboard;
	if (WB)
	{
		WB->Set(m_whiteboardVirtualKeyField->m_vk);
	}
#endif
	
#if USEMEGAMAP
	if (GUIExpander
		&&GUIExpander->myMinimap)
	{
		GUIExpander->myMinimap->Set (m_megaMapVirtualKeyField->m_vk);
	}
#endif

}

int Dialog::GetConUnitPatrolHoldPosOption()
{
	return m_conUnitPatrolHoldPosButton
		? m_conUnitPatrolHoldPosButton->GetState()
		: RECLAIM_AND_ASSIST;
}

int Dialog::GetConUnitPatrolManeuverOption()
{
	return m_conUnitPatrolManeuverButton
		? m_conUnitPatrolManeuverButton->GetState()
		: RECLAIM_AND_ASSIST;
}

int Dialog::GetConUnitPatrolRoamOption()
{
	return m_conUnitPatrolRoamButton
		? m_conUnitPatrolRoamButton->GetState()
		: RECLAIM_AND_ASSIST;
}

int Dialog::GetConUnitPatrolOption(int unitMovementSetting)
{
	switch (unitMovementSetting)
	{
	case 0: return GetConUnitPatrolHoldPosOption();
	case 1: return GetConUnitPatrolManeuverOption();
	case 2: return GetConUnitPatrolRoamOption();
	default: return RECLAIM_AND_ASSIST;
	};
}

int Dialog::GetConUnitGuardHoldPosOption()
{
	return m_conUnitGuardHoldPosButton
		? m_conUnitGuardHoldPosButton->GetState()
		: CAVEDOG;
}

int Dialog::GetConUnitGuardManeuverOption()
{
	return m_conUnitGuardManeuverButton
		? m_conUnitGuardManeuverButton->GetState()
		: CAVEDOG;
}

int Dialog::GetConUnitGuardRoamOption()
{
	return m_conUnitGuardRoamButton
		? m_conUnitGuardRoamButton->GetState()
		: CAVEDOG;
}

int Dialog::GetConUnitGuardOption(int unitMovementSetting)
{
	switch (unitMovementSetting)
	{
	case 0: return GetConUnitGuardHoldPosOption();
	case 1: return GetConUnitGuardManeuverOption();
	case 2: return GetConUnitGuardRoamOption();
	default: return RECLAIM_AND_ASSIST;
	};
}

//reads dialog position from registry
void Dialog::ReadPos()
{
	HKEY hKey;
	DWORD dwDisposition;
	DWORD Size = sizeof(int);

	std::string SubKey = CompanyName_CCSTR;
	SubKey += "\\Eye";

	RegCreateKeyEx(HKEY_CURRENT_USER, SubKey.c_str(), NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	RegQueryValueEx(hKey, "DialogPosX", NULL, NULL, (unsigned char*)&posX, &Size);
	RegQueryValueEx(hKey, "DialogPosY", NULL, NULL, (unsigned char*)&posY, &Size);

	RegCloseKey(hKey);
}

void Dialog::WriteSettings()
{
	HKEY hKey;
	HKEY hKey1;
	DWORD dwDisposition;

	RegCreateKeyEx(HKEY_CURRENT_USER, CompanyName_CCSTR, NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey1, &dwDisposition);
	RegCreateKeyEx(hKey1, "Eye", NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	for (auto w : m_widgets)
	{
		if (!w->m_disabled)
		{
			w->RegistryWrite(hKey);
		}
	}

	RegCloseKey(hKey);
	RegCloseKey(hKey1);
}

void Dialog::ReadSettings()
{
	HKEY hKey;
	DWORD dwDisposition;

	std::string SubKey = CompanyName_CCSTR;
	SubKey += "\\Eye";

	RegCreateKeyEx(HKEY_CURRENT_USER, SubKey.c_str(), NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	for (auto w : m_widgets)
	{
		if (!w->m_disabled)
		{
			w->RegistryRead(hKey);
		}
	}

	RegCloseKey(hKey);
}

void Dialog::WritePos()
{
	HKEY hKey;
	HKEY hKey1;
	DWORD dwDisposition;

	RegCreateKeyEx(HKEY_CURRENT_USER, CompanyName_CCSTR, NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey1, &dwDisposition);

	RegCreateKeyEx(hKey1, "Eye", NULL, TADRCONFIGREGNAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	RegSetValueEx(hKey, "DialogPosX", NULL, REG_DWORD, (unsigned char*)&posX, sizeof(int));
	RegSetValueEx(hKey, "DialogPosY", NULL, REG_DWORD, (unsigned char*)&posY, sizeof(int));

	RegCloseKey(hKey);
	RegCloseKey(hKey1);
}

void Dialog::CorrectPos()
{
    RECT bounds;
    if (DataShare->TAProgress == TAInGame) {
        std::memcpy(&bounds, &(*TAmainStruct_PtrPtr)->GameSreen_Rect, sizeof(bounds));
    }
    else {
        bounds.left = bounds.top = 0;
        bounds.right = (*TAProgramStruct_PtrPtr)->ScreenWidth;
        bounds.bottom = (*TAProgramStruct_PtrPtr)->ScreenHeight;
    }

    if(posX < bounds.left)
		posX = bounds.left;
	if(posX > 1+ bounds.right - DialogWidth)
		posX = 1+ bounds.right - DialogWidth;

	if(posY < bounds.top)
		posY = bounds.top;
	if(posY > 1+ bounds.bottom - DialogHeight)
		posY = 1+ bounds.bottom - DialogHeight;
}

bool Dialog::IsShow (LPRECT rect_p)
{
	if (NULL!=rect_p)
	{
		rect_p->left = posX;
		rect_p->top = posY;
		rect_p->right = posX + DialogWidth;
		rect_p->bottom = posY + DialogHeight;
	}
	return m_visible;
}
int __stdcall EnterOption (PInlineX86StackBuffer X86StrackBuffer)
{
    ((Dialog*)LocalShare->Dialog)->ShowDialog();
	return 0;
}

int __stdcall PressInOption (PInlineX86StackBuffer X86StrackBuffer)
{
    ((Dialog*)LocalShare->Dialog)->SetAll();
    ((Dialog*)LocalShare->Dialog)->HideDialog();
	return 0;
}
