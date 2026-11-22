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
#include "Widgets/TabButton.h"

#include <stdio.h>
#include <algorithm>

Dialog::Dialog(BOOL Vidmem_a)
    : DialogBase(Vidmem_a, DialogWidth, DialogHeight, ROW_HEIGHT)
{
	lpMenuButton    = CreateSurfPCXResource(15, Vidmem_a);
    lpPagePrevious = CreateSurfPCXResource(16, Vidmem_a);
    lpPageNext = CreateSurfPCXResource(17, Vidmem_a);
    lpStyleSelector = CreateSurfPCXResource(18, Vidmem_a);
    lpViewSelector = CreateSurfPCXResource(19, Vidmem_a);
	lpStagedButton3 = CreateSurfPCXResource(6,  Vidmem_a);
	lpCheckBox      = CreateSurfPCXResource(10, Vidmem_a);
	lpStagedButton1 = CreateSurfPCXResource(11, Vidmem_a);


    BuildControls();

	LocalShare->Dialog = this;
	FirstBlit = true;

	ReadPos();
	ReadSettings();
    UpdateEconomyControls();
    SelectPage(Controls);

	EnterOption_hook= new InlineSingleHook ( EnterOption_Address, 5, INLINE_5BYTESLAGGERJMP, EnterOption);
	PressInOption_hook= new InlineSingleHook ( PressInOption_Address, 5, INLINE_5BYTESLAGGERJMP, PressInOption);

	IDDrawSurface::OutptTxt ( "New Dialog");
}

// All profiles use this one control definition and the same saved settings.
// Capability flags omit unsupported controls; there is no second flat layout.
#define PAGE_ROW(n) (34 + (n) * 22)

void Dialog::AddControl(Page page, const std::shared_ptr<Widget>& widget)
{
    m_pages[page].push_back(widget);
    m_widgets.push_back(widget);
}

