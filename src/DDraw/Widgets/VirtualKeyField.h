#pragma once

#include "Widget.h"

class VirtualKeyField : public Widget
{
public:
	VirtualKeyField(int x, int y, int width, int height, int vk_value, const std::string &registryKey);
	virtual ~VirtualKeyField() { }
	virtual bool DoMessage(Dialog* dialog, HWND winProchWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	virtual std::string ToString();
	virtual void DoDraw(Dialog*);
	virtual void RegistryRead(HKEY hKey);
	virtual void RegistryWrite(HKEY hKey);

	int m_vk;
	std::string m_registryKey;
};
