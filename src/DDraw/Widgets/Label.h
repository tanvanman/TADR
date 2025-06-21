#pragma once

#include "Widget.h"

class Label : public Widget
{
public:
	Label(int x, int y, const std::string& text);
	Label(int x, int y, int width, int height, const std::string& text);
	virtual ~Label() { }
	virtual std::string ToString();
	virtual void DoDraw(Dialog*);
	std::string m_text;
};
