#include "TeamColorNanolathe.h"
#include "config.h"
#if REPAIR_HEALTH_SYNC_ENABLE
#include "RepairHealthSync.h"
#endif
#include "hook/hook.h"
#include "iddrawsurface.h"
#include "TAbugfix.h"      // g_currentOrderUnit / g_currentOrderUnitTick
#include "tamem.h"
#include "TAConfig.h"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <memory>

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

std::unique_ptr<InlineSingleHook> g_entry, g_preTag, g_postTag, g_reverseEntry, g_reversePreTag, g_reversePostTag, g_palette, g_paletteAdvance, g_nanoframeStart, g_nanoframeColors;

// Flat open-addressed tag table, replacing std::unordered_map<void*, Tag> (2026-09-08).
// AdvancePalette/SetPalette look a tag up PER PARTICLE, and the map was node-based: hash, bucket,
// then a pointer chase into a separately allocated node, plus a node allocated and freed per
// emitter burst. Same algorithm without the indirection. Deletion tombstones so probe chains
// survive; Prune rebuilds from survivors, which clears them and costs no more than before.
//
// NOT stored on TA's SFX object, which would be cheaper: ExplosionStruct/DebrisStruct are opaque
// blobs here (data2[6], data4[36]), so those bytes are UNKNOWN rather than known-free.
//
// 8192, not 4096: aux effects alone reach 3000 and the palette hooks also see explosion and
// model-effect objects, so 4096 would run at a load factor where probe clusters can hit
// kTagMaxProbe. Both arrays are zero-init, so they cost .bss, not file size.
const unsigned kTagSlots = 8192;              // power of two
const unsigned kTagMaxProbe = 64;             // bound the work; exceeding it just loses a tag,
                                              // which shows as a default-coloured nanolathe beam

struct TagEntry { void* key; DWORD expiresAt; BYTE playerColor; BYTE state; };
enum { kTagEmpty = 0, kTagUsed = 1, kTagDead = 2 };

TagEntry g_tagTable[kTagSlots];
TagEntry g_tagScratch[kTagSlots];
unsigned g_tagCount = 0;

inline unsigned TagHash(void* key) {
	// Pointers are 4/8/16-byte aligned, so the low bits carry no entropy -- mix them out.
	unsigned h = (unsigned)(uintptr_t)key;
	h ^= h >> 16; h *= 0x7FEB352Du; h ^= h >> 15;
	return h & (kTagSlots - 1);
}

// Returns the entry itself, not a Tag* aliased onto its middle -- that pun happened to work
// because the field order matches, but it is exactly the kind of thing that breaks silently.
TagEntry* TagFind(void* key) {
	unsigned i = TagHash(key);
	for (unsigned n = 0; n < kTagMaxProbe; ++n) {
		TagEntry& e = g_tagTable[i];
		if (e.state == kTagEmpty) return nullptr;        // empty ends the chain; tombstones do not
		if (e.state == kTagUsed && e.key == key) return &e;
		i = (i + 1) & (kTagSlots - 1);
	}
	return nullptr;
}

void TagInsert(void* key, DWORD expiresAt, BYTE playerColor) {
	unsigned i = TagHash(key);
	int firstFree = -1;
	for (unsigned n = 0; n < kTagMaxProbe; ++n) {
		TagEntry& e = g_tagTable[i];
		if (e.state == kTagUsed && e.key == key) {       // replace in place
			e.expiresAt = expiresAt; e.playerColor = playerColor;
			return;
		}
		if (e.state != kTagUsed && firstFree < 0) firstFree = (int)i;
		if (e.state == kTagEmpty) break;
		i = (i + 1) & (kTagSlots - 1);
	}
	if (firstFree < 0) return;                           // table saturated: drop the tag (cosmetic)
	TagEntry& e = g_tagTable[firstFree];
	e.key = key; e.expiresAt = expiresAt; e.playerColor = playerColor; e.state = kTagUsed;
	++g_tagCount;
}

