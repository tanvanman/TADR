#include "config.h"
#include "EconomyPanels.h"
#include "iddrawsurface.h"
#include "dialog.h"
#include "font.h"
#include "tafunctions.h"
#include "TenPlayerReplay.h"
#include "ResourceBarAi.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <climits>

extern PALETTEENTRY TAPalette[];

namespace
{
    unsigned char PaletteColour(int red, int green, int blue)
    {
        int best = 0, distance = INT_MAX;
        // Index 1 is the legacy transparency key; never use it for this UI.
        for (int i = 0; i < 256; ++i)
        {
            if (i == 1) continue;
            int r = TAPalette[i].peRed - red, g = TAPalette[i].peGreen - green, b = TAPalette[i].peBlue - blue;
            int d = r*r + g*g + b*b;
            if (d < distance) { best = i; distance = d; }
        }
        return static_cast<unsigned char>(best);
    }

    double ResourceValue(double value)
    {
        return std::isfinite(value) && value > 0.0 ? value : 0.0;
    }

    // At most five characters, leaving room for a sign in each rate column.
    // Truncate suffix values so 999,999 cannot round up to a six-character 1000k.
    void Number(char (&text)[16], double value)
    {
        value = ResourceValue(value);
        if (value >= 10.0 && value < 10000.0)
        {
            sprintf_s(text, "%.0f", std::floor(value));
            return;
        }
        const char* suffix[] = { "", "k", "M", "G", "T" };
        int unit = 0;
        while (value >= 1000.0 && unit < 4) { value /= 1000.0; ++unit; }
        if (value >= 1000.0) { strcpy_s(text, ">999T"); return; }
        if (value < 100.0 && value != std::floor(value))
            sprintf_s(text, "%.1f%s", std::floor(value * 10.0) / 10.0, suffix[unit]);
        else
            sprintf_s(text, "%.0f%s", std::floor(value), suffix[unit]);
    }

    bool Observer()
    {
        int original = LocalShare->OrgLocalPlayerID;
        auto ta = *TAmainStruct_PtrPtr;
        return DataShare->PlayingDemo || (original >= 0 && original < 10 &&
            ta->Players[original].PlayerInfo && (ta->Players[original].PlayerInfo->PropertyMask & WATCH));
    }

    bool Inside(const RECT& rect, int x, int y)
    {
        return x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom;
    }
}

EconomyPanels::EconomyPanels()
{
    // Closest TA palette equivalents of the ProCast reference colours.
    m_background = PaletteColour(15, 23, 29);
    m_track = 0;
    m_text = PaletteColour(255, 255, 255);
    m_green = PaletteColour(110, 236, 101);
    m_red = PaletteColour(255, 0, 0);
    m_metal = PaletteColour(139, 209, 241);
    m_energy = PaletteColour(242, 232, 0);
}

EconomyPanels::~EconomyPanels()
{
    if (m_bars.surface) m_bars.surface->Release();
    if (m_totals.surface) m_totals.surface->Release();
}

bool EconomyPanels::UseNewBars() const
{
    if (!MODERN_ALLY_RESOURCE_UI_ENABLE) return false;
    auto dialog = static_cast<Dialog*>(LocalShare->Dialog);
    return !dialog || dialog->GetNewResourceBarsEnabled();
}

bool EconomyPanels::Contains(int x, int y) const
{
    if (!UseNewBars()) return false;
    return (m_bars.visible && Inside(m_bars.rect, x, y)) ||
        (DataShare->PlayingDemo && m_totals.visible && Inside(m_totals.rect, x, y));
}

