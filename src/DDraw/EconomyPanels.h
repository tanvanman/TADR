#pragma once

#include "oddraw.h"
#include "tamem.h"
#include <array>
#include <vector>

// ProCast-inspired economy display by TAG_Venom. Owns display state only;
// the legacy CIncome renderer and Recorder shared-memory ABI remain separate.
class EconomyPanels
{
public:
    EconomyPanels();
    ~EconomyPanels();
    bool UseNewBars() const;
    void Blit(LPDIRECTDRAWSURFACE destination, const RECT* legacyBounds = nullptr);
    bool Message(UINT message, WPARAM wParam, LPARAM lParam);
    bool Contains(int x, int y) const;
    void SetBackgroundType(int type) { m_backgroundType = type >= 0 && type <= 2 ? type : 1; }
    void ResetPositions();

private:
    static const int Width = 184;
    static const int ColumnGap = 4;
    static const int PanelGap = 3;
    static const int MaxWidth = Width * 3 + ColumnGap * 2;
    static const int MaxHeight = 600;
    static const int TotalsGroupGap = 3;
    struct Panel
    {
        LPDIRECTDRAWSURFACE surface = nullptr;
        RECT rect = {};
        int x = -1, y = -1, width = Width, height = 0, scale = 100;
        int page = 0, pages = 1, rows = 1, columns = 1;
        int collapsedX = -1, collapsedY = -1;
        bool hidden = false, visible = false, right = true, moved = false;
        std::vector<unsigned char> pixels;
    };
    struct Player
    {
        int slot;
        char name[30];
        char side[5];
        unsigned char colour, textColour;
        bool haveRates;
        double stored[2], capacity[2], made[2], spent[2], total[3];
    };
    struct RateSample
    {
        bool seeded = false, ready = false;
        int tick = 0, dpid = 0, interval = 30;
        char name[30] = {};
        double counters[4] = {}, rates[4] = {};
    };
    std::array<RateSample, 10> m_rates;
    Panel m_bars, m_totals;
    std::vector<Player> m_players;
    std::vector<Player> m_allPlayers;
    std::array<std::vector<Player>, 3> m_ranked;
    Panel* m_drag = nullptr;
    POINT m_mouse = {};
    int m_backgroundType = 1;
    int m_totalsMask = 7;
    int m_barRowHeight = 42, m_totalsRowHeight = 12;
    enum DisplayMode { Hidden = 0, Incomes = 1, Totals = 2, Both = 3 };
    DisplayMode m_displayMode = Both;
    int m_enabledView = -1, m_requestedView = -1, m_totalsColumns = 1;
    int m_incomeLayout = -1, m_totalsLayout = -1;
    bool m_layoutReplay = false, m_sideBySide = false;
    bool m_wheelDockActive = false;
    RECT m_wheelDock = {};
    const unsigned char* m_hudFont = nullptr;
    int m_fontTop = 0, m_fontHeight = 8;
    DWORD m_sampleTime = 0;
    int m_gameTime = -1;
    bool m_replay = false;
    bool m_pressed = false, m_tabClick = false;
    bool m_usingNewBars = true;
    RECT m_legacyBounds = {};
    unsigned char m_background, m_track, m_text, m_green, m_red, m_metal, m_energy;

    void Sample();
    void Layout();
    void PublishView();
    void DrawBars();
    void DrawTotals();
    void DrawPanel(Panel& panel);
    int FooterTop(const Panel& panel) const;
    void BlitPanel(Panel& panel, LPDIRECTDRAWSURFACE destination);
    void Text(Panel& panel, int x, int y, const char* text, unsigned char colour, int maxChars = 25, int maxWidth = Width);
    int TextWidth(const char* text, int maxChars = 25) const;
    void SelectFont();
    void Box(Panel& panel, int x, int y, int width, int height, unsigned char colour);
    void Bar(Panel& panel, int x, int y, int width, double value, double maximum, unsigned char colour, int thickness = 3);
    static int Scaled(const Panel& panel, int value) { return value * panel.scale / 100; }
    void ViewPlayer(int slot);
};
