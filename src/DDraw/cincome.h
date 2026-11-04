#ifndef cincomeH
#define cincomeH

#include "tamem.h"

#include <vector>
#include <memory>

class EconomyPanels;

// Legacy drawing uses logical pixels; dragging and cursors use screen pixels.
struct ResourceBarTransform
{
    static int Pixels(int value, int scale) { return value * scale / 100; }
    static int Fit(int requested, int width, int height, const RECT& viewport)
    {
        int result = requested;
        if (width > 0 && result > (viewport.right - viewport.left) * 100 / width)
            result = (viewport.right - viewport.left) * 100 / width;
        if (height > 0 && result > (viewport.bottom - viewport.top) * 100 / height)
            result = (viewport.bottom - viewport.top) * 100 / height;
        return result > 0 ? result : 1;
    }
    static POINT Logical(POINT point, int x, int y, int scale, int width = 210, int height = 300)
    {
        int dx = (point.x - x) * width, dy = (point.y - y) * height;
        int px = Pixels(width, scale), py = Pixels(height, scale);
        if (px < 1) px = 1;
        if (py < 1) py = 1;
        // Floor keeps points outside the left/top outside at enlarged scales.
        return { x + (dx >= 0 ? dx / px : -((-dx + px - 1) / px)),
            y + (dy >= 0 ? dy / py : -((-dy + py - 1) / py)) };
    }
};

class CIncome
{
  private:
    std::unique_ptr<EconomyPanels> m_economyPanels;
    bool m_usedNewBars = false;
    int m_legacyScale = 100;
    RECT m_legacyRect = {};
    POINT m_widgetLocal = {};
    void UpdateLegacyLayout();
    // Weather report bitmap cache: rebuild on input change (~1 Hz from the
    // clock tick), per-frame just RLE-blit the cached pixels.
    struct WeatherCacheKey
    {
        int   timeSeconds;
        int   wind;
        int   windMin;
        int   windMax;
        int   tidal;
        int   raceSide;
        void* fontHandle;
        int   watchMode;
        int   x1;
        int   x2;
        int   y1;
        int   y2;
        int   fontAlpha;
    };
    std::vector<unsigned char> m_weatherCache;
    int                        m_weatherCacheW;
    int                        m_weatherCacheH;
    int                        m_weatherCacheBlitX;
    int                        m_weatherCacheBlitY;
    unsigned char              m_weatherCacheAlpha;
    bool                       m_weatherCacheValid;
    WeatherCacheKey            m_weatherCacheKey;

    // RLE of opaque pixel runs in m_weatherCache; one memcpy per run
    // avoids the alpha-test-per-pixel branch.
    struct WeatherRun
    {
        short row;
        short startX;
        short length;
        short pad;
    };
    std::vector<WeatherRun>    m_weatherRuns;

    LPDIRECTDRAWSURFACE lpIncomeSurf;
    unsigned int BlitState;
    bool m_followPulseOn = false;
    int BackgroundType;

    int ShowAllyIncome();
    int ShowAllIncome();
    void SelectReplayPlayer(int slot);

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
    int m_replayPlayers[10] = {};
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
    bool IsPointOver(int x, int y);
    void ResetEconomyPositions();
};

#endif