void EconomyPanels::Sample()
{
    auto ta = *TAmainStruct_PtrPtr;
    m_players.clear();
    m_allPlayers.clear();
    int original = LocalShare->OrgLocalPlayerID;
    bool watch = Observer();
    for (int slot = 0; slot < 10; ++slot)
    {
        const auto& player = ta->Players[slot];
        // Retain defeated players with an occupied slot for historical totals.
        // TenPlayerReplay changes the original spectator's controller to remote
        // human when that slot carries the tenth player's packets. Its WATCH
        // flag can remain set; ordinary local/remote spectators stay excluded.
        bool funnel = TenPlayerReplay_HasFunnelEconomySample(&player);
        if (slot == original && watch && !funnel) continue;
        if (!player.PlayerInfo || !player.Name[0] || player.My_PlayerType == Player_none ||
            ((player.PlayerInfo->PropertyMask & WATCH) && !funnel)) continue;
        Player entry = {};
        entry.slot = slot;
        memcpy(entry.name, player.Name, sizeof(entry.name) - 1);
        int side = static_cast<unsigned char>(player.PlayerInfo->RaceSide);
        if (side < static_cast<int>(ta->RaceCounter) && ta->RaceSideDataAry)
            memcpy(entry.side, ta->RaceSideDataAry[side].name, sizeof(entry.side) - 1);
        entry.colour = GetPlayerDotColor(slot);
        // Same logo -> configured PlayerNDotColors mapping as ProCast's release
        // UI assets, including +logo/custom INI colours. ProCast lightens only
        // the native dark-blue text for legibility; the identity swatch stays
        // the actual team colour. No colours depend on slot or ranking order.
        const auto& colour = TAPalette[entry.colour];
        entry.textColour = colour.peRed == 23 && colour.peGreen == 71 && colour.peBlue == 231
            ? PaletteColour(83, 119, 239) : entry.colour;
        const auto& res = player.PlayerRes;
        entry.stored[0] = ResourceValue(res.fCurrentMetal);
        entry.stored[1] = ResourceValue(res.fCurrentEnergy);
        entry.capacity[0] = ResourceValue(res.fMaxMetalStorage);
        entry.capacity[1] = ResourceValue(res.fMaxEnergyStorage);
        // Remote 0x28 packets update cumulative counters, not the four HUD rate
        // fields. Despite their names, the consumed counters accumulate demand
        // before settlement, just like TA/ProCast's negative HUD values. They
        // are not accepted spending during a stall. Use these recorded counters
        // consistently for every controller, without touching the simulation.
        double counters[] = { ResourceValue(res.fTotalMetalProduced), ResourceValue(res.fTotalEnergyProduced),
            ResourceValue(res.fTotalMetalConsumed), ResourceValue(res.fTotalEnergyConsumed) };
        auto& sample = m_rates[slot];
        bool reset = !sample.seeded || sample.dpid != player.DirectPlayID ||
            strncmp(sample.name, entry.name, sizeof(sample.name)) != 0 || ta->GameTime < sample.tick;
        for (int i = 0; i < 4; ++i) reset = reset || counters[i] < sample.counters[i];
        if (reset)
        {
            sample = {};
            sample.seeded = true;
            sample.dpid = player.DirectPlayID;
            memcpy(sample.name, entry.name, sizeof(sample.name));
            memcpy(sample.counters, counters, sizeof(counters));
            sample.tick = ta->GameTime;
        }
        else
        {
            int ticks = ta->GameTime - sample.tick;
            bool changed = false;
            for (int i = 0; i < 4; ++i) changed = changed || counters[i] != sample.counters[i];
            // An unchanged render sample is not a fresh zero-activity packet.
            // Hold the last rate until counters advance; never move its time
            // baseline forward during a gap between recorded updates.
            if (ticks > 0 && changed)
            {
                for (int i = 0; i < 4; ++i) sample.rates[i] = (counters[i] - sample.counters[i]) * 30.0 / ticks;
                memcpy(sample.counters, counters, sizeof(counters));
                sample.tick = ta->GameTime;
                sample.interval = ticks;
                sample.ready = true;
            }
        }
        // A long gap is unknown, rather than measured zero. Allow at least ten
        // simulation seconds, or four observed update intervals, and retain
        // rates while paused. The next real update uses the complete interval.
        entry.haveRates = sample.ready && ta->GameTime - sample.tick <= std::max(300, sample.interval * 4);
        entry.made[0] = sample.rates[0]; entry.made[1] = sample.rates[1];
        entry.spent[0] = sample.rates[2]; entry.spent[1] = sample.rates[3];
        if (!player.PlayerActive)
        {
            entry.haveRates = true;
            entry.made[0] = entry.made[1] = entry.spent[0] = entry.spent[1] = 0;
        }
        entry.total[0] = ResourceValue(res.fTotalMetalProduced);
        entry.total[1] = ResourceValue(res.fTotalEnergyProduced);
        entry.total[2] = std::max(0, static_cast<int>(player.UnitsNumber));
        m_allPlayers.push_back(entry);
        if (watch || (original >= 0 && original < 10 && slot != original &&
            ta->Players[original].AllyFlagAry[slot] && player.AllyFlagAry[original]))
            m_players.push_back(entry);
    }
    for (int metric = 0; metric < 3; ++metric)
    {
        m_ranked[metric] = m_allPlayers;
        std::stable_sort(m_ranked[metric].begin(), m_ranked[metric].end(),
            [metric](const Player& a, const Player& b) { return a.total[metric] > b.total[metric]; });
    }
}

