#ifndef dialogH
#define dialogH


#define DialogHeight 234
#define DialogWidth 382

#define None 0
#define OKButton 1
#define OKButtonWidth 78
#define OKButtonHeight 20
#define OKButtonPosX 296
#define OKButtonPosY 207
#define StagedButton3 2
#define StagedButton3Width 120
#define StagedButton3Height 20
#define StagedButton3PosX 20
#define StagedButton3PosY 30
#define KeyCode 3
#define KeyCodeWidth 60
#define KeyCodeHeight 15
#define KeyCodePosX 20
#define KeyCodePosY 200
#define ShareBox 4
#define ShareBoxWidth 170
#define ShareBoxHeight 103
#define ShareBoxPosX 200
#define ShareBoxPosY 60
#define OptimizeDT 5
#define OptimizeDTWidth 16
#define OptimizeDTHeight 16
#define OptimizeDTPosX 200
#define OptimizeDTPosY 170
#define VSync 6
#define VSyncWidth 120
#define VSyncHeight 20
#define VSyncPosX 20
#define VSyncPosY 66
#define FullRings 7
#define FullRingsWidth 16
#define FullRingsHeight 16
#define FullRingsPosX 200
#define FullRingsPosY 190
#define AutoClickDelay 8
#define AutoClickDelayWidth 28
#define AutoClickDelayHeight 15
#define AutoClickDelayPosX 200
#define AutoClickDelayPosY 30
#define MexSnapRadiusId 12
#define MexSnapRadiusWidth 28
#define MexSnapRadiusHeight 15
#define MexSnapRadiusPosX 200
#define MexSnapRadiusPosY 30
#define WhiteboardKey 9
#define WhiteboardKeyWidth 60
#define WhiteboardKeyHeight 15
#define WhiteboardKeyPosX 20
#define WhiteboardKeyPosY 168
#define SetVisible 10
#define SetVisibleWidth 96
#define SetVisibleHeight 20
#define SetVisiblePosX 35
#define SetVisiblePosY 115

#define MegaMapKey (11)
#define MegaMapKeyPosX (20)
#define MegaMapKeyPoxY (137)
#define MegamapKeyWidth (60)
#define MegamapKeyHeight (15)


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

	char MexSnapRadiusText[10];
	bool MexSnapRadiusFocus;

    int VirtualKeyCode;
    bool KeyCodeFocus;

    int VirtualWhiteboardKey;
    bool WhiteboardKeyFocus;

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
	void DrawMexSnapRadius();
    void DrawWhiteboardKey();
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