void TagErase(void* key) {
	unsigned i = TagHash(key);
	for (unsigned n = 0; n < kTagMaxProbe; ++n) {
		TagEntry& e = g_tagTable[i];
		if (e.state == kTagEmpty) return;
		if (e.state == kTagUsed && e.key == key) {
			e.state = kTagDead;                          // tombstone: keeps later probes reachable
			if (g_tagCount) --g_tagCount;
			return;
		}
		i = (i + 1) & (kTagSlots - 1);
	}
}

void TagClear() {
	memset(g_tagTable, 0, sizeof(g_tagTable));
	g_tagCount = 0;
}
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
	// Rebuild from the survivors: drops expired entries AND clears the tombstones erase leaves
	// behind, so probe chains stay short. Same O(capacity) walk the map version did.
	unsigned survivors = 0;
	for (unsigned i = 0; i < kTagSlots; ++i) {
		const TagEntry& e = g_tagTable[i];
		if (e.state == kTagUsed && gameTime <= e.expiresAt) {
			g_tagScratch[survivors++] = e;
		}
	}
	TagClear();
	for (unsigned i = 0; i < survivors; ++i) {
		const TagEntry& e = g_tagScratch[i];
		TagInsert(e.key, e.expiresAt, e.playerColor);
	}
}

int __stdcall NanoframeStartProc(PInlineX86StackBuffer p) {
#if REPAIR_HEALTH_SYNC_ENABLE
	RepairHealthSync::BeginNanoframeDraw((UnitStruct*)p->Edx);
#endif
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

// Memo cache for the nearest-unit lookup below.
//
// SetPendingForSource runs from EmitSfx_NanoParticles (0x004720D0, 9+ call sites in the order
// handlers) and used to scan all 15,000 unit slots per call to produce ONE colour byte. Live
// sampling put ~37% of TA's main thread in that scan. The emitter is handed only a point, so the
// builder identity genuinely isn't available -- but a builder's emission point is bit-stable while
// it builds, so the answer memoises.
//
// Keyed on the exact source position. Entries store the resolved COLOUR, never the UnitStruct*, so
// a unit dying inside the TTL cannot leave a dangling pointer. Worst case on a stale hit is a beam
// drawn in the previous owner's colour for under half a second; this path is cosmetic and touches
// no simulation state. A GameTime reset (new game) makes now - stamp underflow to a huge unsigned,
// which fails the TTL test and refills the entry -- no explicit reset needed.
const unsigned kMemoSlots    = 64;    // power of two
const DWORD    kMemoTtlTicks = 15;    // ~0.5 s at 30 tps

struct MemoEntry {
	int x, y, z;
	DWORD stamp;      // GameTime when resolved; 0 = empty
	BYTE color;
};
MemoEntry g_memo[kMemoSlots] = {};
unsigned g_memoHits = 0, g_memoMisses = 0;
DWORD g_memoLogTick = 0;

inline unsigned MemoHash(int x, int y, int z) {
	unsigned h = (unsigned)x * 0x9E3779B1u ^ (unsigned)y * 0x85EBCA77u ^ (unsigned)z * 0xC2B2AE3Du;
	h ^= h >> 15;
	return h & (kMemoSlots - 1);
}

UnitStruct* NearestUnitToPoint(TAdynmemStruct* ta, int sourceX, int sourceY, int sourceZ) {
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
	return nearest;
}

// Fast path counters. The cross-check below is the point: it proves the dispatch unit agrees with
// what the scan would have chosen, instead of assuming it.
unsigned g_fastHits = 0, g_fastChecks = 0, g_fastMismatch = 0;

void SetPendingForSource(const Position_Dword* source) {
	TAdynmemStruct* ta = GetTADynmem();
	if (!source || !ta || !ta->BeginUnitsArray_p || !ta->EndOfUnitsArray_p) { SetPending(nullptr); return; }

	PruneExpiredTags(ta->GameTime);
	const int sourceX = *(const int*)&source->x_;
	const int sourceZ = *(const int*)&source->z_;
	const int sourceY = *(const int*)&source->y_;

	// The emitter is handed a bare position, which is the only reason the scan below exists. But
	// EmitSfx_NanoParticles is called from the builder's own order handler, so during dispatch the
	// emitting unit is already known -- exact and O(1), where the scan is 15,000 slots.
	UnitStruct* dispatched = (g_currentOrderUnitTick == ta->GameTime)
		? (UnitStruct*)g_currentOrderUnit : nullptr;
	if (dispatched && dispatched >= ta->BeginUnitsArray_p && dispatched <= ta->EndOfUnitsArray_p &&
		dispatched->IsUnit && dispatched->UnitType && dispatched->Owner_PlayerPtr0) {
		SetPending(dispatched);
		++g_fastHits;
		if ((g_fastHits & 0xFF) == 0) {          // 1 in 256: verify against the scan
			const BYTE fast = g_pendingPlayerColor;
			SetPending(NearestUnitToPoint(ta, sourceX, sourceY, sourceZ));
			++g_fastChecks;
			if (g_pendingPlayerColor != fast) ++g_fastMismatch;
			g_pendingPlayerColor = fast;         // the dispatch unit is the authority
		}
		return;
	}

	const DWORD now = (DWORD)ta->GameTime;
	MemoEntry& slot = g_memo[MemoHash(sourceX, sourceY, sourceZ)];
	if (slot.stamp != 0 && (now - slot.stamp) < kMemoTtlTicks &&
		slot.x == sourceX && slot.y == sourceY && slot.z == sourceZ) {
		g_pendingPlayerColor = slot.color;
		++g_memoHits;
		return;
	}
	++g_memoMisses;

	SetPending(NearestUnitToPoint(ta, sourceX, sourceY, sourceZ));

	slot.x = sourceX; slot.y = sourceY; slot.z = sourceZ;
	slot.stamp = now; slot.color = g_pendingPlayerColor;

	// Report the hit rate, so the fix is verified rather than assumed. Miss path only: on a
	// well-behaved cache that is the rare one, and the counters are cumulative anyway.
	if (now - g_memoLogTick >= 900) {
		g_memoLogTick = now;
		const unsigned total = g_memoHits + g_memoMisses;
		IDDrawSurface::OutptFmtTxt(
			"[TeamColorNanolathe] dispatch=%u (checked %u, mismatch %u) | memo hits=%u misses=%u (%u%% hit) scan=%u units\n",
			g_fastHits, g_fastChecks, g_fastMismatch,
			g_memoHits, g_memoMisses, total ? (g_memoHits * 100u / total) : 0u,
			(unsigned)(ta->EndOfUnitsArray_p - ta->BeginUnitsArray_p + 1));
	}
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
	TagInsert((void*)p->Esi, fallbackExpiry, g_pendingPlayerColor);
	// One emitter call can create several particles. Keep the selected colour for
	// the complete burst; the next emitter entry replaces it before another burst.
	return 0;
}

int __stdcall PostTagEmitter(PInlineX86StackBuffer p) {
	TagEntry* e = TagFind((void*)p->Esi);
	if (e) e->expiresAt = *(DWORD*)(p->Esi + 4);
	return 0;
}

int __stdcall SetPalette(PInlineX86StackBuffer p) {
	TagEntry* e = TagFind((void*)p->Ebp);
	TAdynmemStruct* ta = GetTADynmem();
	if (!e) return 0;
	if (!ta || (DWORD)ta->GameTime > e->expiresAt) {
		TagErase((void*)p->Ebp);
		return 0;
	}
	const BYTE playerColor = e->playerColor;
	if (playerColor >= kPlayerColorCount) return 0;
	const ColorConfig& config = g_colorConfigs[playerColor];
	const unsigned colorOffset = (p->Edx + g_streamCursor[playerColor]++) % config.streamCount;
	p->Edx = (DWORD)((int)config.stream[colorOffset] - (int)kDefault);
	return 0;
}

int __stdcall AdvancePalette(PInlineX86StackBuffer p) {
	const unsigned step = (p->Eax & 0xFFFF) - 1;
	if (step >= 7) return 0;
	TagEntry* e = TagFind((void*)p->Ecx);
	const BYTE current = p->Edx & 0xFF;
	if (!e) {
		if (!IsConfiguredStreamColor(current)) return 0;
	} else {
		const BYTE playerColor = e->playerColor;
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
#if REPAIR_HEALTH_SYNC_ENABLE
	if (g_nanoframeStart) return;
	LoadColorConfig();
	// This entry hook is also the safe display-only construction-progress
	// substitution point used by RepairHealthSync, so install it even when
	// custom nanolathe colours are disabled.
	g_nanoframeStart.reset(new InlineSingleHook(kNanoframeStart, 10, INLINE_5BYTESLAGGERJMP, NanoframeStartProc));
	if (!g_enabled) {
		IDDrawSurface::OutptTxt("[TeamColorNanolathe] disabled");
		return;
	}
#else
	if (g_palette) return;
	LoadColorConfig();
	if (!g_enabled) {
		IDDrawSurface::OutptTxt("[TeamColorNanolathe] disabled");
		return;
	}
#endif
	g_entry.reset(new InlineSingleHook(kEmitterEntry, 7, INLINE_5BYTESLAGGERJMP, EmitterEntry));
	g_preTag.reset(new InlineSingleHook(kEmitterPreTag, 6, INLINE_5BYTESLAGGERJMP, PreTagEmitter));
	g_postTag.reset(new InlineSingleHook(kEmitterPostTag, 5, INLINE_5BYTESLAGGERJMP, PostTagEmitter));
	g_reverseEntry.reset(new InlineSingleHook(kReverseEmitterEntry, 7, INLINE_5BYTESLAGGERJMP, ReverseEmitterEntry));
	g_reversePreTag.reset(new InlineSingleHook(kReverseEmitterPreTag, 6, INLINE_5BYTESLAGGERJMP, PreTagEmitter));
	g_reversePostTag.reset(new InlineSingleHook(kReverseEmitterPostTag, 5, INLINE_5BYTESLAGGERJMP, PostTagEmitter));
	g_palette.reset(new InlineSingleHook(kPalette, 6, INLINE_5BYTESLAGGERJMP, SetPalette));
	g_paletteAdvance.reset(new InlineSingleHook(kPaletteAdvance, 11, INLINE_5BYTESLAGGERJMP, AdvancePalette));
#if !REPAIR_HEALTH_SYNC_ENABLE
	g_nanoframeStart.reset(new InlineSingleHook(kNanoframeStart, 10, INLINE_5BYTESLAGGERJMP, NanoframeStartProc));
#endif
	g_nanoframeColors.reset(new InlineSingleHook(kNanoframeColors, 10, INLINE_5BYTESLAGGERJMP, NanoframeColorsProc));
	IDDrawSurface::OutptTxt("[TeamColorNanolathe] installed");
}
void Shutdown() {
	g_nanoframeColors.reset(); g_nanoframeStart.reset();
	g_paletteAdvance.reset(); g_palette.reset(); g_reversePostTag.reset(); g_reversePreTag.reset(); g_reverseEntry.reset(); g_postTag.reset(); g_preTag.reset(); g_entry.reset();
	TagClear(); g_pendingPlayerColor = 0xFF; g_nanoframePlayerColor = 0xFF;
	g_enabled = false;
	memset(g_streamCursor, 0, sizeof(g_streamCursor));
}
}