void EconomyPanels::Layout()
{
    SelectFont();
    auto dialog = static_cast<Dialog*>(LocalShare->Dialog);
    RECT bounds = (*TAmainStruct_PtrPtr)->GameSreen_Rect;
    int width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
    bool replay = DataShare->PlayingDemo != 0;
    m_totalsMask = dialog ? (dialog->GetReplayTotalsMask() & 7) : 7;
    int groups = (m_totalsMask & 1 ? 1 : 0) + (m_totalsMask & 2 ? 1 : 0) + (m_totalsMask & 4 ? 1 : 0);
    int enabled = dialog ? (dialog->GetEconomyEnabledView() & 3) : Both;
    if (!replay) enabled &= Incomes;
    int requested = dialog ? dialog->GetEconomyView() : Both;
    // Totals cannot be displayed in live play or when all categories are off.
    // Normalise the live selector to the effective view instead of leaving it
    // claiming Totals/Both while only incomes are on screen.
    if (!(enabled & Totals) && (requested == Totals || requested == Both)) requested = Incomes;
    if (m_requestedView != requested || m_enabledView != enabled || m_layoutReplay != replay)
    {
        m_displayMode = static_cast<DisplayMode>(requested);
        m_requestedView = requested;
        m_bars.hidden = m_totals.hidden = false;
        m_bars.collapsedX = m_totals.collapsedX = m_bars.collapsedY = m_totals.collapsedY = -1;
        m_enabledView = enabled;
        m_layoutReplay = replay;
        m_drag = nullptr;
        m_wheelDockActive = false;
    }
    PublishView();
    bool totalsAvailable = UseNewBars() && replay && (enabled & Totals) && groups > 0 && !m_allPlayers.empty();
    DisplayMode mode = static_cast<DisplayMode>(m_displayMode & enabled);
    m_bars.visible = UseNewBars() && (mode & Incomes) && !m_players.empty();
    m_totals.visible = totalsAvailable && (mode & Totals);
    if (width <= 0 || height <= 0)
    {
        m_bars.visible = m_totals.visible = false;
        return;
    }
    // Fixed native pixels: saved scale preferences are deliberately ignored.
    m_bars.scale = m_totals.scale = 100;
    Panel* panels[] = { &m_bars, &m_totals };
    int gap = m_bars.visible && m_totals.visible ? PanelGap : 0;
    int footer = m_fontHeight + 2;
    int cardHeight = (m_fontHeight + 3) + 2 * (m_fontHeight + 5) + 1;
    int headingHeight = m_fontHeight + 1;
    m_barRowHeight = cardHeight + 2;
    m_totalsRowHeight = m_fontHeight + 6;
    int barCount = static_cast<int>(m_players.size());
    int totalCount = static_cast<int>(m_allPlayers.size());
    int incomeLayout = dialog ? dialog->GetIncomeLayout() : 0;
    int totalsLayout = dialog ? dialog->GetReplayTotalsLayout() : 1;
    if (m_incomeLayout != incomeLayout || m_totalsLayout != totalsLayout)
    {
        m_wheelDockActive = false;
        m_incomeLayout = incomeLayout; m_totalsLayout = totalsLayout;
    }
    int maxBarColumns = std::max(1, std::min(3, std::min(std::max(1, barCount), (width + ColumnGap) / (Width + ColumnGap))));
    m_bars.columns = incomeLayout ? maxBarColumns : 1;
    // Prefer complete rows (ten players become a 2x5 grid), otherwise wrap
    // at the available native-width columns rather than a single long strip.
    if (incomeLayout) for (int columns = maxBarColumns; columns >= 2; --columns)
        if (barCount % columns == 0) { m_bars.columns = columns; break; }
    int fullBars = m_bars.hidden ? 10 : footer + m_barRowHeight * ((barCount + m_bars.columns - 1) / m_bars.columns);
    auto totalsHeight = [&](int columns, int rows) {
        int blocks = (groups + columns - 1) / columns;
        return m_totals.hidden ? 10 : blocks * (headingHeight + TotalsGroupGap + m_totalsRowHeight * rows);
    };
    int maxColumns = std::max(1, std::min(groups, (width + ColumnGap) / (Width + ColumnGap)));
    m_totalsColumns = totalsLayout == 2 ? maxColumns : 1;
    bool wheelDock = m_wheelDockActive && (mode == Incomes || mode == Totals);
    m_bars.width = m_bars.columns * Width + (m_bars.columns - 1) * ColumnGap;
    m_totals.width = m_totalsColumns * Width + (m_totalsColumns - 1) * ColumnGap;
    int fullTotals = totalsHeight(m_totalsColumns, totalCount);
    m_sideBySide = m_bars.visible && m_totals.visible && !m_bars.hidden && !m_totals.hidden &&
        fullBars + fullTotals + gap > height && fullBars <= std::min(height, MaxHeight) &&
        fullTotals <= std::min(height, MaxHeight) && width >= m_bars.width + gap + m_totals.width;
    int blocks = (groups + m_totalsColumns - 1) / m_totalsColumns;
    int overhead = (headingHeight + TotalsGroupGap) * blocks;
    // Keep complete rankings whenever possible. Only an exceptionally small
    // viewport pages totals; ordinary 900/1080 layouts show all ten players.
    int totalsBudget = height;
    if (m_bars.visible && !m_sideBySide)
        totalsBudget -= (m_bars.hidden ? 10 : footer + m_barRowHeight) + gap;
    if (height >= fullBars + fullTotals + gap) totalsBudget = fullTotals;
    int rowHeight = m_totalsRowHeight * blocks;
    m_totals.rows = std::max(1, (std::min(totalsBudget, MaxHeight) - overhead - footer) / std::max(1, rowHeight));
    if (fullTotals <= std::min(totalsBudget, MaxHeight)) m_totals.rows = std::max(1, totalCount);
    m_totals.rows = std::min(m_totals.rows, std::max(1, totalCount));
    m_totals.height = m_totals.hidden ? 10 : overhead + rowHeight * m_totals.rows + (m_totals.rows < totalCount ? footer : 0);
    int barsBudget = m_sideBySide ? height : height - (m_totals.visible ? m_totals.height + gap : 0);
    m_bars.rows = std::max(1, (std::min(barsBudget, MaxHeight) - footer) / m_barRowHeight * m_bars.columns);
    m_bars.rows = std::min(m_bars.rows, std::max(1, barCount));
    m_bars.height = m_bars.hidden ? 10 : footer + m_barRowHeight * ((m_bars.rows + m_bars.columns - 1) / m_bars.columns);
    for (auto panel : panels)
    {
        int count = static_cast<int>(panel == &m_bars ? m_players.size() : m_allPlayers.size());
        panel->pages = std::max(1, (count + panel->rows - 1) / panel->rows);
        panel->page = std::max(0, std::min(panel->page, panel->pages - 1));
        if (!panel->visible) { panel->rect = {}; continue; }
        int panelWidth = std::min(width, panel->hidden ? 10 : panel->width);
        int panelHeight = std::min(height, Scaled(*panel, panel->height));
        if (wheelDock && !panel->hidden)
        {
            panel->height = std::min(std::min(height, MaxHeight), std::max(fullBars, fullTotals));
            panel->width = std::max(m_bars.columns * Width + (m_bars.columns - 1) * ColumnGap,
                m_totalsColumns * Width + (m_totalsColumns - 1) * ColumnGap);
            int x = std::max(static_cast<int>(bounds.left), std::min(static_cast<int>(m_wheelDock.left), static_cast<int>(bounds.right) - panel->width));
            int y = std::max(static_cast<int>(bounds.top), std::min(static_cast<int>(m_wheelDock.top), static_cast<int>(bounds.bottom) - panel->height));
            m_wheelDock = { x, y, x + std::min(width, panel->width), y + panel->height };
            panel->rect = m_wheelDock;
            continue;
        }
        // Incomes belong at the top right and totals at the bottom right.
        // Totals can span several columns. Short displays can place their
        // vertical panel to the left of incomes without hiding lower ranks.
        // Preserve positions the viewer has explicitly dragged.
        if (!panel->moved || panel->x < 0)
            panel->x = bounds.right - panel->width - (m_sideBySide && panel == &m_totals ? m_bars.width + gap : 0);
        if (!panel->moved || panel->y < 0)
            panel->y = panel == &m_totals ? bounds.bottom - panelHeight : bounds.top;
        // Keep a collapsed tab beside its former heading, rather than
        // jumping to the bottom edge when the panel becomes ten pixels tall.
        if (panel->hidden && panel->collapsedY >= 0) panel->y = panel->collapsedY;
        panel->x = std::max(static_cast<int>(bounds.left), std::min(panel->x, static_cast<int>(bounds.right) - Scaled(*panel, panel->width)));
        panel->y = std::max(static_cast<int>(bounds.top), std::min(panel->y, static_cast<int>(bounds.bottom) - panelHeight));
        int x = panel->hidden ? (panel->collapsedX >= 0
            ? std::max(static_cast<int>(bounds.left), std::min(panel->collapsedX, static_cast<int>(bounds.right) - panelWidth))
            : (panel->right ? bounds.right - panelWidth : bounds.left)) : panel->x;
        panel->rect = { x, panel->y, x + panelWidth, panel->y + panelHeight };
    }
    auto intersects = [](const RECT& a, const RECT& b) {
        return a.right > b.left && a.left < b.right && a.bottom > b.top && a.top < b.bottom;
    };
    if (m_bars.visible && m_totals.visible && !m_bars.hidden && !m_totals.hidden && intersects(m_bars.rect,m_totals.rect))
    {
        if (m_sideBySide)
        {
            Panel& anchor = m_drag ? *m_drag : (m_bars.moved && !m_totals.moved ? m_bars : m_totals);
            Panel& other = &anchor == &m_bars ? m_totals : m_bars;
            int x = anchor.rect.left - other.width - gap;
            if (x < bounds.left) x = anchor.rect.right + gap;
            if (x + other.width <= bounds.right)
            { other.x = x; other.rect.left = x; other.rect.right = x + other.width; }
            else
            {
                m_bars.x = bounds.right - m_bars.width;
                m_totals.x = m_bars.x - gap - m_totals.width;
                m_bars.rect.left = m_bars.x; m_bars.rect.right = m_bars.x + m_bars.width;
                m_totals.rect.left = m_totals.x; m_totals.rect.right = m_totals.x + m_totals.width;
            }
        }
        else
        {
            m_bars.y = std::max(static_cast<int>(bounds.top), static_cast<int>(m_totals.rect.top) - m_bars.height - gap);
            m_bars.rect.top = m_bars.y; m_bars.rect.bottom = m_bars.y + m_bars.height;
            if (m_bars.rect.bottom + gap > m_totals.rect.top && m_bars.rect.bottom + gap + m_totals.height <= bounds.bottom)
            {
                m_totals.y = m_bars.rect.bottom + gap;
                m_totals.rect.top = m_totals.y; m_totals.rect.bottom = m_totals.y + m_totals.height;
            }
        }
    }
    // Keep each collapsed edge tab accessible behind neither panel nor tab.
    for (auto panel : panels)
    {
        auto other = panel == &m_bars ? &m_totals : &m_bars;
        if (!panel->visible || !panel->hidden || !other->visible || !intersects(panel->rect, other->rect)) continue;
        int y = other->rect.bottom + 2;
        if (y + 10 > bounds.bottom) y = other->rect.top - 12;
        if (y >= bounds.top && y + 10 <= bounds.bottom)
        { panel->rect.top = y; panel->rect.bottom = y + 10; }
        else
        {
            panel->right = !panel->right;
            panel->rect.left = panel->right ? bounds.right - 10 : bounds.left;
            panel->rect.right = panel->rect.left + 10;
        }
    }

}

