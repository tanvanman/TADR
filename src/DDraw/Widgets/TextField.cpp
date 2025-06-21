#include "TextField.h"
#include "../Dialog.h"

TextField::TextField(int x, int y, int width, int height, const std::string& text, const std::string &registryKey) :
	Label(x, y, width, height, text),
	m_maxLines((height - 5) / 10),
	m_currentLines(0),
	m_registryKey(registryKey)
{ }

bool TextField::DoMessage(Dialog* dialog, HWND winProchWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (m_focused && msg == WM_CHAR)
	{
		const TCHAR ch = wParam;
		if (ch == 8) // backspace
		{
			if (!m_text.empty())
			{
				m_text.pop_back();
			}
		}
		else if (m_currentLines <= m_maxLines)
		{
			m_text.push_back(ch);
		}
		return true;
	}
	return false;
}

std::string TextField::ToString()
{
	return m_text;
}

void TextField::DoDraw(Dialog* dialog)
{
	m_currentLines = dialog->DrawTextField(m_x, m_y, m_width, m_height, m_text, m_focused ? 255 : 208);
}

void TextField::RegistryRead(HKEY hKey)
{
	if (!m_registryKey.empty())
	{
		char buffer[1024];
		DWORD size = sizeof(buffer);
		if (RegQueryValueEx(hKey, m_registryKey.c_str(), NULL, NULL, (unsigned char*)&buffer, &size) == ERROR_SUCCESS)
		{
			m_text = buffer;
		}
	}
}

void TextField::RegistryWrite(HKEY hKey)
{
	if (!m_registryKey.empty())
	{
		RegSetValueEx(hKey, m_registryKey.c_str(), NULL, REG_SZ, (unsigned char*)m_text.c_str(), m_text.size());
	}
}
