#pragma once

#include "Label.h"

class TextField : public Label
{
public:
	TextField(int x, int y, int width, int height, const std::string& text, const std::string &registryKey);
	virtual ~TextField() { }
	virtual bool DoMessage(Dialog *dialog, HWND winProchWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	virtual std::string ToString();
	virtual void DoDraw(Dialog*);
	virtual void RegistryRead(HKEY hKey);
	virtual void RegistryWrite(HKEY hKey);

	int m_maxLines;
	int m_currentLines;
	std::string m_registryKey;
};