void EconomyPanels::PublishView()
{
    auto dialog = static_cast<Dialog*>(LocalShare->Dialog);
    if (dialog) dialog->SetEconomyView(m_displayMode);
    m_requestedView = m_displayMode;
}

void EconomyPanels::ResetPositions()
{
    m_drag = nullptr;
    m_wheelDockActive = false;
    m_pressed = m_tabClick = false;
    // Reapply enabled panels without changing their saved checkboxes. This also
    // restores a view hidden through F4, or a panel collapsed with its arrow.
    m_enabledView = -1;
    auto dialog = static_cast<Dialog*>(LocalShare->Dialog);
    int available = dialog ? dialog->GetEconomyEnabledView() : Both;
    if (!DataShare->PlayingDemo) available &= Incomes;
    m_displayMode = static_cast<DisplayMode>(available);
    PublishView();
    for (auto panel : { &m_bars, &m_totals })
    {
        panel->x = panel->y = panel->collapsedX = panel->collapsedY = -1;
        panel->moved = panel->hidden = false;
        panel->page = 0;
    }
}

void EconomyPanels::Box(Panel& panel, int x, int y, int width, int height, unsigned char colour)
{
    int endX = std::min(panel.width, x + width), endY = std::min(panel.height, y + height);
    for (int row = std::max(0, y); row < endY; ++row)
        for (int col = std::max(0, x); col < endX; ++col)
            panel.pixels[row * panel.width + col] = colour;
}

