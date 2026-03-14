#pragma once

#include <windows.h>
#include <winreg.h>

#include <string>

class DialogBase;

class Widget
{
public:
	Widget(int x, int y, int width, int height);
	virtual ~Widget() { }

	virtual bool Message(DialogBase* dialog, HWND winProchWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	virtual bool DoMessage(DialogBase* dialog, HWND winProchWnd, UINT msg, WPARAM wParam, LPARAM lParam) { return false; }

	virtual bool IsInside(int x, int y);
	virtual std::string ToString() = 0;
	virtual void Draw(DialogBase*);
	virtual void DoDraw(DialogBase*) = 0;
	virtual void RegistryRead(HKEY hKey) { }
	virtual void RegistryWrite(HKEY hKey) { }

	int m_x;
	int m_y;
	int m_width;
	int m_height;
	bool m_focused;
	bool m_disabled;
	bool m_hidden;
};
