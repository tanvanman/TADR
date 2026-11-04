#include "Widget.h"
#include "../DialogBase.h"
#include "../font.h"

Widget::Widget(int x, int y, int width, int height) :
	m_x(x),
	m_y(y),
	m_width(width),
	m_height(height),
	m_focused(false),
	m_disabled(false),
	m_hidden(false)
{ }

bool Widget::Message(DialogBase *dialog, HWND winProchWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (m_hidden || m_disabled)
	{
		return false;
	}

	return DoMessage(dialog, winProchWnd, msg, wParam, lParam);
}

bool Widget::IsInside(int x, int y)
{
	return !m_disabled && !m_hidden && x >= m_x && x < m_x + m_width && y >= m_y && y < m_y + m_height;
}

void Widget::Draw(DialogBase* dialog)
{
	if (!m_hidden)
	{
		this->DoDraw(dialog);
        if (m_dimmed)
        {
            int width = m_width, y = m_y - 1, height = m_height + 2;
            if (!width)
            {
                for (unsigned char c : ToString())
                {
                    if (c >= 33 && c <= 90) width += SmallFontOffsetUC[c - 33][0];
                    else if (c >= 'a' && c <= 'z') width += SmallFontOffsetLC[c - 'a'][0];
                    else if (c == ' ') width += 6;
                }
                y = m_y + 5; height = 12;
            }
            dialog->DimRect(m_x, y, width, height);
        }
	}
}
