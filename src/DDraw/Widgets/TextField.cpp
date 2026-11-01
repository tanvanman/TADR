#include "TextField.h"
#include "../DialogBase.h"

TextField::TextField(int x, int y, int width, int height, const std::string& text, const std::string &registryKey) :
	Label(x, y, width, height, text),
	m_maxLines((height - 5) / 10),
	m_currentLines(0),
	m_registryKey(registryKey)
{ }

bool TextField::DoMessage(DialogBase* dialog, HWND winProchWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

void TextField::DoDraw(DialogBase* dialog)
{
	m_currentLines = dialog->DrawTextField(m_x, m_y, m_width, m_height, m_text, m_focused ? 255 : 208);
}

void TextField::RegistryRead(HKEY hKey)
{
	if (!m_registryKey.empty())
	{
		char buffer[1024];
		DWORD size = sizeof(buffer);
		DWORD type = REG_NONE;
		if (RegQueryValueEx(hKey, m_registryKey.c_str(), NULL, &type,
			reinterpret_cast<BYTE*>(buffer), &size) == ERROR_SUCCESS && type == REG_SZ)
		{
			// Legacy writers omitted the NUL. Never scan beyond returned bytes.
			const char* end = static_cast<const char*>(memchr(buffer, 0, size));
			m_text.assign(buffer, end ? end - buffer : size);
		}
	}
}

void TextField::RegistryWrite(HKEY hKey)
{
	if (!m_registryKey.empty())
	{
		RegSetValueEx(hKey, m_registryKey.c_str(), NULL, REG_SZ, (unsigned char*)m_text.c_str(), m_text.size() + 1);
	}
}
