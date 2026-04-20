#include "HudNotifications.h"
#include "iddrawsurface.h"
#include "tamem.h"
#include "tafunctions.h"

#include <algorithm>
#include <cstring>

HudNotifications* HudNotifications::m_instance = nullptr;

HudNotifications* HudNotifications::GetInstance()
{
	if (!m_instance)
		m_instance = new HudNotifications();
	return m_instance;
}

HudLineId HudNotifications::AddLine(const std::string& key, std::string text)
{
	HudLineId id = m_nextId++;
	if (m_nextId == INVALID_HUD_LINE_ID)
		++m_nextId;
	m_lines[id] = { key, std::move(text) };
	return id;
}

void HudNotifications::UpdateLine(HudLineId id, std::string text)
{
	auto it = m_lines.find(id);
	if (it != m_lines.end())
		it->second.text = std::move(text);
}

void HudNotifications::RemoveLine(HudLineId id)
{
	if (id == INVALID_HUD_LINE_ID)
		return;
	m_lines.erase(id);
	// Clean up any group-token tracking (O(n) but groups are tiny).
	for (auto& kv : m_groupTokens)
	{
		auto& vec = kv.second;
		vec.erase(std::remove(vec.begin(), vec.end(), id), vec.end());
	}
}

void HudNotifications::SetLines(const std::string& key, std::vector<std::string> lines)
{
	ClearLines(key);
	for (auto& text : lines)
		m_groupTokens[key].push_back(AddLine(key, std::move(text)));
}

void HudNotifications::ClearLines(const std::string& key)
{
	auto it = m_groupTokens.find(key);
	if (it != m_groupTokens.end())
	{
		for (HudLineId id : it->second)
			m_lines.erase(id);
		m_groupTokens.erase(it);
	}
}

void HudNotifications::Blit(LPVOID lpSurfaceMem, int dwWidth, int dwHeight, int lPitch)
{
	if (lpSurfaceMem == nullptr || m_lines.empty())
		return;
	if (DataShare->TAProgress != TAInGame)
		return;

	TAProgramStruct* programPtr = *(TAProgramStruct**)0x0051fbd0;
	TAdynmemStruct*  taPtr      = *(TAdynmemStruct**)0x00511de8;

	OFFSCREEN offscreen;
	std::memset(&offscreen, 0, sizeof(offscreen));
	offscreen.Height            = dwHeight;
	offscreen.Width             = lPitch;
	offscreen.lPitch            = lPitch;
	offscreen.lpSurface         = lpSurfaceMem;
	offscreen.ScreenRect.left   = 0;
	offscreen.ScreenRect.right  = dwWidth;
	offscreen.ScreenRect.top    = 0;
	offscreen.ScreenRect.bottom = dwHeight;

	programPtr->fontHandle      = (unsigned char*)taPtr->COMIXFontHandle;
	programPtr->fontFrontColour = taPtr->desktopGUI.RadarObjecColor[15];
	programPtr->fontBackColour  = programPtr->fontAlpha;
	int fontHeight = programPtr->fontHandle[0];

	bool clockMode = (taPtr->SoftwareDebugMode & softwaredebugmode::Clock) != 0;

	// Collect all lines sorted by (key, id): key-alphabetical then insertion order.
	// "challenge" sorts before "vote", so challenge lines occupy the lower slots.
	std::vector<std::pair<std::pair<std::string, HudLineId>, const std::string*>> sorted;
	sorted.reserve(m_lines.size());
	for (auto& kv : m_lines)
		sorted.push_back({{ kv.second.key, kv.first }, &kv.second.text});
	std::sort(sorted.begin(), sorted.end());

	for (int n = 0; n < (int)sorted.size(); ++n)
	{
		int yOff = dwHeight - fontHeight * (n - 1) - 64;
		if (clockMode)
			yOff -= fontHeight;
		DrawTextInScreen(&offscreen,
			const_cast<char*>(sorted[n].second->c_str()),
			129, yOff, -1);
	}
}
