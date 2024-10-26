#ifndef cincomeH
#define cincomeH

#include "tamem.h"

class CIncome
{
  private:
    LPDIRECTDRAWSURFACE lpIncomeSurf;
    unsigned int BlitState;
    int BackgroundType;

    int ShowAllyIncome();
    int ShowAllIncome();

    void PaintStoragebar(int posx, int posy, int Player, int Type);
    LPVOID SurfaceMemory;
    int lPitch;
	void DrawText(char* String, int posx, int posy, char Color);
	void ShowMyViewIncome (int posx, int posy);
    void ShowPlayerIncome(int Player, int posx, int posy);
    int DrawStorageText(float Storage, int posx, int posy);
    void DrawPlayerRect(int posx, int posy, char Color);
	int DrawMinimiseWidget(int posx, int posy, char Color);
	int DrawMinimiseWidgetTooltip(int posx, int posy, char Color);
    void FillRect(char Color);
    bool First;
    int posX;
    int posY;
	int CursorX;
	int CursorY;
    void ReadPos();
    void WritePos();
    void CorrectPos();

    bool StartedInRect;
	bool Minimised;
	int MinimiseWidgetBoxMin[2];
	int MinimiseWidgetBoxMax[2];
  public:
    CIncome(BOOL VidMem);
    ~CIncome();
    void BlitIncome(LPDIRECTDRAWSURFACE DestSurf);
	void BlitWeatherReport(LPVOID lpSurfaceMem, int dwWidth, int dwHeight, int lPitch);
	void Set(int BGType);
    bool Message(HWND WinProchWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	void _DrawText(char* String, int posx, int posy, char Color);
	int GetMinimiseWidgetXPos();

	bool IsShow (RECT * Rect_p);
};

#endif