// Reuse the loaded side FNT used by TA's top resource HUD. Only globally
// blank rows in the displayed alphabet are trimmed; glyph pixels and variable
// advances stay native.
void EconomyPanels::SelectFont()
{
    m_hudFont = nullptr; m_fontTop = 0; m_fontHeight = 8;
    auto ta = *TAmainStruct_PtrPtr;
    int slot = ta->LocalHumanPlayer_PlayerID;
    if (slot < 0 || slot >= 10 || !ta->Players[slot].PlayerInfo || !ta->RaceCounter) return;
    int side = static_cast<unsigned char>(ta->Players[slot].PlayerInfo->RaceSide);
    if (side >= std::min(static_cast<int>(ta->RaceCounter), static_cast<int>(_countof(ta->RaceSideDataAry)))) side = 0;
    const auto* font = reinterpret_cast<const unsigned char*>(ta->RaceSideDataAry[side].Font_File);
    if (!font || font[0] == 0 || font[0] > 32) return;
    int top = font[0], bottom = -1;
    auto include = [&](unsigned code)
    {
        if (code < font[3]) return;
        unsigned offset = font[4 + (code - font[3]) * 2] | (font[5 + (code - font[3]) * 2] << 8);
        if (!offset || font[offset] > 64) return;
        int width = font[offset];
        for (int row = 0; row < font[0]; ++row)
            for (int col = 0; col < width; ++col)
            {
                int bit = row * width + col;
                if (font[offset + 1 + bit / 8] & (128 >> (bit % 8)))
                { top = std::min(top, row); bottom = std::max(bottom, row); }
            }
    };
    const char* labels = "0123456789+-.kMGT/><Total metal producedTotal energy producedTotal current unitsMy Original Camera wheelME";
    for (const unsigned char* c = reinterpret_cast<const unsigned char*>(labels); *c; ++c) include(*c);
    // Accented player names may need additional rows. Grow spacing and page
    // rather than clipping their accents or scaling any of the font's pixels.
    for (const auto& player : m_allPlayers)
    {
        for (const unsigned char* c = reinterpret_cast<const unsigned char*>(player.name); *c; ++c) include(*c);
        for (const unsigned char* c = reinterpret_cast<const unsigned char*>(player.side); *c; ++c) include(*c);
    }
    if (bottom < top) return;
    m_hudFont = font; m_fontTop = top; m_fontHeight = bottom - top + 1;
    if (ta->desktopGUI.RadarObjecColor[10] != 1 && ta->desktopGUI.RadarObjecColor[12] != 1)
    {
        m_green = ta->desktopGUI.RadarObjecColor[10];
        m_red = ta->desktopGUI.RadarObjecColor[12];
    }
}

int EconomyPanels::TextWidth(const char* text, int maxChars) const
{
    int width = 0;
    for (int i = 0; text[i] && i < maxChars; ++i)
    {
        unsigned code = static_cast<unsigned char>(text[i]);
        if (!m_hudFont) { width += 8; continue; }
        if (code < m_hudFont[3]) continue;
        unsigned offset = m_hudFont[4 + (code - m_hudFont[3]) * 2] | (m_hudFont[5 + (code - m_hudFont[3]) * 2] << 8);
        if (offset && m_hudFont[offset] <= 64) width += m_hudFont[offset];
    }
    return width;
}

void EconomyPanels::Text(Panel& panel, int x, int y, const char* text, unsigned char colour, int maxChars, int maxWidth)
{
    int count = std::min(maxChars, static_cast<int>(strlen(text)));
    maxWidth = std::max(0, std::min(maxWidth, panel.width - x));
    while (count && TextWidth(text, count) > maxWidth) --count;
    if (!count) return;
    int width = TextWidth(text, count);
    if (m_backgroundType == 1) Box(panel, x - 1, y - 1, width + 2, m_fontHeight + 2, 0);
    auto glyphs = [&](bool outline)
    {
        int pen = x;
        for (int i = 0; i < count; ++i)
        {
            unsigned code = static_cast<unsigned char>(text[i]), offset = 0;
            int advance = 8;
            if (m_hudFont)
            {
                if (code < m_hudFont[3]) continue;
                offset = m_hudFont[4 + (code - m_hudFont[3]) * 2] | (m_hudFont[5 + (code - m_hudFont[3]) * 2] << 8);
                if (!offset || m_hudFont[offset] > 64) continue;
                advance = m_hudFont[offset];
            }
            for (int row = 0; row < m_fontHeight; ++row)
                for (int col = 0; col < advance; ++col)
                {
                    int bit = (row + m_fontTop) * advance + col;
                    bool on = m_hudFont ? (m_hudFont[offset + 1 + bit / 8] & (128 >> (bit % 8))) != 0
                        : (static_cast<unsigned char>(ThinFont[code * 8 + row]) & (128 >> col)) != 0;
                    if (on)
                    {
                        if (outline) Box(panel, pen + col - 1, y + row - 1, 3, 3, 0);
                        else Box(panel, pen + col, y + row, 1, 1, colour);
                    }
                }
            pen += advance;
        }
    };
    if (m_backgroundType == 0) glyphs(true);
    glyphs(false);
}

void EconomyPanels::Bar(Panel& panel, int x, int y, int width, double value, double maximum, unsigned char colour, int thickness)
{
    Box(panel, x - 1, y - 1, width + 2, thickness + 2, 0);
    Box(panel, x, y, width, thickness, m_track);
    double fraction = maximum > 0 ? std::min(1.0, ResourceValue(value) / maximum) : 0;
    Box(panel, x, y, static_cast<int>(fraction * width), thickness, colour);
}

int EconomyPanels::FooterTop(const Panel& panel) const
{
    // Wheel switching reserves the taller view's hit area. Keep the income
    // footer attached to its actual rows rather than to that empty padding.
    return &panel == &m_bars ? m_barRowHeight * ((panel.rows + panel.columns - 1) / panel.columns) : panel.height - m_fontHeight;
}

void EconomyPanels::DrawPanel(Panel& panel)
{
    panel.pixels.assign(panel.width * panel.height, m_backgroundType == 2 ? m_background : 1);
    bool left = panel.right == panel.hidden;
    int arrowX = panel.hidden ? 0 : panel.width - 10;
    for (int col = 0; col < 9; ++col)
    {
        int rows = left ? col + 1 : 9 - col;
        Box(panel, arrowX + col, (9 - rows) / 2, 1, rows, m_energy);
    }
    if (!panel.hidden)
    {
        char footer[32];
        if (panel.pages > 1) sprintf_s(footer, "<  %d/%d  >", panel.page + 1, panel.pages);
        else strcpy_s(footer, &panel == &m_bars && Observer() ? "My Original Camera" : "");
        if (footer[0]) Text(panel, 4, FooterTop(panel), footer, m_energy);
    }
}

