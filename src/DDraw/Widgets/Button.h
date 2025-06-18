#pragma once

#include "Widget.h"
#include "../oddraw.h"

#include <vector>
#include <functional>

class Button : public Widget
{
public:
	Button(int x, int y, LPDIRECTDRAWSURFACE skin, int initialState, int numStates, bool depressable,
		const std::vector<std::string>& stateLabels, const std::string& registryKey, std::function<void(int)> onStateChange = std::function<void(int)>());
	virtual ~Button() { }

	virtual bool DoMessage(Dialog* dialog, HWND winProchWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	virtual std::string ToString();
	virtual void DoDraw(Dialog*);
	virtual void RegistryRead(HKEY hKey);
	virtual void RegistryWrite(HKEY hKey);
	virtual int GetState() { return m_state; }
	virtual void SetState(int state) { m_state = state % m_numStates; }

private:
	std::vector<std::string> m_stateLabels;
	int m_numStates;
	int m_state;
	int m_depressed;
	std::function<void(int)> m_onStateChange;
	LPDIRECTDRAWSURFACE m_skin;
	int m_skinWidth;
	std::string m_registryKey;
};