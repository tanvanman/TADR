#include "IntegerField.h"
#include "../Dialog.h"

IntegerField::IntegerField(int x, int y, int width, int height, int value, int minValue, int maxValue, const std::string &registryKey) :
	Widget(x, y, width, height),
	m_value(value),
	m_defaultValue(value),
	m_minValue(minValue),
	m_maxValue(maxValue),
	m_registryKey(registryKey)
{
	if (m_value < m_minValue)
	{
		m_value = m_defaultValue;
	}
	if (m_value > m_maxValue)
	{
		m_value = m_defaultValue;
	}
}

bool IntegerField::DoMessage(Dialog* dialog, HWND winProchWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (m_focused && msg == WM_CHAR)
	{
		const char ch = wParam;
		if (ch == 8)
		{
			int new_value = m_value / 10;
			if (m_value >= m_minValue && new_value <= m_maxValue)
			{
				m_value = new_value;
			}
		}
		else if (ch == '-')
		{
			m_value--;
			if (m_value < m_minValue)
			{
				m_value = m_minValue;
			}
		}
		else if (ch == '+')
		{
			m_value++;
			if (m_value > m_maxValue)
			{
				m_value = m_maxValue;
			}
		}
		else if (ch == '=')
		{
			m_value = m_defaultValue;
		}
		else if (ch >= '0' && ch <= '9')
		{
			int new_value = m_value * 10 + ch - '0';
			if (new_value < m_minValue || new_value > m_maxValue)
			{
				new_value = ch - '0';
			}
			if (new_value >= m_minValue && new_value <= m_maxValue)
			{
				m_value = new_value;
			}
		}
		return true;
	}
	return 0;
}

std::string IntegerField::ToString()
{
	if (m_disabled)
	{
		return "NA";
	}
	else
	{
		return std::to_string(m_value);
	}
}

void IntegerField::DoDraw(Dialog* dialog)
{
	std::string text = this->ToString();
	dialog->DrawTextField(m_x, m_y, m_width, m_height, text.c_str(), m_focused ? 255 : 208);
}

void IntegerField::RegistryRead(HKEY hKey)
{
	int value;
	DWORD size = sizeof(value);
	if (RegQueryValueEx(hKey, m_registryKey.c_str(), NULL, NULL, (unsigned char*)&value, &size) == ERROR_SUCCESS)
	{
		if (value >= m_minValue && value <= m_maxValue)
		{
			m_value = value;
		}
	}
}

void IntegerField::RegistryWrite(HKEY hKey)
{
	RegSetValueEx(hKey, m_registryKey.c_str(), NULL, REG_DWORD, (unsigned char*)&m_value, sizeof(m_value));
}