void EconomyPanels::DrawBars()
{
    DrawPanel(m_bars);
    if (m_bars.hidden) return;
    int first = m_bars.page * m_bars.rows;
    int resourceLine = m_fontHeight + 5;
    int nameLine = m_fontHeight + 3;
    for (int row = 0; row < m_bars.rows && first + row < static_cast<int>(m_players.size()); ++row)
    {
        const Player& player = m_players[first + row];
        int x = (row % m_bars.columns) * (Width + ColumnGap);
        int y = (row / m_bars.columns) * m_barRowHeight;
        int sideX = Width - 14 - TextWidth(player.side, 4);
        Box(m_bars, x + 4, y, 10, 10, player.colour);
        Text(m_bars, x + 18, y, player.name, m_text, 18, sideX - 22);
        Text(m_bars, x + sideX, y, player.side, m_text, 4);
        Box(m_bars, x + 4, y + m_fontHeight + 1, Width - 8, 1, player.textColour);
        if (ReplayCameraFollowSlot() == player.slot ||
            (DataShare->LosViewOn == player.slot && IsWatchingOtherPlayerPov()))
            Box(m_bars, x, y, 2, m_barRowHeight - 2, player.textColour);
        for (int resource = 0; resource < 2; ++resource)
        {
            int line = y + nameLine + resourceLine * resource;
            char stored[16], capacity[16], made[16], spent[16], value[32];
            Number(stored, player.stored[resource]); Number(capacity, player.capacity[resource]);
            Number(made, player.made[resource]); Number(spent, player.spent[resource]);
            unsigned char colour = resource == 0 ? m_metal : m_energy;
            Text(m_bars, x + 4, line, resource == 0 ? "M" : "E", colour);
            sprintf_s(value, "%s/%s", stored, capacity);
            Text(m_bars, x + 18, line, value, colour, 11, 74);
            if (player.haveRates) sprintf_s(value, "+%s", made); else strcpy_s(value, "--");
            Text(m_bars, x + 96, line, value, m_green, 6, 40);
            if (player.haveRates) sprintf_s(value, "-%s", spent); else strcpy_s(value, "--");
            Text(m_bars, x + 140, line, value, m_red, 6, 40);
        }
    }
    // Paint bars last so the next row's text backdrop/outline cannot erase
    // their final pixel. No glyph ink overlaps the outlined resource bars.
    for (int row = 0; row < m_bars.rows && first + row < static_cast<int>(m_players.size()); ++row)
        for (int resource = 0; resource < 2; ++resource)
        {
            const Player& player = m_players[first + row];
            int x = (row % m_bars.columns) * (Width + ColumnGap);
            int y = (row / m_bars.columns) * m_barRowHeight + nameLine + resourceLine * resource + m_fontHeight + 1;
            Bar(m_bars, x + 4, y, Width - 8, player.stored[resource], player.capacity[resource], resource == 0 ? m_metal : m_energy);
        }
}

void EconomyPanels::DrawTotals()
{
    DrawPanel(m_totals);
    if (m_totals.hidden) return;
    const char* titles[] = { "Total metal produced", "Total energy produced", "Total current units" };
    int ordinal = 0;
    int blockHeight = m_fontHeight + 1 + m_totalsRowHeight * m_totals.rows + TotalsGroupGap;
    for (int metric = 0; metric < 3; ++metric)
    {
        if (!(m_totalsMask & (1 << metric))) continue;
        int x = (ordinal % m_totalsColumns) * (Width + ColumnGap);
        int y = (ordinal / m_totalsColumns) * blockHeight;
        ++ordinal;
        Text(m_totals, x + 4, y, titles[metric], m_text);
        int first = m_totals.page * m_totals.rows;
        for (int row = 0; row < m_totals.rows && first + row < static_cast<int>(m_ranked[metric].size()); ++row)
        {
            const Player& player = m_ranked[metric][first + row];
            int line = y + m_fontHeight + 1 + row * m_totalsRowHeight;
            char value[16]; Number(value, player.total[metric]);
            int valueX = x + Width - 4 - TextWidth(value);
            Box(m_totals, x + 4, line + 1, 8, 8, player.colour);
            Text(m_totals, x + 16, line, player.name, m_text, 18, valueX - x - 20);
            Text(m_totals, valueX, line, value, metric < 2 ? m_green : m_text);
        }
    }
    // Resource colours identify metrics; the enlarged swatches identify players.
    ordinal = 0;
    for (int metric = 0; metric < 3; ++metric)
    {
        if (!(m_totalsMask & (1 << metric))) continue;
        int x = (ordinal % m_totalsColumns) * (Width + ColumnGap);
        int y = (ordinal / m_totalsColumns) * blockHeight;
        ++ordinal;
        int first = m_totals.page * m_totals.rows;
        double maximum = m_ranked[metric].empty() ? 0 : m_ranked[metric][0].total[metric];
        for (int row = 0; row < m_totals.rows && first + row < static_cast<int>(m_ranked[metric].size()); ++row)
        {
            const Player& player = m_ranked[metric][first + row];
            int line = y + m_fontHeight + 1 + row * m_totalsRowHeight + m_fontHeight + 1;
            unsigned char colour = metric == 0 ? m_metal : metric == 1 ? m_energy : m_text;
            Bar(m_totals, x + 4, line, Width - 8, player.total[metric], maximum, colour, 3);
        }
    }
}

