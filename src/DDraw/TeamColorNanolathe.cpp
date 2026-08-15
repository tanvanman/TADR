#include "TeamColorNanolathe.h"
#include "hook/hook.h"
#include "iddrawsurface.h"
#include "tamem.h"
#include "TAConfig.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <unordered_map>

namespace {
const DWORD kEmitterEntry = 0x004720D0, kEmitterPreTag = 0x00472169, kEmitterPostTag = 0x0047217C;
const DWORD kReverseEmitterEntry = 0x00472200, kReverseEmitterPreTag = 0x00472299, kReverseEmitterPostTag = 0x004722AC;
const DWORD kPalette = 0x00473F3B;
const DWORD kPaletteAdvance = 0x004739E6;
const DWORD kNanoframeStart = 0x00458DF1;
const DWORD kNanoframeColors = 0x00458E8E;
const BYTE kDefault = 0xA1;
const unsigned kPlayerColorCount = 10;
const unsigned kMaxStreamColors = 15;
const unsigned kFrameColorCount = 16;

struct ColorConfig {
	unsigned streamCount;
	BYTE stream[kMaxStreamColors];
	BYTE frame[kFrameColorCount];
};

const ColorConfig kDefaultColorConfigs[kPlayerColorCount] = {
	{ 6, { 224, 225, 226, 227, 228, 229 }, { 224, 224, 225, 225, 226, 226, 227, 227, 228, 228, 229, 229, 230, 230, 231, 231 } },
	{ 6, { 249, 201, 202, 203, 204, 205 }, { 201, 201, 201, 202, 202, 203, 203, 204, 204, 205, 205, 206, 206, 207, 207, 207 } },
	{ 7, { 81, 82, 83, 84, 85, 86, 87 }, { 80, 80, 81, 81, 82, 82, 83, 83, 84, 84, 85, 85, 86, 87, 88, 89 } },
	{ 6, { 233, 234, 235, 236, 237, 238 }, { 232, 232, 233, 233, 234, 234, 235, 235, 236, 236, 237, 237, 238, 238, 239, 239 } },
	{ 7, { 103, 104, 105, 106, 107, 108, 109 }, { 103, 103, 104, 104, 105, 105, 106, 106, 107, 107, 108, 108, 109, 109, 110, 111 } },
	{ 6, { 217, 218, 219, 220, 221, 222 }, { 216, 216, 217, 217, 218, 218, 219, 219, 220, 220, 221, 221, 222, 222, 223, 223 } },
	{ 6, { 208, 193, 194, 195, 196, 197 }, { 192, 192, 193, 193, 194, 194, 195, 195, 196, 196, 197, 197, 198, 198, 199, 199 } },
	{ 7, { 89, 90, 91, 92, 93, 94, 95 }, { 88, 88, 89, 89, 90, 90, 91, 91, 92, 92, 93, 93, 94, 94, 95, 95 } },
	{ 7, { 129, 130, 131, 132, 133, 134, 135 }, { 128, 128, 129, 129, 130, 130, 131, 131, 132, 132, 133, 133, 134, 134, 135, 135 } },
	{ 7, { 65, 66, 67, 68, 69, 70, 71 }, { 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79 } }
};

struct Tag { DWORD expiresAt; BYTE playerColor; };
std::unique_ptr<InlineSingleHook> g_entry, g_preTag, g_postTag, g_reverseEntry, g_reversePreTag, g_reversePostTag, g_palette, g_paletteAdvance, g_nanoframeStart, g_nanoframeColors;
std::unordered_map<void*, Tag> g_tags;
BYTE g_pendingPlayerColor = 0xFF;
BYTE g_nanoframePlayerColor = 0xFF;
ColorConfig g_colorConfigs[kPlayerColorCount];
unsigned g_streamCursor[kPlayerColorCount] = {};
bool g_enabled = false;
DWORD g_lastPruneTime = 0;

bool ParseColorList(const char* text, BYTE* output, unsigned capacity, unsigned requiredCount, unsigned& count) {
	count = 0;
	const char* cursor = text;
	while (true) {
		while (*cursor == ' ' || *cursor == '\t') ++cursor;
		if (*cursor == '\0' || *cursor == ';') break;
		if (count >= capacity) return false;

		char* end = nullptr;
		const long value = strtol(cursor, &end, 10);
		if (end == cursor || value < 0 || value > 255) return false;
		output[count++] = (BYTE)value;
		cursor = end;

		while (*cursor == ' ' || *cursor == '\t') ++cursor;
		if (*cursor == '\0' || *cursor == ';') break;
		if (*cursor != ',') return false;
		++cursor;
	}
	return count > 0 && (requiredCount == 0 || count == requiredCount);
}

void LoadColorConfig() {
	memcpy(g_colorConfigs, kDefaultColorConfigs, sizeof(g_colorConfigs));
	memset(g_streamCursor, 0, sizeof(g_streamCursor));
	g_enabled = MyConfig && MyConfig->GetIniBool("TeamColorNanolathe", FALSE);
	if (!g_enabled) return;

	char key[64];
	char value[512];
	for (unsigned player = 0; player < kPlayerColorCount; ++player) {
		sprintf_s(key, sizeof(key), "Player%uStreamColors", player + 1);
		if (MyConfig->GetIniStr(key, value, sizeof(value), NULL) > 0) {
			BYTE parsed[kMaxStreamColors];
			unsigned count = 0;
			if (ParseColorList(value, parsed, kMaxStreamColors, 0, count)) {
				memcpy(g_colorConfigs[player].stream, parsed, count);
				g_colorConfigs[player].streamCount = count;
			} else {
				IDDrawSurface::OutptFmtTxt("[TeamColorNanolathe] invalid %s; using defaults", key);
			}
		}

		sprintf_s(key, sizeof(key), "Player%uFrameColors", player + 1);
		if (MyConfig->GetIniStr(key, value, sizeof(value), NULL) > 0) {
			BYTE parsed[kFrameColorCount];
			unsigned count = 0;
			if (ParseColorList(value, parsed, kFrameColorCount, kFrameColorCount, count)) {
				memcpy(g_colorConfigs[player].frame, parsed, kFrameColorCount);
			} else {
				IDDrawSurface::OutptFmtTxt("[TeamColorNanolathe] invalid %s; using defaults", key);
			}
		}
	}
}

bool IsConfiguredStreamColor(BYTE color) {
	for (unsigned player = 0; player < kPlayerColorCount; ++player) {
		for (unsigned index = 0; index < g_colorConfigs[player].streamCount; ++index) {
			if (g_colorConfigs[player].stream[index] == color) return true;
		}
	}
	return false;
}

TAdynmemStruct* GetTADynmem() {
	return *(TAdynmemStruct**)0x00511DE8;
}

void PruneExpiredTags(DWORD gameTime) {
	if (gameTime - g_lastPruneTime < 90) return;
	g_lastPruneTime = gameTime;
	for (auto it = g_tags.begin(); it != g_tags.end();) {
		if (gameTime > it->second.expiresAt) {
			it = g_tags.erase(it);
		} else {
			++it;
		}
	}
}

int __stdcall NanoframeStartProc(PInlineX86StackBuffer p) {
	g_nanoframePlayerColor = 0xFF;
	UnitStruct* unit = (UnitStruct*)p->Edx;
	if (unit && unit->Owner_PlayerPtr0 && unit->Owner_PlayerPtr0->PlayerInfo) {
		const BYTE color = unit->Owner_PlayerPtr0->PlayerInfo->PlayerLogoColor;
		if (color < 10) g_nanoframePlayerColor = color;
	}
	return 0;
}

BYTE MapNanoframeColorInternal(BYTE playerColor, DWORD stockColor) {
	if (!g_enabled || playerColor >= kPlayerColorCount || stockColor < 0xA0 || stockColor > 0xAF) {
		return (BYTE)stockColor;
	}
	const unsigned offset = (stockColor - 0xA0) & 0x0F;
	return g_colorConfigs[playerColor].frame[offset];
}

int __stdcall NanoframeColorsProc(PInlineX86StackBuffer p) {
	p->Esi = MapNanoframeColorInternal(g_nanoframePlayerColor, p->Esi);
	p->Ebx = MapNanoframeColorInternal(g_nanoframePlayerColor, p->Ebx);
	return 0;
}

void SetPending(UnitStruct* unit) {
	g_pendingPlayerColor = 0xFF;
	if (!unit || !unit->Owner_PlayerPtr0 || !unit->Owner_PlayerPtr0->PlayerInfo) return;
	const BYTE color = unit->Owner_PlayerPtr0->PlayerInfo->PlayerLogoColor;
	if (color >= kPlayerColorCount) return;
	g_pendingPlayerColor = color;
}

void SetPendingForSource(const Position_Dword* source) {
	TAdynmemStruct* ta = GetTADynmem();
	if (!source || !ta || !ta->BeginUnitsArray_p || !ta->EndOfUnitsArray_p) { SetPending(nullptr); return; }

	PruneExpiredTags(ta->GameTime);
	const int sourceX = *(const int*)&source->x_;
	const int sourceZ = *(const int*)&source->z_;
	const int sourceY = *(const int*)&source->y_;
	UnitStruct* nearest = nullptr;
	unsigned __int64 bestDistance = ~0ull;
	for (UnitStruct* unit = ta->BeginUnitsArray_p; unit <= ta->EndOfUnitsArray_p; ++unit) {
		if (!unit->IsUnit || !unit->UnitType || !unit->Owner_PlayerPtr0) continue;
		const __int64 dx = (__int64)*(int*)&unit->XPos__ - sourceX;
		const __int64 dz = (__int64)*(int*)&unit->ZPos__ - sourceZ;
		const __int64 dy = (__int64)*(int*)&unit->YPos__ - sourceY;
		const unsigned __int64 distance = dx * dx + dz * dz + dy * dy;
		if (distance < bestDistance) { bestDistance = distance; nearest = unit; }
	}
	SetPending(nearest);
}

int __stdcall EmitterEntry(PInlineX86StackBuffer p) {
	SetPendingForSource(*(const Position_Dword**)(p->Esp + 4));
	return 0;
}

int __stdcall ReverseEmitterEntry(PInlineX86StackBuffer p) {
	// EmitSfx_NanoParticlesReverse(target, source, priority): the builder-side endpoint is its second argument.
	SetPendingForSource(*(const Position_Dword**)(p->Esp + 8));
	return 0;
}

int __stdcall PreTagEmitter(PInlineX86StackBuffer p) {
	TAdynmemStruct* ta = GetTADynmem();
	const DWORD fallbackExpiry = ta ? ta->GameTime + 300 : 300;
	g_tags[(void*)p->Esi] = { fallbackExpiry, g_pendingPlayerColor };
	// One emitter call can create several particles. Keep the selected colour for
	// the complete burst; the next emitter entry replaces it before another burst.
	return 0;
}

int __stdcall PostTagEmitter(PInlineX86StackBuffer p) {
	auto it = g_tags.find((void*)p->Esi);
	if (it != g_tags.end()) it->second.expiresAt = *(DWORD*)(p->Esi + 4);
	return 0;
}

int __stdcall SetPalette(PInlineX86StackBuffer p) {
	auto it = g_tags.find((void*)p->Ebp);
	TAdynmemStruct* ta = GetTADynmem();
	if (it == g_tags.end()) return 0;
	if (!ta || (DWORD)ta->GameTime > it->second.expiresAt) {
		g_tags.erase(it);
		return 0;
	}
	const BYTE playerColor = it->second.playerColor;
	if (playerColor >= kPlayerColorCount) return 0;
	const ColorConfig& config = g_colorConfigs[playerColor];
	const unsigned colorOffset = (p->Edx + g_streamCursor[playerColor]++) % config.streamCount;
	p->Edx = (DWORD)((int)config.stream[colorOffset] - (int)kDefault);
	return 0;
}

int __stdcall AdvancePalette(PInlineX86StackBuffer p) {
	const unsigned step = (p->Eax & 0xFFFF) - 1;
	if (step >= 7) return 0;
	auto it = g_tags.find((void*)p->Ecx);
	const BYTE current = p->Edx & 0xFF;
	if (it == g_tags.end()) {
		if (!IsConfiguredStreamColor(current)) return 0;
	} else {
		const BYTE playerColor = it->second.playerColor;
		if (playerColor >= kPlayerColorCount) return 0;
		bool found = false;
		const ColorConfig& config = g_colorConfigs[playerColor];
		for (unsigned index = 0; index < config.streamCount; ++index) {
			if (config.stream[index] == current) {
				found = true;
				break;
			}
		}
		if (!found) return 0;
	}
	// The displaced instructions rebuild the colour as (EDX & 0xFFF0) + AX.
	// Keep the assigned colour stable instead of letting TA advance it through
	// adjacent palette entries as the particle travels.
	p->Edx = current & 0xF0;
	p->Eax = current & 0x0F;
	return 0;
}
}