void Dialog::BuildControls()
{
    m_widgets.push_back(std::make_shared<Label>(16, 10, "OPTIONS"));
    const char* names[] = { "Controls", "Units", "Resources" };
    for (int page = Controls; page < PageCount; ++page)
        m_widgets.push_back(std::make_shared<TabButton>(8, 32 + page * 40, lpMenuButton,
            names[page], [this, page]() { return m_page == page; }, [this, page]() { SelectPage(page); }));
    AddControl(Controls, std::make_shared<Label>(132, 10, "CONTROLS"));
    AddControl(Units, std::make_shared<Label>(132, 10, "CONSTRUCTION UNITS"));
    AddControl(Economy, std::make_shared<Label>(132, 10, "ALLIED RESOURCE BARS"));
#if MAX_MEX_SNAP_RADIUS || MAX_WRECK_SNAP_RADIUS
    AddControl(Controls, std::make_shared<Label>(132, PAGE_ROW(0), "Snap Override Key"));
    AddControl(Controls, m_clickSnapOverrideVirtualKeyField = std::make_shared<VirtualKeyField>(250, PAGE_ROW(0), 50, ROW_HEIGHT, VK_MENU, "ClickSnapOverrideKey"));
#endif
#if TA_HOOK_ENABLE
    AddControl(Controls, std::make_shared<Label>(132, PAGE_ROW(1), "Autoclick Key"));
    AddControl(Controls, m_autoClickVirtualKeyField = std::make_shared<VirtualKeyField>(250, PAGE_ROW(1), 50, ROW_HEIGHT, 88, "KeyCode"));
#endif
#if USEWHITEBOARD
    AddControl(Controls, std::make_shared<Label>(132, PAGE_ROW(2), "Whiteboard Key"));
    AddControl(Controls, m_whiteboardVirtualKeyField = std::make_shared<VirtualKeyField>(250, PAGE_ROW(2), 50, ROW_HEIGHT, VK_OEM_5, "WhiteboardKey"));
#endif
#if USEMEGAMAP
    AddControl(Controls, std::make_shared<Label>(132, PAGE_ROW(3), "Megamap Key"));
    AddControl(Controls, m_megaMapVirtualKeyField = std::make_shared<VirtualKeyField>(250, PAGE_ROW(3), 50, ROW_HEIGHT, VK_TAB, "MegamapKey"));
#endif
    AddControl(Controls, std::make_shared<Label>(132, PAGE_ROW(4), "Rotate Build Key"));
    AddControl(Controls, m_rotateBuildVirtualKeyField = std::make_shared<VirtualKeyField>(250, PAGE_ROW(4), 50, ROW_HEIGHT, VK_OEM_2, "RotateBuildKey"));
    AddControl(Controls, std::make_shared<Label>(132, 166, "Mex-Snap Radius"));
    AddControl(Controls, m_mexSnapRadiusIntegerField = std::make_shared<IntegerField>(250, 166, 20, ROW_HEIGHT,
        CTAHook::GetDefaultMexSnapRadius(), 0, CTAHook::GetMaxMexSnapRadius(), "MexSnapRadius2"));
    m_mexSnapRadiusIntegerField->m_disabled = CTAHook::GetMaxMexSnapRadius() == 0;
    if (!m_mexSnapRadiusIntegerField->m_disabled)
        AddControl(Controls, std::make_shared<Label>(274, 166, "(0-" + std::to_string(CTAHook::GetMaxMexSnapRadius()) + ")"));
    AddControl(Controls, std::make_shared<Label>(132, 188, "Wreck-Snap Radius"));
    AddControl(Controls, m_wreckSnapRadiusIntegerField = std::make_shared<IntegerField>(250, 188, 20, ROW_HEIGHT,
        CTAHook::GetDefaultWreckSnapRadius(), 0, CTAHook::GetMaxWreckSnapRadius(), "WreckSnapRadius2"));
    m_wreckSnapRadiusIntegerField->m_disabled = CTAHook::GetMaxWreckSnapRadius() == 0;
    if (!m_wreckSnapRadiusIntegerField->m_disabled)
        AddControl(Controls, std::make_shared<Label>(274, 188, "(0-" + std::to_string(CTAHook::GetMaxWreckSnapRadius()) + ")"));
    AddControl(Controls, std::make_shared<Label>(310, PAGE_ROW(0), "Chat Macro F11"));
    AddControl(Controls, m_chatMacroTextField = std::make_shared<TextField>(310, PAGE_ROW(1), 186, 4 * ROW_HEIGHT,
        "+setshareenergy 1000\x0d+setsharemetal 1000\x0d+shareall\x0d+shootall", "ShareText"));
#if TA_HOOK_ENABLE
    AddControl(Controls, m_optimiseDtRowsButton = std::make_shared<Button>(322, 146, lpCheckBox, 1, 2, false, std::vector<std::string>(), "OptimizeDT"));
    AddControl(Controls, std::make_shared<Label>(342, 146, "Optimize DT Rows"));
    AddControl(Controls, m_enableFullRingsButton = std::make_shared<Button>(322, 168, lpCheckBox, 1, 2, false, std::vector<std::string>(), "FullRings"));
    AddControl(Controls, std::make_shared<Label>(342, 168, "Enable FullRings"));
#endif
    AddControl(Controls, m_chatBackdropButton = std::make_shared<Button>(322, 190, lpCheckBox, 0, 2, false, std::vector<std::string>(), "ChatBackdrop"));
    AddControl(Controls, std::make_shared<Label>(342, 190, "Accessible Chat"));
    AddControl(Controls, m_vsyncButton = std::make_shared<Button>(322, 212, lpCheckBox, 0, 2, false, std::vector<std::string>(), "VSync"));
    AddControl(Controls, std::make_shared<Label>(342, 212, "VSync"));
#if ALLIED_BUILD_QUEUE_ENABLE
    AddControl(Controls, m_showAlliedBuildQueuesButton = std::make_shared<Button>(322, 234, lpCheckBox,
        1, 2, false, std::vector<std::string>(), "ShowAlliedBuildQueues"));
    AddControl(Controls, std::make_shared<Label>(342, 234, "Show ally queues"));
#endif

    AddControl(Units, m_buildMenuRotationOverlayButton = std::make_shared<Button>(132, 34, lpCheckBox,
        1, 2, false, std::vector<std::string>(), "BuildMenuRotationOverlay"));
    AddControl(Units, std::make_shared<Label>(152, 34, "Build rotation overlay"));
    AddControl(Units, std::make_shared<Label>(212, 80, "PATROLLING"));
    AddControl(Units, std::make_shared<Label>(376, 80, "GUARDING"));
    AddControl(Units, std::make_shared<Label>(132, 104, "Hold Pos"));
    AddControl(Units, std::make_shared<Label>(132, 126, "Maneuver"));
    AddControl(Units, std::make_shared<Label>(132, 148, "Roam"));
#if PATROLING_CONS_RECLAIM_OR_ASSIST_ENABLE
    const std::vector<std::string> patrolLabels({ "Reclaim Only", "Both", "Assist Only" });
    AddControl(Units, m_conUnitPatrolHoldPosButton = std::make_shared<Button>(212, 104, lpStagedButton3, RECLAIM_ONLY, 3, true, patrolLabels, "ConUnitsPatrolHoldPosOption"));
    AddControl(Units, m_conUnitPatrolManeuverButton = std::make_shared<Button>(212, 126, lpStagedButton3, RECLAIM_AND_ASSIST, 3, true, patrolLabels, "ConUnitsPatrolManeuverOption"));
    AddControl(Units, m_conUnitPatrolRoamButton = std::make_shared<Button>(212, 148, lpStagedButton3, RECLAIM_AND_ASSIST, 3, true, patrolLabels, "ConUnitsPatrolRoamOption"));
#else
    AddControl(Units, std::make_shared<Label>(212, 104, "Not available"));
#endif
#if FIXED_POSN_GUARDING_CONS_ENABLE
    const std::vector<std::string> guardLabels({ "Stay", "Cavedog", "Scatter" });
    AddControl(Units, m_conUnitGuardHoldPosButton = std::make_shared<Button>(376, 104, lpStagedButton3, CAVEDOG, 3, true, guardLabels, "ConUnitsGuardHoldPosOption"));
    AddControl(Units, m_conUnitGuardManeuverButton = std::make_shared<Button>(376, 126, lpStagedButton3, CAVEDOG, 3, true, guardLabels, "ConUnitsGuardManeuverOption"));
    AddControl(Units, m_conUnitGuardRoamButton = std::make_shared<Button>(376, 148, lpStagedButton3, CAVEDOG, 3, true, guardLabels, "ConUnitsGuardRoamOption"));
#else
    AddControl(Units, std::make_shared<Label>(376, 104, "Not available"));
#endif

    AddControl(Economy, std::make_shared<Label>(132, 34, "SETTINGS"));
#if MODERN_ALLY_RESOURCE_UI_ENABLE
    AddControl(Economy, std::make_shared<Label>(132, 56, "Style"));
    AddControl(Economy, m_resourceBarStyleButton = std::make_shared<Button>(376, 56, lpStyleSelector,
        1, 2, true, std::vector<std::string>({ "Classic", "Enhanced" }), "NewAllyResourceBars", [this](int) { UpdateEconomyControls(); }));
#endif
    const int backgroundY = MODERN_ALLY_RESOURCE_UI_ENABLE ? 78 : 56;
    AddControl(Economy, std::make_shared<Label>(132, backgroundY, "Background"));
    AddControl(Economy, m_resourceBarBackgroundButton = std::make_shared<Button>(376, backgroundY, lpStagedButton3,
        1, 3, true, std::vector<std::string>({ "None", "Text", "Solid" }), "BackGround", [](int state)
        { if (LocalShare->Income) static_cast<CIncome*>(LocalShare->Income)->Set(state); }));
#if MODERN_ALLY_RESOURCE_UI_ENABLE
    AddEnhancedControl(std::make_shared<Label>(132, 112, "ENHANCED SETTINGS"));
    AddEnhancedControl(std::make_shared<Label>(132, 134, "Show"));
    AddEnhancedControl(m_economyViewButton = std::make_shared<Button>(376, 134, lpViewSelector,
        2, 4, true, std::vector<std::string>({ "Incomes", "Totals", "Both", "Off" }), "EconomyView", [this](int state)
        {
            bool totalsUnavailable = !GetReplayTotalsEnabled() ||
                (DataShare->TAProgress == TAInGame && !DataShare->PlayingDemo);
            if ((state == 1 || state == 2) && totalsUnavailable) m_economyViewButton->SetState(3);
        }));
    AddEnhancedControl(std::make_shared<Label>(132, 156, "Mouse wheel swaps Incomes/Totals in single view"));
    AddEnhancedControl(std::make_shared<Label>(132, 180, "TOTALS INCLUDED"));
    AddEnhancedControl(m_metalTotalsButton = std::make_shared<Button>(244, 180, lpCheckBox, 1, 2, false, std::vector<std::string>(), "ReplayTotalsMetal"));
    AddEnhancedControl(std::make_shared<Label>(264, 180, "Metal"));
    AddEnhancedControl(m_energyTotalsButton = std::make_shared<Button>(316, 180, lpCheckBox, 1, 2, false, std::vector<std::string>(), "ReplayTotalsEnergy"));
    AddEnhancedControl(std::make_shared<Label>(336, 180, "Energy"));
    AddEnhancedControl(m_unitTotalsButton = std::make_shared<Button>(392, 180, lpCheckBox, 1, 2, false, std::vector<std::string>(), "ReplayTotalsUnits"));
    AddEnhancedControl(std::make_shared<Label>(412, 180, "Units"));
    AddEnhancedControl(std::make_shared<Label>(132, 202, "Metal/Energy: total produced. Units: current total."));
    AddEnhancedControl(std::make_shared<Label>(132, 224, "Income layout"));
    AddEnhancedControl(m_incomeLayoutButton = std::make_shared<Button>(376, 224, lpStyleSelector,
        0, 2, true, std::vector<std::string>({ "Vertical", "Horizontal" }), "IncomeLayout"));
    AddEnhancedControl(std::make_shared<Label>(132, 246, "Totals layout"));
    AddEnhancedControl(m_replayTotalsLayoutButton = std::make_shared<Button>(376, 246, lpStyleSelector,
        0, 2, true, std::vector<std::string>({ "Vertical", "Horizontal" }), "TotalsLayout"));
    AddEnhancedControl(std::make_shared<TabButton>(128, 272, lpMenuButton, "Restore", []() { return false; }, []()
    { if (LocalShare->Income) static_cast<CIncome*>(LocalShare->Income)->ResetEconomyPositions(); }));
#endif
    m_widgets.push_back(m_okButton = std::make_shared<TabButton>(8, 272, lpMenuButton, "OK", []() { return false; }, [this]()
    { SetAll(); HideDialog(); }));
    auto previousPage = std::make_shared<Button>(400, 288, lpPagePrevious, 0, 1, true,
        std::vector<std::string>(), "", [this](int) { PlaySound_Effect("nextbuildmenu", 0); SelectPage((m_page + PageCount - 1) % PageCount); });
    previousPage->m_colourKey = 9;
    m_widgets.push_back(previousPage);
    auto nextPage = std::make_shared<Button>(450, 288, lpPageNext, 0, 1, true,
        std::vector<std::string>(), "", [this](int) { PlaySound_Effect("nextbuildmenu", 0); SelectPage((m_page + 1) % PageCount); });
    nextPage->m_colourKey = 9;
    m_widgets.push_back(nextPage);
}