void EconomyPanels::BlitPanel(Panel& panel, LPDIRECTDRAWSURFACE destination)
{
    if (!panel.visible) return;
    if (!panel.surface)
    {
        DDSURFACEDESC desc = {};
        desc.dwSize = sizeof(desc);
        desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
        desc.dwWidth = MaxWidth; desc.dwHeight = MaxHeight;
        if (static_cast<LPDIRECTDRAW>(LocalShare->TADirectDraw)->CreateSurface(&desc, &panel.surface, nullptr) != DD_OK) return;
    }
    if (panel.surface->IsLost() != DD_OK && panel.surface->Restore() != DD_OK) return;
    DDSURFACEDESC lock = {};
    lock.dwSize = sizeof(lock);
    if (panel.surface->Lock(nullptr, &lock, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, nullptr) != DD_OK) return;
    for (int row = 0; row < panel.height; ++row)
        memcpy(static_cast<unsigned char*>(lock.lpSurface) + row * lock.lPitch, panel.pixels.data() + row * panel.width, panel.width);
    panel.surface->Unlock(nullptr);
    RECT source = { 0, 0, panel.rect.right - panel.rect.left, panel.height };
    // Clip source and destination equally; never resample the font.
    if (panel.rect.bottom - panel.rect.top < Scaled(panel, panel.height))
        source.bottom = std::min(panel.height, static_cast<int>(panel.rect.bottom - panel.rect.top) * 100 / panel.scale);
    if (source.bottom > 0)
    {
        DDBLTFX effects = {};
        effects.dwSize = sizeof(effects);
        effects.ddckSrcColorkey.dwColorSpaceLowValue = effects.ddckSrcColorkey.dwColorSpaceHighValue = 1;
        destination->Blt(&panel.rect, panel.surface, &source, DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &effects);
    }
}

void EconomyPanels::Blit(LPDIRECTDRAWSURFACE destination, const RECT* legacyBounds)
{
    if (!UseNewBars())
    {
        m_bars.visible = m_totals.visible = false;
        m_enabledView = -1;
        m_drag = nullptr;
        m_wheelDockActive = false;
        return;
    }
    if (!MODERN_ALLY_RESOURCE_UI_ENABLE || DataShare->TAProgress != TAInGame ||
        GetCurrentThreadId() != LocalShare->GuiThreadId) return;
    auto ta = *TAmainStruct_PtrPtr;
    m_legacyBounds = legacyBounds ? *legacyBounds : RECT{};
    if (!ta->GameingState_Ptr || !ResourceBarsGameState(ta->GameingState_Ptr->State))
    {
        m_bars.visible = m_totals.visible = false;
        return;
    }
    DWORD now = GetTickCount();
    if (m_gameTime < 0 || ta->GameTime < m_gameTime || m_replay != (DataShare->PlayingDemo != 0))
    {
        m_bars.page = m_totals.page = 0;
        m_drag = nullptr;
        m_sampleTime = now - 250;
    }
    m_gameTime = ta->GameTime;
    m_replay = DataShare->PlayingDemo != 0;
    if (m_allPlayers.empty() || now - m_sampleTime >= 250) { Sample(); m_sampleTime = now; }
    Layout();
    if (m_bars.visible) DrawBars();
    if (m_totals.visible) DrawTotals();
    BlitPanel(m_bars, destination);
    BlitPanel(m_totals, destination);
    auto dialog = static_cast<Dialog*>(LocalShare->Dialog);
    if (dialog && ((m_bars.visible && Inside(m_bars.rect, m_mouse.x, m_mouse.y)) ||
        (m_totals.visible && Inside(m_totals.rect, m_mouse.x, m_mouse.y))))
        dialog->BlitCursor(destination, m_mouse.x, m_mouse.y);
}

void EconomyPanels::ViewPlayer(int slot)
{
    if (!Observer()) return;
    auto ta = *TAmainStruct_PtrPtr;
    const int following = ReplayCameraFollowSlot();
    if (slot < 0)
    {
        SetReplayCameraFollowSlot(-1);
        DataShare->LosViewOn = 10;
    }
    else if (slot == following)
    {
        SetReplayCameraFollowSlot(-1);
        return;
    }
    else if (slot == DataShare->LosViewOn && IsWatchingOtherPlayerPov())
    {
        SetReplayCameraFollowSlot(slot);
        return;
    }
    else
    {
        DataShare->LosViewOn = slot;
        if (following >= 0) SetReplayCameraFollowSlot(slot);
    }
    DeselectUnits(); UpdateSelectUnitEffect(); ApplySelectUnitMenu_Wapper();
    const bool control = slot >= 0 ? bool(DataShare->PlayingDemo) :
        LocalShare->OrgLocalPlayerID != ta->LocalHumanPlayer_PlayerID;
    ViewPlayerLos_Replay(slot < 0 ? 0 : slot, control, slot < 0);
}