namespace TeamColorNanolathe {
bool IsEnabled() {
	return g_enabled;
}

unsigned char MapNanoframeColor(unsigned char playerColor, unsigned char stockColor) {
	return MapNanoframeColorInternal(playerColor, stockColor);
}

void Install() {
	if (g_palette) return;
	LoadColorConfig();
	if (!g_enabled) {
		IDDrawSurface::OutptTxt("[TeamColorNanolathe] disabled");
		return;
	}
	g_entry.reset(new InlineSingleHook(kEmitterEntry, 7, INLINE_5BYTESLAGGERJMP, EmitterEntry));
	g_preTag.reset(new InlineSingleHook(kEmitterPreTag, 6, INLINE_5BYTESLAGGERJMP, PreTagEmitter));
	g_postTag.reset(new InlineSingleHook(kEmitterPostTag, 5, INLINE_5BYTESLAGGERJMP, PostTagEmitter));
	g_reverseEntry.reset(new InlineSingleHook(kReverseEmitterEntry, 7, INLINE_5BYTESLAGGERJMP, ReverseEmitterEntry));
	g_reversePreTag.reset(new InlineSingleHook(kReverseEmitterPreTag, 6, INLINE_5BYTESLAGGERJMP, PreTagEmitter));
	g_reversePostTag.reset(new InlineSingleHook(kReverseEmitterPostTag, 5, INLINE_5BYTESLAGGERJMP, PostTagEmitter));
	g_palette.reset(new InlineSingleHook(kPalette, 6, INLINE_5BYTESLAGGERJMP, SetPalette));
	g_paletteAdvance.reset(new InlineSingleHook(kPaletteAdvance, 11, INLINE_5BYTESLAGGERJMP, AdvancePalette));
	g_nanoframeStart.reset(new InlineSingleHook(kNanoframeStart, 10, INLINE_5BYTESLAGGERJMP, NanoframeStartProc));
	g_nanoframeColors.reset(new InlineSingleHook(kNanoframeColors, 10, INLINE_5BYTESLAGGERJMP, NanoframeColorsProc));
	IDDrawSurface::OutptTxt("[TeamColorNanolathe] installed");
}
void Shutdown() {
	g_nanoframeColors.reset(); g_nanoframeStart.reset();
	g_paletteAdvance.reset(); g_palette.reset(); g_reversePostTag.reset(); g_reversePreTag.reset(); g_reverseEntry.reset(); g_postTag.reset(); g_preTag.reset(); g_entry.reset();
	g_tags.clear(); g_pendingPlayerColor = 0xFF; g_nanoframePlayerColor = 0xFF;
	g_enabled = false;
	memset(g_streamCursor, 0, sizeof(g_streamCursor));
}
}
