#ifndef dialogH
#define dialogH

#include "oddraw.h"

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

class Dialog
{
  friend class Widget;
  friend class Button;
  friend class Label;

public:
	Dialog(BOOL VidMem_a);
	~Dialog();
	void ShowDialog();
	void HideDialog();
	bool IsShow(LPRECT rect_p);
	void BlitDialog(LPDIRECTDRAWSURFACE DestSurf);
	bool Message(HWND WinProchWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

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

	int DrawTextField(int posX, int posY, int width, int height, const std::string& text, char color);
	void DrawText(LPDIRECTDRAWSURFACE DestSurf, int x, int y, const char* Text);
	void DrawSmallText(LPDIRECTDRAWSURFACE DestSurf, int x, int y, const char* Text);
	void DrawTexture(int x, int y, int width, int height, LPDIRECTDRAWSURFACE texture, int texturePosX, int texturePosY);
	void BlitCursor(LPDIRECTDRAWSURFACE DestSurf, int x, int y);

  private:
	// all widgets
	std::vector<std::shared_ptr<Widget> > m_widgets;

	// those widgets that we need to refer to after construction
	std::shared_ptr <VirtualKeyField> m_clickSnapOverrideVirtualKeyField;
	std::shared_ptr <VirtualKeyField> m_autoClickVirtualKeyField;
	std::shared_ptr <VirtualKeyField> m_whiteboardVirtualKeyField;
	std::shared_ptr <VirtualKeyField> m_megaMapVirtualKeyField;
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

    LPDIRECTDRAWSURFACE lpDialogSurf;
    LPDIRECTDRAWSURFACE lpBackground;
    LPDIRECTDRAWSURFACE lpCursor;
    LPDIRECTDRAWSURFACE lpOKButton;
    LPDIRECTDRAWSURFACE lpStagedButton3;
    LPDIRECTDRAWSURFACE lpCheckBox;
    LPDIRECTDRAWSURFACE lpStagedButton1;
    LPDIRECTDRAWSURFACE lpStandardButton;

    LPDIRECTDRAWSURFACE lpUCFont;
    LPDIRECTDRAWSURFACE lpLCFont;
    LPDIRECTDRAWSURFACE lpSmallUCFont;
    LPDIRECTDRAWSURFACE lpSmallLCFont;

	InlineSingleHook * EnterOption_hook;
	InlineSingleHook * PressInOption_hook;

	bool DialogVisible;
	bool VidMem;
	bool Move;
	int posX, posY;
	int posXPrev, posYPrev;
	int CursorPosX, CursorPosY;
	int CursorBackground;
    bool FirstBlit;

	// surface and pitch of current target of DrawTextField
    LPVOID SurfaceMemory;
    int lPitch;
	// helpers of DrawTextField
	void DrawTinyText(char* String, int posx, int posy, char Color);
	void FillRect(int x, int y, int x2, int y2, char Color);

	bool _Message(HWND WinProchWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    void RenderDialog();

	void ReadRegistry();
    void WriteRegistry();

    void ReadPos();
    void WritePos();
    void ReadSettings();
    void WriteSettings();

    void CorrectPos();
    void RestoreAll();

	void RestoreCursor ();
};


int __stdcall PressInOption (PInlineX86StackBuffer X86StrackBuffer);

int __stdcall EnterOption (PInlineX86StackBuffer X86StrackBuffer);

#endif