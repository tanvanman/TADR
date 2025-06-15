#include "VirtualKeyField.h"
#include "../Dialog.h"
#include "../hook/etc.h"	// vtkToStr

VirtualKeyField::VirtualKeyField(int x, int y, int width, int height, int vk_value, const std::string &registryKey) :
	Widget(x, y, width, height),
	m_vk(vk_value),
	m_registryKey(registryKey)
{ }

bool VirtualKeyField::DoMessage(Dialog* dialog, HWND winProchWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (m_focused && msg == WM_KEYDOWN)
	{
		m_vk = wParam;
		return true;
	}
	else if (m_focused && msg == WM_CHAR)
	{
		return true;
	}
	return false;
}

std::string VirtualKeyField::ToString()
{
	char buffer[32];
	vkToStr(m_vk, buffer, sizeof(buffer));
	return buffer;
}

void VirtualKeyField::DoDraw(Dialog* dialog)
{
	std::string text = this->ToString();
	dialog->DrawTextField(m_x, m_y, m_width, m_height, text.c_str(), m_focused ? 255 : 208);
}

void VirtualKeyField::RegistryRead(HKEY hKey)
{
	int value;
	DWORD size = sizeof(value);
	if (RegQueryValueEx(hKey, m_registryKey.c_str(), NULL, NULL, (unsigned char*)&value, &size) == ERROR_SUCCESS)
	{
		m_vk = value;
	}
}

void VirtualKeyField::RegistryWrite(HKEY hKey)
{
	RegSetValueEx(hKey, m_registryKey.c_str(), NULL, REG_DWORD, (unsigned char*)&m_vk, sizeof(m_vk));
}