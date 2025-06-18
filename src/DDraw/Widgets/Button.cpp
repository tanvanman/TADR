#include "Button.h"
#include "../Dialog.h"

Button::Button(int x, int y, LPDIRECTDRAWSURFACE skin, int initialState, int numStates, bool depressable,
	const std::vector<std::string>& stateLabels, const std::string& registryKey, std::function<void(int)> onStateChange) :
	Widget(x, y, 16, 16),
	m_stateLabels(stateLabels),
	m_numStates(numStates),
	m_state(initialState),
	m_depressed(false),
	m_onStateChange(onStateChange),
	m_skin(skin),
	m_skinWidth(0),
	m_registryKey(registryKey)
{
	DDSURFACEDESC surfaceDesc;
	ZeroMemory(&surfaceDesc, sizeof(surfaceDesc));
	surfaceDesc.dwSize = sizeof(surfaceDesc);
	if (skin->GetSurfaceDesc(&surfaceDesc) == ERROR_SUCCESS)
	{
		m_skinWidth = surfaceDesc.dwWidth;
		m_width = surfaceDesc.dwWidth / (m_numStates + depressable);
		m_height = surfaceDesc.dwHeight;
	}
}

bool Button::DoMessage(Dialog* dialog, HWND winProchWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	bool mouseInside = IsInside(LOWORD(lParam) - dialog->posX, HIWORD(lParam) - dialog->posY);
	if (msg == WM_LBUTTONDOWN)
	{
		if (mouseInside)
		{
			m_depressed = true;
			return true;
		}
	}
	else if (msg == WM_LBUTTONUP)
	{
		if (m_depressed)
		{
			m_depressed = false;
			if (mouseInside)
			{
				m_state = (m_state + 1) % m_numStates;
				if (m_onStateChange)
				{
					m_onStateChange(m_state);
				}
				return true;
			}
		}
	}
	return false;
}

std::string Button::ToString()
{
	if (unsigned(m_state) < m_stateLabels.size())
	{
		return m_stateLabels[m_state];
	}
	else
	{
		return std::to_string(m_state);
	}
}

void Button::DoDraw(Dialog* dialog)
{
	int xofs = m_skinWidth;

	if (m_depressed || m_disabled)
	{
		xofs = m_numStates * m_width;
	}
	if (xofs > m_skinWidth - m_width)
	{
		xofs = m_state * m_width;
	}
	if (xofs <= m_skinWidth - m_width)
	{
		int dy = (ROW_HEIGHT - m_height) / 2;
		dialog->DrawTexture(m_x, m_y + dy, m_width, m_height, m_skin, xofs, 0);
	}

	if (unsigned(m_state) < m_stateLabels.size())
	{
		std::string label = m_stateLabels[m_state];
		int dx = m_numStates > 0 ? 4 : (m_width - 8 * label.size()) / 2;
		int dy = 3;
		dialog->DrawText(dialog->lpDialogSurf, m_x + dx, m_y + dy, label.c_str());
	}
}

void Button::RegistryRead(HKEY hKey)
{
	int value = 0;
	DWORD size = 1;
	if (RegQueryValueEx(hKey, m_registryKey.c_str(), NULL, NULL, (unsigned char*)&value, &size) == ERROR_SUCCESS)
	{
		if (value >= 0 && value < m_numStates)
		{
			m_state = value;
		}
	}
}

void Button::RegistryWrite(HKEY hKey)
{
	RegSetValueEx(hKey, m_registryKey.c_str(), NULL, REG_BINARY, (unsigned char*)&m_state, 1);
}