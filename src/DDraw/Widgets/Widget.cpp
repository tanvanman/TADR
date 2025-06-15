#include "Widget.h"

Widget::Widget(int x, int y, int width, int height) :
	m_x(x),
	m_y(y),
	m_width(width),
	m_height(height),
	m_focused(false),
	m_disabled(false),
	m_hidden(false)
{ }

bool Widget::Message(Dialog *dialog, HWND winProchWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

void Widget::Draw(Dialog* dialog)
{
	if (!m_hidden)
	{
		this->DoDraw(dialog);
	}
}
