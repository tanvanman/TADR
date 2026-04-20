#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

using HudLineId = uint32_t;
static const HudLineId INVALID_HUD_LINE_ID = 0;

// Service for rendering persistent text lines at the bottom of the game screen.
//
// Lines are identified by a unique token (HudLineId) returned from AddLine().
// That token can later be passed to UpdateLine() to refresh the text in place,
// or to RemoveLine() to remove it.
//
// Lines are grouped by a string key and rendered in key-alphabetical order
// (lower keys appear nearer the bottom edge), then by insertion order within
// each key.  "challenge" therefore always sorts before "vote".
//
// SetLines/ClearLines are retained for callers that replace a whole group at
// once (e.g. ChallengeResponse).
class HudNotifications
{
public:
	static HudNotifications* GetInstance();

	// Add a new line to the named group.  Returns a token for later UpdateLine
	// or RemoveLine.  Never returns INVALID_HUD_LINE_ID.
	HudLineId AddLine(const std::string& key, std::string text);

	// Change the text of an existing line.  No-op if id is unknown.
	void UpdateLine(HudLineId id, std::string text);

	// Remove a specific line.  No-op if id is unknown.
	void RemoveLine(HudLineId id);

	// Replace all lines for a named group atomically (for callers that don't
	// need per-line token control).  Passing an empty vector = ClearLines.
	void SetLines(const std::string& key, std::vector<std::string> lines);
	void ClearLines(const std::string& key);

	void Blit(void* lpSurfaceMem, int dwWidth, int dwHeight, int lPitch);

private:
	HudNotifications() = default;

	struct HudLine {
		std::string key;
		std::string text;
	};

	static HudNotifications* m_instance;

	std::map<HudLineId, HudLine>                   m_lines;
	std::map<std::string, std::vector<HudLineId>>  m_groupTokens;  // for SetLines/ClearLines
	HudLineId m_nextId = 1;
};
