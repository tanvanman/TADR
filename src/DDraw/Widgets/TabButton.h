#pragma once

#include "Widget.h"
#include "../oddraw.h"
#include <functional>

// Page/action button using TA's original 96x31 F2 menu skin.
class TabButton : public Widget
{
public:
    TabButton(int x, int y, LPDIRECTDRAWSURFACE skin, const std::string& label,
        std::function<bool()> selected, std::function<void()> select);
    bool DoMessage(DialogBase*, HWND, UINT, WPARAM, LPARAM) override;
    void DoDraw(DialogBase*) override;
    std::string ToString() override { return m_label; }
    void CancelInteraction() override { Widget::CancelInteraction(); m_pressed = false; }

private:
    LPDIRECTDRAWSURFACE m_skin;
    std::string m_label;
    std::function<bool()> m_selected;
    std::function<void()> m_select;
    bool m_pressed = false;
};
