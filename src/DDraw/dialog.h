#ifndef dialogH
#define dialogH

#include "config.h"
#include "DialogBase.h"

#include <memory>
#include <string>
#include <vector>
#include <array>

struct tagInlineX86StackBuffer;
typedef struct tagInlineX86StackBuffer * PInlineX86StackBuffer;

class InlineSingleHook;
class VirtualKeyField;
class IntegerField;
class TextField;
class Button;
class TabButton;
class Widget;

#define ROW_HEIGHT 18
#define ROW(n) ((n)*ROW_HEIGHT + ROW_HEIGHT/2 - 1)
#define DialogHeight 320
#define DialogWidth 512

class Dialog : public DialogBase
{
public:
	Dialog(BOOL VidMem_a);
	~Dialog();
	void ShowDialog();
	void HideDialog();
	bool IsShow(LPRECT rect_p);

	// BlitDialog: handles FirstBlit initialisation, then delegates to DialogBase.
	void BlitDialog(LPDIRECTDRAWSURFACE DestSurf) override;

	// Message: handles ctrl-F2 toggle and wraps DialogBase::Message in SEH.
	bool Message(HWND WinProchWnd, UINT Msg, WPARAM wParam, LPARAM lParam) override;

	void SetAll();

	static const int RECLAIM_ONLY = 0;
	static const int RECLAIM_AND_ASSIST = 1;
	static const int ASSIST_ONLY = 2;
	int GetConUnitPatrolHoldPosOption();
	int GetConUnitPatrolManeuverOption();
	int GetConUnitPatrolRoamOption();
	int GetConUnitPatrolOption(int unitMovementSetting);

	static const int STAY = 0;
	static const int CAVEDOG = 1;
	static const int SCATTER = 2;
	int GetConUnitGuardHoldPosOption();
	int GetConUnitGuardManeuverOption();
	int GetConUnitGuardRoamOption();
	int GetConUnitGuardOption(int unitMovementSetting);

	int GetClickSnapOverrideKey();

	// VK code the player has assigned to the build-rotation cycle action.
	// Default VK_OEM_2 ('/'). Read by CUnitRotate::Message.
	int GetRotateBuildKey();

	// True when the player wants the build-menu rotation overlays
	// (chevrons / buildrotate.gaf) drawn AND wants quadrant clicks on
	// rotatable structure buttons intercepted to pre-set rotation.
	// When false the build menu behaves stock. Default ON.
	bool GetBuildMenuRotationOverlayEnabled();

#if ALLIED_BUILD_QUEUE_ENABLE
	// Local display preference for allied queued-build overlays. Default ON.
	bool GetShowAlliedBuildQueuesEnabled();
#endif

	// True when the player wants a solid black backdrop drawn behind in-game
	// chat text (top-left) for legibility. Default OFF. Read each frame by
	// ChatBackdrop::Draw.
	bool GetChatBackdropEnabled();

	bool GetReplayTotalsEnabled();
	int GetReplayTotalsMask();
	int GetEconomyEnabledView();
	int GetReplayTotalsLayout();
	int GetIncomeLayout();
	bool GetNewResourceBarsEnabled();
	int GetEconomyView();
	void SetEconomyView(int view);

  private:
	enum Page { Controls, Units, Economy, PageCount };
	std::array<std::vector<std::shared_ptr<Widget>>, PageCount> m_pages;
	int m_page = Controls;
	void BuildControls();
	void AddControl(Page page, const std::shared_ptr<Widget>& widget);
    void AddEnhancedControl(const std::shared_ptr<Widget>& widget);
    std::vector<std::shared_ptr<Widget>> m_enhancedControls;
	void SelectPage(int page);
	bool EditingField() const;
	void UpdateEconomyControls();
	// all widgets
	// (m_widgets vector is inherited from DialogBase)

	// those widgets that we need to refer to after construction
	std::shared_ptr <VirtualKeyField> m_clickSnapOverrideVirtualKeyField;
	std::shared_ptr <VirtualKeyField> m_autoClickVirtualKeyField;
	std::shared_ptr <VirtualKeyField> m_whiteboardVirtualKeyField;
	std::shared_ptr <VirtualKeyField> m_megaMapVirtualKeyField;
	std::shared_ptr <VirtualKeyField> m_rotateBuildVirtualKeyField;
	std::shared_ptr <Button>          m_buildMenuRotationOverlayButton;
#if ALLIED_BUILD_QUEUE_ENABLE
	std::shared_ptr <Button>          m_showAlliedBuildQueuesButton;
#endif
	std::shared_ptr <Button>          m_chatBackdropButton;
	std::shared_ptr <Button>          m_metalTotalsButton;
	std::shared_ptr <Button>          m_energyTotalsButton;
	std::shared_ptr <Button>          m_unitTotalsButton;
	std::shared_ptr <Button>          m_economyViewButton;
	std::shared_ptr <Button>          m_replayTotalsLayoutButton;
	std::shared_ptr <Button>          m_incomeLayoutButton;
	std::shared_ptr <Button>          m_resourceBarStyleButton;
	std::shared_ptr <IntegerField> m_mexSnapRadiusIntegerField;
	std::shared_ptr <IntegerField> m_wreckSnapRadiusIntegerField;
	std::shared_ptr <TextField> m_chatMacroTextField;
	std::shared_ptr <Button> m_resourceBarBackgroundButton;
	std::shared_ptr <Button> m_vsyncButton;
	std::shared_ptr <Button> m_optimiseDtRowsButton;
	std::shared_ptr <Button> m_enableFullRingsButton;
	std::shared_ptr <TabButton> m_okButton;
	std::shared_ptr <Button> m_conUnitPatrolHoldPosButton;
	std::shared_ptr <Button> m_conUnitPatrolManeuverButton;
	std::shared_ptr <Button> m_conUnitPatrolRoamButton;
	std::shared_ptr <Button> m_conUnitGuardHoldPosButton;
	std::shared_ptr <Button> m_conUnitGuardManeuverButton;
	std::shared_ptr <Button> m_conUnitGuardRoamButton;

	// Button skin surfaces (Dialog-specific; fonts/background/cursor are in DialogBase)
    LPDIRECTDRAWSURFACE lpMenuButton;
    LPDIRECTDRAWSURFACE lpPagePrevious, lpPageNext;
    LPDIRECTDRAWSURFACE lpStyleSelector, lpViewSelector;
    LPDIRECTDRAWSURFACE lpStagedButton3;
    LPDIRECTDRAWSURFACE lpCheckBox;
    LPDIRECTDRAWSURFACE lpStagedButton1;

	InlineSingleHook * EnterOption_hook;
	InlineSingleHook * PressInOption_hook;

	bool FirstBlit;

	void RestoreAll() override;
	void RenderDialog() override;
	void OnDragMoved() override;
	void OnMouseInsideDialog(int mx, int my) override;

	void ReadRegistry();
    void WriteRegistry();

    void ReadPos();
    void WritePos();
    void ReadSettings();
    void WriteSettings();

    void CorrectPos();
};


int __stdcall PressInOption (PInlineX86StackBuffer X86StrackBuffer);

int __stdcall EnterOption (PInlineX86StackBuffer X86StrackBuffer);

#endif
