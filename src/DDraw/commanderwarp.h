#ifndef commanderwarpH
#define commanderwarpH


#define DISABLED 0
#define ENABLED 1
#define CLIENTDONE 2
#define ALLDONE 3

#define ButtonHeight 20
#define ButtonWidth 96
#define ButtonXPos (LocalShare->ScreenWidth-300)
#define ButtonYPos 130

class CWarp
{
  private:
    void SetPos(int x, int y);
    int *MapX;
    int *MapY;
    LPDIRECTDRAWSURFACE lpButton;
    bool WithinButton(int x, int y);
    bool ButtonDown;
    bool StartedinButton;
    void BlitBtn(LPDIRECTDRAWSURFACE DestSurf);
    short yPos;
    short xPos;
  public:
    CWarp(BOOL VidMem);
    ~CWarp();
    bool Message(HWND WinProcWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    void Blit(LPDIRECTDRAWSURFACE DestSurf);
};

#endif
