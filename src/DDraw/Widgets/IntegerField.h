#pragma once

#include "Widget.h"

class IntegerField : public Widget
{
public:
	IntegerField(int x, int y, int width, int height, int value, int minValue, int maxValue, const std::string &registryKey);
	virtual ~IntegerField() { }
	virtual bool DoMessage(Dialog* dialog, HWND winProchWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	virtual std::string ToString();
	virtual void DoDraw(Dialog*);
	virtual void RegistryRead(HKEY hKey);
	virtual void RegistryWrite(HKEY hKey);

	int m_value;
	int m_defaultValue;
	int m_minValue;
	int m_maxValue;
	std::string m_registryKey;
};