void Dialog::SelectPage(int page)
{
    if (page < 0 || page >= PageCount) page = Controls;
    for (auto& widget : m_widgets) widget->CancelInteraction();
    Move = false;
    m_page = page;
    for (int group = Controls; group < PageCount; ++group)
        for (auto& widget : m_pages[group]) widget->m_hidden = group != page;
}

void Dialog::AddEnhancedControl(const std::shared_ptr<Widget>& widget)
{
    AddControl(Economy, widget);
    m_enhancedControls.push_back(widget);
}

void Dialog::UpdateEconomyControls()
{
    bool disabled = !GetNewResourceBarsEnabled();
    for (const auto& widget : m_enhancedControls)
    {
        widget->m_disabled = widget->m_dimmed = disabled;
        if (disabled) widget->CancelInteraction();
    }
}

bool Dialog::EditingField() const
{
    for (const auto& w : m_widgets)
        if (w->m_focused && !w->m_hidden && !w->m_disabled &&
            (dynamic_cast<VirtualKeyField*>(w.get()) || dynamic_cast<TextField*>(w.get()) || dynamic_cast<IntegerField*>(w.get())))
            return true;
    return false;
}

Dialog::~Dialog()
{
	// lpDialogSurf, background, fonts, and cursor are released by ~DialogBase().
	if(lpMenuButton)
		lpMenuButton->Release();
    if (lpPagePrevious) lpPagePrevious->Release();
    if (lpPageNext) lpPageNext->Release();
    if (lpStyleSelector) lpStyleSelector->Release();
    if (lpViewSelector) lpViewSelector->Release();
	if(lpStagedButton3)
		lpStagedButton3->Release();
	if(lpCheckBox)
		lpCheckBox->Release();
	if(lpStagedButton1)
		lpStagedButton1->Release();

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

    posX = ((*TAProgramStruct_PtrPtr)->ScreenWidth - DialogWidth) / 2;
    posY = ((*TAProgramStruct_PtrPtr)->ScreenHeight - DialogHeight) / 2;
	CorrectPos(); //make sure dialog is inside screen
	RestoreAll ( );
	RenderDialog();
	m_visible = true;
}

