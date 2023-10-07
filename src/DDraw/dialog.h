#ifndef dialogH
#define dialogH


#define DialogHeight 234
#define DialogWidth 382

#define COL0 20
#define COL1 200
#define ROW(n) (30+n*35)

#define None 0

#define SetVisible 1
#define SetVisibleWidth 96
#define SetVisibleHeight 20
#define SetVisiblePosX 35
#define SetVisiblePosY 115

#define OKButton 2
#define OKButtonWidth 78
#define OKButtonHeight 20
#define OKButtonPosX 296
#define OKButtonPosY ROW(5)

//////////////// COL0

#define ClickSnapOverrideKeyId 3
#define ClickSnapOverrideKeyWidth 60
#define ClickSnapOverrideKeyHeight 15
#define ClickSnapOverrideKeyPosX COL0
#define ClickSnapOverrideKeyPosY ROW(0)

#define KeyCode 4
#define KeyCodeWidth 60
#define KeyCodeHeight 15
#define KeyCodePosX COL0
#define KeyCodePosY ROW(1)

#define WhiteboardKey 5
#define WhiteboardKeyWidth 60
#define WhiteboardKeyHeight 15
#define WhiteboardKeyPosX COL0
#define WhiteboardKeyPosY ROW(2)

#define MegaMapKey (6)
#define MegamapKeyWidth (60)
#define MegamapKeyHeight (15)
#define MegaMapKeyPosX (COL0)
#define MegaMapKeyPoxY ROW(3)

#define StagedButton3 7
#define StagedButton3Width 120
#define StagedButton3Height 20
#define StagedButton3PosX COL0
#define StagedButton3PosY ROW(4)

#define VSync 8
#define VSyncWidth 120
#define VSyncHeight 20
#define VSyncPosX COL0
#define VSyncPosY ROW(5)

/////////// COL1

#define ClickSnapRadiusId 13
#define ClickSnapRadiusWidth 28
#define ClickSnapRadiusHeight 15
#define ClickSnapRadiusPosX COL1
#define ClickSnapRadiusPosY ROW(0)

#define AutoClickDelay 12
#define AutoClickDelayWidth 28
#define AutoClickDelayHeight 15
#define AutoClickDelayPosX COL1
#define AutoClickDelayPosY ROW(0)

#define ShareBox 9
#define ShareBoxWidth 170
#define ShareBoxHeight 103
#define ShareBoxPosX COL1
#define ShareBoxPosY 60

#define OptimizeDT 10
#define OptimizeDTWidth 16
#define OptimizeDTHeight 16
#define OptimizeDTPosX COL1
#define OptimizeDTPosY 170

#define FullRings 11
#define FullRingsWidth 16
#define FullRingsHeight 16
#define FullRingsPosX COL1
#define FullRingsPosY 190

extern HINSTANCE HInstance;
struct tagInlineX86StackBuffer;
typedef struct tagInlineX86StackBuffer * PInlineX86StackBuffer;

class InlineSingleHook;
class Dialog
{
  private:
    bool DialogVisible;
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
    int posX;
    int posY;
    int CursorPosX;
    int CursorPosY;
	int CursorBackground;
    int X,Y;
    bool Move;
    bool First;
    LPVOID SurfaceMemory;
    int lPitch;
    bool OKButtonPushed;
    int StagedButton3State;
    bool StagedButton3Pushed;
    int StartedIn;

    bool ShareBoxFocus;
    char ShareText[1000];
    int ShareHeight;
    int MaxLines;
    int Lines;

    bool OptimizeDTEnabled;

    bool VSyncEnabled;
    bool VSyncPushed;
	bool VidMem;

    bool FullRingsEnabled;

    char AutoClickDelayText[10];
    bool AutoClickDelayFocus;

	char ClickSnapRadiusText[10];
	bool ClickSnapRadiusFocus;

    int VirtualKeyCode;
    bool KeyCodeFocus;

    int VirtualWhiteboardKey;
    bool WhiteboardKeyFocus;

	int ClickSnapOverrideKey;
	bool ClickSnapOverrideKeyFocus;

	int VirtualMegamap;
	bool MegmapFocus;

    bool SetVisiblePushed;
    
    bool Inside(int x, int y, int Control);
    void RenderDialog();
    void DrawTinyText(char *String, int posx, int posy, char Color);
    void FillRect(int x, int y, int x2, int y2, char Color);

    void DrawBackgroundButton();
    void DrawKeyCode();
    void DrawShareBox();
    void DrawOptimizeDT();
    void DrawVSync();
    void DrawFullRings();
	void DrawDelay();
	void DrawClickSnapRadius();
    void DrawWhiteboardKey();
	void DrawClickSnapOverrideKey();
    void DrawVisibleButton();

	void DrawMegaMapKey ();

    
    void ReadRegistry();
    void WriteRegistry();

    void ReadPos();
    void WritePos();
    void ReadSettings();
    void WriteSettings();

    void CorrectPos();
    void RestoreAll();

    void SetVisibleList();
	void RestoreCursor ();
  public:
    Dialog(BOOL VidMem_a);
    ~Dialog();
    void ShowDialog();
    void HideDialog();
	bool IsShow (LPRECT rect_p);
    void BlitDialog(LPDIRECTDRAWSURFACE DestSurf);
    bool Message(HWND WinProchWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	void SetAll();

    void DrawText(LPDIRECTDRAWSURFACE DestSurf, int x, int y, char *Text);
    void DrawSmallText(LPDIRECTDRAWSURFACE DestSurf, int x, int y, char *Text);
    void BlitCursor(LPDIRECTDRAWSURFACE DestSurf, int x, int y);
};


int __stdcall PressInOption (PInlineX86StackBuffer X86StrackBuffer);

int __stdcall EnterOption (PInlineX86StackBuffer X86StrackBuffer);

#endif