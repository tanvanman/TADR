#include "Label.h"
#include "../Dialog.h"

Label::Label(int x, int y, const std::string& text) :
	Widget(x, y, 0, 0),
	m_text(text)
{ }

Label::Label(int x, int y, int width, int height, const std::string& text) :
	Widget(x, y, width, height),
	m_text(text)
{ }

std::string Label::ToString()
{
	return m_text;
}

void Label::DoDraw(Dialog* dialog)
{
	dialog->DrawSmallText(dialog->lpDialogSurf, m_x, m_y + 5, m_text.c_str());
}