void Dialog::HideDialog()
{
	for (auto& widget : m_widgets) widget->CancelInteraction();
	Move = false;
	m_visible  = false;
	CursorPosX = -1;
	CursorPosY = -1;

	WritePos();
	WriteSettings();
}

void Dialog::RenderDialog()
{
    RenderBackground();
    // A recessed divider echoes TA's left-hand options navigation. Existing
    // backgrounds, fonts and button skins remain the only image resources.
    if (lpDialogSurf)
    {
        DDBLTFX fill = {}; fill.dwSize = sizeof(fill);
        RECT dark = { 120, 8, 121, DialogHeight - 8 };
        RECT light = { 121, 8, 122, DialogHeight - 8 };
        fill.dwFillColor = 0;
        lpDialogSurf->Blt(&dark, nullptr, nullptr, DDBLT_WAIT | DDBLT_COLORFILL, &fill);
        fill.dwFillColor = 90;
        lpDialogSurf->Blt(&light, nullptr, nullptr, DDBLT_WAIT | DDBLT_COLORFILL, &fill);
    }
    if (m_page == Economy && MODERN_ALLY_RESOURCE_UI_ENABLE && lpDialogSurf)
    {
        DDBLTFX fill = {}; fill.dwSize = sizeof(fill);
        for (int top : { 102 })
        {
            RECT line = { 132, top, 496, top + 1 }; fill.dwFillColor = 0;
            lpDialogSurf->Blt(&line, nullptr, nullptr, DDBLT_WAIT | DDBLT_COLORFILL, &fill);
            line.top = top + 1; line.bottom = top + 2; fill.dwFillColor = 90;
            lpDialogSurf->Blt(&line, nullptr, nullptr, DDBLT_WAIT | DDBLT_COLORFILL, &fill);
        }
    }
    for (auto& widget : m_widgets) widget->Draw(this);

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

	lpMenuButton->Restore();
    lpPagePrevious->Restore();
    lpPageNext->Restore();
    lpStyleSelector->Restore();
    lpViewSelector->Restore();
	lpStagedButton3->Restore();
	lpCheckBox->Restore();
	lpStagedButton1->Restore();


	RestoreFromPCX(15, lpMenuButton);
    RestoreFromPCX(16, lpPagePrevious);
    RestoreFromPCX(17, lpPageNext);
    RestoreFromPCX(18, lpStyleSelector);
    RestoreFromPCX(19, lpViewSelector);
	RestoreFromPCX(6,  lpStagedButton3);
	RestoreFromPCX(10, lpCheckBox);
	RestoreFromPCX(11, lpStagedButton1);


	RenderDialog();
}

