#include "TabButton.h"
#include "../DialogBase.h"
#include "../font.h"
#include "../tafunctions.h"

TabButton::TabButton(int x, int y, LPDIRECTDRAWSURFACE skin, const std::string& label,
    std::function<bool()> selected, std::function<void()> select)
    : Widget(x, y, 104, 39), m_skin(skin), m_label(label), m_selected(selected), m_select(select)
{
    DDSURFACEDESC desc = {};
    desc.dwSize = sizeof(desc);
    if (skin && skin->GetSurfaceDesc(&desc) == DD_OK)
    {
        m_width = desc.dwWidth / 2 + 8;
        m_height = desc.dwHeight + 8;
    }
}

bool TabButton::DoMessage(DialogBase* dialog, HWND, UINT message, WPARAM key, LPARAM point)
{
    bool inside = IsInside(static_cast<short>(LOWORD(point)) - dialog->posX,
        static_cast<short>(HIWORD(point)) - dialog->posY);
    if (message == WM_LBUTTONDOWN && inside) { m_pressed = true; return true; }
    if (message == WM_LBUTTONUP && m_pressed)
    {
        m_pressed = false;
        if (inside) { PlaySound_Effect("Options", 0); m_select(); }
        return true;
    }
    if (message == WM_KEYDOWN && m_focused && (key == VK_RETURN || key == VK_SPACE))
    { PlaySound_Effect("Options", 0); m_select(); return true; }
    return false;
}

void TabButton::DoDraw(DialogBase* dialog)
{
    bool selected = m_selected && m_selected();
    if (dialog->lpDialogSurf)
    {
        // Recessed bezel around the unscaled native F2 button face.
        const DWORD colours[] = { 90, 0, 44, 0 };
        for (int inset = 0; inset < 4; ++inset)
        {
            RECT edge = { m_x + inset, m_y + inset, m_x + m_width - inset, m_y + m_height - inset };
            DDBLTFX fill = {}; fill.dwSize = sizeof(fill); fill.dwFillColor = colours[inset];
            dialog->lpDialogSurf->Blt(&edge, nullptr, nullptr, DDBLT_WAIT | DDBLT_COLORFILL, &fill);
        }
    }
    if (m_skin)
        dialog->DrawTexture(m_x + 4, m_y + 4, m_width - 8, m_height - 8, m_skin,
            selected || m_pressed ? m_width - 8 : 0, 0);
    int textWidth = 0;
    for (unsigned char c : m_label)
    {
        if (c >= 33 && c <= 90) textWidth += FontOffsetUC[c - 33][0];
        else if (c >= 'a' && c <= 'z') textWidth += FontOffsetLC[c - 'a'][0];
        else if (c == ' ') textWidth += 7;
    }
    dialog->DrawText(dialog->lpDialogSurf, m_x + (m_width - textWidth) / 2,
        m_y + (m_height - 14) / 2, m_label.c_str());
}