bool EconomyPanels::Message(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!UseNewBars())
    {
        m_bars.visible = m_totals.visible = false;
        m_enabledView = -1;
        m_drag = nullptr;
        m_wheelDockActive = false;
        return false;
    }
    if (DataShare->TAProgress != TAInGame)
    {
        // Apply the enabled panels again after leaving a match;
        // seeking within the current replay keeps temporary F4 choices.
        m_enabledView = -1;
        m_bars.visible = m_totals.visible = false;
        m_drag = nullptr;
        return false;
    }
    auto ta = *TAmainStruct_PtrPtr;
    if (!ta->GameingState_Ptr || !ResourceBarsGameState(ta->GameingState_Ptr->State)) return false;
    Layout();
    if (m_drag && !m_drag->visible) m_drag = nullptr;
    if (message == WM_LBUTTONDBLCLK && m_tabClick)
    {
        m_tabClick = false;
        m_pressed = true;
        return true;
    }
    if (message == WM_LBUTTONDOWN) m_tabClick = false;
    if (message == WM_KEYDOWN && wParam == VK_F4 &&
        (!m_players.empty() || m_totals.visible))
    {
        // Ignore key-repeat so one press advances exactly one display state.
        if (lParam & (1L << 30)) return true;
        // A collapsed panel is a temporary hide, not another F4 view. The
        // first press restores it in place; subsequent presses cycle views.
        if ((m_bars.visible && m_bars.hidden) || (m_totals.visible && m_totals.hidden))
        {
            m_bars.hidden = m_totals.hidden = false;
            m_bars.collapsedX = m_totals.collapsedX = m_bars.collapsedY = m_totals.collapsedY = -1;
            m_drag = nullptr;
            m_wheelDockActive = false;
            Layout();
            return true;
        }
        auto dialog = static_cast<Dialog*>(LocalShare->Dialog);
        int enabled = dialog ? (dialog->GetEconomyEnabledView() & 3) : Both;
        if (!DataShare->PlayingDemo) enabled &= Incomes;
        if (m_allPlayers.empty()) enabled &= ~Totals;
        if (m_players.empty()) enabled &= ~Incomes;
        if (!enabled) return false;
        DisplayMode mode = static_cast<DisplayMode>(m_displayMode & enabled);
        bool scoreboard = (ta->GameOptionMask & 0x80) != 0;
        bool passToTA = false;
        if (scoreboard)
        {
            if (mode != Hidden) { m_displayMode = Hidden; passToTA = true; }
            else if (enabled & Incomes) m_displayMode = Incomes;
            else { m_displayMode = static_cast<DisplayMode>(enabled); passToTA = true; }
            // Stock TA leaves incomes selected when its scoreboard closes.
            if (passToTA && (enabled & Incomes)) m_displayMode = Incomes;
        }
        else if (mode == Incomes) m_displayMode = enabled & Totals ? Totals : Hidden;
        else if (mode == Totals) m_displayMode = enabled == Both ? Both : Hidden;
        else if (mode == Both) m_displayMode = Hidden;
        else passToTA = true; // let TA turn its scoreboard on
        m_bars.hidden = m_totals.hidden = false;
        m_drag = nullptr;
        m_wheelDockActive = false;
        PublishView();
        Layout();
        return !passToTA;
    }
    // Wheel coordinates are screen-relative; all other mouse messages are client-relative.
    POINT point = { static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)) };
    if (message == WM_MOUSEWHEEL) ScreenToClient((*TAProgramStruct_PtrPtr)->TAClass_Hwnd, &point);
    if (message == WM_CAPTURECHANGED || message == WM_CANCELMODE)
    { m_drag = nullptr; m_pressed = m_tabClick = false; return false; }
    if (message == WM_MOUSEMOVE && m_drag)
    {
        if (!(wParam & MK_LBUTTON)) { m_drag = nullptr; return false; }
        if (m_wheelDockActive)
        {
            m_wheelDock.left += point.x - m_mouse.x;
            m_wheelDock.top += point.y - m_mouse.y;
        }
        else
        {
            m_drag->moved = true;
            m_drag->x += point.x - m_mouse.x;
            m_drag->y += point.y - m_mouse.y;
        }
        m_mouse = point;
        m_drag->right = point.x >= ((*TAmainStruct_PtrPtr)->GameSreen_Rect.left + (*TAmainStruct_PtrPtr)->GameSreen_Rect.right) / 2;
        Layout();
        return true;
    }
    if (message == WM_LBUTTONUP && (m_drag || m_pressed))
    { m_drag = nullptr; m_pressed = false; return true; }
    if (message == WM_MOUSEMOVE) m_mouse = point;
    Panel* panels[] = { &m_totals, &m_bars };
    for (auto panel : panels)
    {
        if (!panel->visible || !Inside(panel->rect, point.x, point.y)) continue;
        int x = (point.x - panel->rect.left) * 100 / panel->scale;
        int y = (point.y - panel->rect.top) * 100 / panel->scale;
        if (message == WM_MOUSEWHEEL)
        {
            // Both already shows each view. Consume wheel input over either
            // panel without replacing it or changing its current page.
            if (m_displayMode == Both || m_displayMode == Hidden) return true;
            if (m_enabledView == Both && m_totalsMask && DataShare->PlayingDemo &&
                static_cast<Dialog*>(LocalShare->Dialog)->GetReplayTotalsEnabled() && !m_allPlayers.empty())
            {
                // Keep the replacement under the pointer. Both single views
                // share this temporary dock until F4, reset or a preference change.
                if (!m_wheelDockActive)
                {
                    m_wheelDock = panel->rect;
                    m_wheelDockActive = true;
                }
                m_displayMode = GET_WHEEL_DELTA_WPARAM(wParam) < 0 ? Totals : Incomes;
                m_bars.hidden = m_totals.hidden = false;
                m_drag = nullptr;
                PublishView();
                Layout();
            }
            return true;
        }
        if (message == WM_LBUTTONDOWN)
        {
            m_pressed = true;
            if (panel->hidden || (x >= panel->width - 10 && y < 10))
            {
                panel->collapsedX = panel->hidden ? -1 : panel->rect.right - 10;
                panel->collapsedY = panel->hidden ? -1 : panel->rect.top;
                panel->hidden = !panel->hidden;
                m_tabClick = true;
            }
            else if (y >= FooterTop(*panel) - 2 && y < FooterTop(*panel) + m_fontHeight && panel->pages > 1)
                panel->page = (panel->page + (x < panel->width / 2 ? panel->pages - 1 : 1)) % panel->pages;
            else { m_drag = panel; m_mouse = point; }
            Layout();
            return true;
        }
        if (message == WM_LBUTTONDBLCLK && panel == &m_bars && !panel->hidden)
        {
            if (y >= FooterTop(*panel) - 2 && y < FooterTop(*panel) + m_fontHeight && panel->pages == 1) ViewPlayer(-1);
            else if (y < FooterTop(*panel))
            {
                int column = x / (Width + ColumnGap);
                if (column >= panel->columns || x % (Width + ColumnGap) >= Width) return true;
                int cell = (y / m_barRowHeight) * panel->columns + column;
                if (cell >= panel->rows) return true;
                int row = panel->page * panel->rows + cell;
                if (row < static_cast<int>(m_players.size())) ViewPlayer(m_players[row].slot);
            }
            return true;
        }
        return message == WM_MOUSEMOVE || message == WM_LBUTTONUP || message == WM_LBUTTONDBLCLK;
    }
    return false;
}