bool Dialog::Message(HWND WinProchWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (DataShare->TAProgress == TAInGame && Msg == WM_KEYDOWN && wParam == VK_F2 &&
		(GetAsyncKeyState(VK_CONTROL) & 0x8000))
	{
		if (lParam & (1L << 30)) return true;
		if (m_visible) { SetAll(); HideDialog(); } else ShowDialog();
		return true;
	}
	if (!m_visible) return false;

	__try
	{
		if (Msg == WM_KEYDOWN && wParam == VK_ESCAPE)
		{ SetAll(); HideDialog(); return true; }
        if (!EditingField() && !(GetAsyncKeyState(VK_CONTROL) & 0x8000) && !(GetAsyncKeyState(VK_MENU) & 0x8000))
        {
            if ((Msg == WM_KEYDOWN || Msg == WM_KEYUP) && (wParam == VK_OEM_COMMA || wParam == VK_OEM_PERIOD))
            {
                if (Msg == WM_KEYDOWN && !(lParam & (1L << 30)))
                {
                    PlaySound_Effect("nextbuildmenu", 0);
                    SelectPage((m_page + (wParam == VK_OEM_COMMA ? PageCount - 1 : 1)) % PageCount);
                }
                return true;
            }
            if (Msg == WM_CHAR && (wParam == ',' || wParam == '.' || wParam == '<' || wParam == '>')) return true;
        }
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

int Dialog::GetRotateBuildKey()
{
	return m_rotateBuildVirtualKeyField ? m_rotateBuildVirtualKeyField->m_vk : VK_OEM_2;
}

bool Dialog::GetBuildMenuRotationOverlayEnabled()
{
	// Default to enabled when the widget hasn't been built yet (e.g. very
	// early ctor calls before the dialog page is laid out).
	return !m_buildMenuRotationOverlayButton
		|| m_buildMenuRotationOverlayButton->GetState() != 0;
}

#if ALLIED_BUILD_QUEUE_ENABLE
bool Dialog::GetShowAlliedBuildQueuesEnabled()
{
	return !m_showAlliedBuildQueuesButton
		|| m_showAlliedBuildQueuesButton->GetState() != 0;
}
#endif

bool Dialog::GetChatBackdropEnabled()
{
	// Default OFF: absent widget (very early ctor calls) => disabled.
	return m_chatBackdropButton
		&& m_chatBackdropButton->GetState() != 0;
}

bool Dialog::GetReplayTotalsEnabled()
{
	return MODERN_ALLY_RESOURCE_UI_ENABLE && GetReplayTotalsMask() != 0;
}

int Dialog::GetReplayTotalsMask()
{
	return (m_metalTotalsButton && m_metalTotalsButton->GetState() ? 1 : 0)
		| (m_energyTotalsButton && m_energyTotalsButton->GetState() ? 2 : 0)
		| (m_unitTotalsButton && m_unitTotalsButton->GetState() ? 4 : 0);
}

int Dialog::GetEconomyEnabledView()
{
    return 1 | (GetReplayTotalsEnabled() ? 2 : 0);
}

int Dialog::GetEconomyView()
{
    return m_economyViewButton ? (m_economyViewButton->GetState() + 1) % 4 : 3;
}

void Dialog::SetEconomyView(int view)
{
    // SetState does not invoke a button callback or write the registry.
    if (m_economyViewButton) m_economyViewButton->SetState(((view & 3) + 3) % 4);
}

int Dialog::GetIncomeLayout()
{
    return m_incomeLayoutButton ? m_incomeLayoutButton->GetState() : 0;
}

int Dialog::GetReplayTotalsLayout()
{
	return m_replayTotalsLayoutButton ? m_replayTotalsLayoutButton->GetState() + 1 : 1;
}

bool Dialog::GetNewResourceBarsEnabled()
{
    return MODERN_ALLY_RESOURCE_UI_ENABLE && (!m_resourceBarStyleButton || m_resourceBarStyleButton->GetState() != 0);
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
		// Classic disables modern controls for input, but must still save their
		// selected values so switching styles does not discard customisation.
		if (!w->m_disabled || std::find(m_pages[Economy].begin(), m_pages[Economy].end(), w) != m_pages[Economy].end())
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
    bounds.left = bounds.top = 0;
    bounds.right = (*TAProgramStruct_PtrPtr)->ScreenWidth;
    bounds.bottom = (*TAProgramStruct_PtrPtr)->ScreenHeight;

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
