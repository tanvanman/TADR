#ifndef dialogH
#define dialogH

#include "DialogBase.h"

#include <memory>
#include <string>
#include <vector>

struct tagInlineX86StackBuffer;
typedef struct tagInlineX86StackBuffer * PInlineX86StackBuffer;

class InlineSingleHook;
class VirtualKeyField;
class IntegerField;
class TextField;
class Button;
class Widget;

#define ROW_HEIGHT 18
#define ROW(n) (n*ROW_HEIGHT + ROW_HEIGHT/2 - 1)
#define DialogHeight (14*ROW_HEIGHT)
#define DialogWidth 382

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

  private:
	// all widgets
	// (m_widgets vector is inherited from DialogBase)

	// those widgets that we need to refer to after construction
	std::shared_ptr <VirtualKeyField> m_clickSnapOverrideVirtualKeyField;
	std::shared_ptr <VirtualKeyField> m_autoClickVirtualKeyField;
	std::shared_ptr <VirtualKeyField> m_whiteboardVirtualKeyField;
	std::shared_ptr <VirtualKeyField> m_megaMapVirtualKeyField;
	std::shared_ptr <VirtualKeyField> m_rotateBuildVirtualKeyField;
	std::shared_ptr <IntegerField> m_mexSnapRadiusIntegerField;
	std::shared_ptr <IntegerField> m_wreckSnapRadiusIntegerField;
	std::shared_ptr <TextField> m_chatMacroTextField;
	std::shared_ptr <Button> m_resourceBarBackgroundButton;
	std::shared_ptr <Button> m_vsyncButton;
	std::shared_ptr <Button> m_optimiseDtRowsButton;
	std::shared_ptr <Button> m_enableFullRingsButton;
	std::shared_ptr <Button> m_okButton;
	std::shared_ptr <Button> m_conUnitPatrolHoldPosButton;
	std::shared_ptr <Button> m_conUnitPatrolManeuverButton;
	std::shared_ptr <Button> m_conUnitPatrolRoamButton;
	std::shared_ptr <Button> m_conUnitGuardHoldPosButton;
	std::shared_ptr <Button> m_conUnitGuardManeuverButton;
	std::shared_ptr <Button> m_conUnitGuardRoamButton;

	// Button skin surfaces (Dialog-specific; fonts/background/cursor are in DialogBase)
    LPDIRECTDRAWSURFACE lpOKButton;
    LPDIRECTDRAWSURFACE lpStagedButton3;
    LPDIRECTDRAWSURFACE lpCheckBox;
    LPDIRECTDRAWSURFACE lpStagedButton1;
    LPDIRECTDRAWSURFACE lpStandardButton;

	InlineSingleHook * EnterOption_hook;
	InlineSingleHook * PressInOption_hook;

	bool FirstBlit;

	void RestoreAll() override;
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
