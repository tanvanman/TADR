#include "NanoframeNoLos.h"

#if NANOFRAME_NO_LOS_ENABLE

#include <windows.h>

#include <cstddef>
#include <cstring>
#include <memory>

#include "hook/hook.h"
#include "iddrawsurface.h"
#include "tamem.h"

// The stubs below address these fields with literal offsets.
static_assert(offsetof(UnitStruct, LosCacheCellX)    == 0x7A,    "UnitStruct::LosCacheCellX moved");
static_assert(offsetof(UnitStruct, LosCacheCellZ)    == 0x7C,    "UnitStruct::LosCacheCellZ moved");
static_assert(offsetof(UnitStruct, UnitID)           == 0xA6,    "UnitStruct::UnitID moved");
static_assert(offsetof(UnitStruct, LosSightFrameIdx) == 0xF8,    "UnitStruct::LosSightFrameIdx moved");
static_assert(offsetof(UnitStruct, Nanoframe)        == 0x104,   "UnitStruct::Nanoframe moved");
static_assert(offsetof(TAdynmemStruct, LosType)      == 0x14281, "TAdynmemStruct::LosType moved");

// Complete, as the engine tests it (0x408A0F: fld [unit+0x104]; fcomp 0.0f; test ah,40h): equal to 0.0 or
// unordered. In integer form, with a = bits & 0x7FFFFFFF: complete iff a == 0 or a > 0x7F800000 (NaN).
// Each stub spells this out, since inline assembly has no macros.

// Resume and call targets for the stubs.
static DWORD g_revealNewUnitResume = 0x00482AC7u;   // after sub esp,24h / mov eax,[esp+28h]
static DWORD g_updateUnitSightResume = 0x004827B7u;
static DWORD g_removeUnitSightResume = 0x00482097u;
static DWORD g_rebuildNextUnit = 0x004818F1u;       // the rebuild's next-unit step (needs edx = ta)
static DWORD g_rebuildUnitBody = 0x004817A4u;       // the rebuild's per-unit body (ebp = unit)
static DWORD g_deathEyeResume = 0x0048674Du;        // after the call to AddEyeball
static DWORD g_addEyeball = 0x00482910u;            // __stdcall(pos*, sight, height, ticks)

typedef void (__stdcall *UnitSightFn)(UnitStruct*);
// Both go through this module's own splices: S1 lets a complete unit through to the engine's body, S3
// lets a registered unit through.
static const UnitSightFn kRevealNewUnit = reinterpret_cast<UnitSightFn>(0x00482AC0u);
static const UnitSightFn kRemoveUnitSight = reinterpret_cast<UnitSightFn>(0x00482090u);

static bool LiveSightOn()
{
	// Map flag bit 1: per-player sight counts are live. Clear = "Permanent" LOS, where only the mapping
	// (bit 0) moves and nothing is ever counted.
	const TAdynmemStruct* ta = *reinterpret_cast<TAdynmemStruct* const*>(0x00511DE8u);
	return (ta->LosType & 2) != 0;
}

static void Suppress(UnitStruct* unit)
{
	unit->LosCacheCellX = 0x7FFF;
	unit->LosCacheCellZ = 0x7FFF;
	unit->LosSightFrameIdx = 0;
}

// A registered unit that is a nanoframe. Unit_TransferOwnership makes one: it creates the receiver's unit
// complete, then copies +0x104 (0x48873D). Takes its sight out of the counts the way its death would, then
// suppresses it.
static void __stdcall Unregister(UnitStruct* unit)
{
	if (LiveSightOn())
		kRemoveUnitSight(unit);
	Suppress(unit);
}

// A suppressed unit that has completed. Returns 1 when it registered the unit, 0 when the engine's
// update should run instead.
static int __stdcall RegisterCompleted(UnitStruct* unit)
{
	// Without live sight there is nothing to count: the engine's update sees the 0x7FFF cell as a move
	// and maps around the unit (bit 0), as it does for a new unit.
	if (!LiveSightOn())
		return 0;
	// A fresh registration, as at spawn: it adds and maps without removing anything first.
	kRevealNewUnit(unit);
	return 1;
}

// S1, 0x482AC0 Los_RevealNewUnit(Unit*): spawn.
static __declspec(naked) void RevealNewUnitStub()
{
	__asm
	{
		mov   eax, [esp + 4]
		mov   ecx, [eax + 0x104]
		and   ecx, 0x7FFFFFFF
		jz    original
		cmp   ecx, 0x7F800000
		ja    original
		mov   dword ptr [eax + 0x7A], 0x7FFF7FFF
		mov   byte ptr [eax + 0xF8], 0
		ret   4
	original:
		sub   esp, 0x24
		mov   eax, [esp + 0x28]
		jmp   dword ptr [g_revealNewUnitResume]
	}
}

// S2, 0x4827B0 Los_UpdateUnitFromPosition(Unit*): every tick for every live unit, and on moves.
static __declspec(naked) void UpdateUnitSightStub()
{
	__asm
	{
		mov   eax, [esp + 4]
		cmp   dword ptr [eax + 0x7A], 0x7FFF7FFF
		je    suppressed
		mov   ecx, [eax + 0x104]
		and   ecx, 0x7FFFFFFF
		jz    original
		cmp   ecx, 0x7F800000
		ja    original
		push  eax
		call  Unregister
		ret   4
	suppressed:
		mov   ecx, [eax + 0x104]
		and   ecx, 0x7FFFFFFF
		jz    completed
		cmp   ecx, 0x7F800000
		ja    completed
		ret   4
	completed:
		push  eax
		call  RegisterCompleted
		test  eax, eax
		jz    original
		ret   4
	original:
		sub   esp, 0x24
		mov   eax, [esp + 0x28]
		jmp   dword ptr [g_updateUnitSightResume]
	}
}

// S3, 0x482090 Los_DecUnitFogSightCounts(Unit*): death, only with live sight on.
static __declspec(naked) void RemoveUnitSightStub()
{
	__asm
	{
		mov   eax, [esp + 4]
		cmp   dword ptr [eax + 0x7A], 0x7FFF7FFF
		jne   original
		ret   4
	original:
		sub   esp, 0x24
		mov   eax, [esp + 0x28]
		jmp   dword ptr [g_removeUnitSightResume]
	}
}

// S4, 0x481796: head of Los_RebuildAllVisibilityMasks' unit loop, in place of cmp word [ebp+0A6h],0 /
// je 4818F1h. ebp = unit, edx = ta; eax is dead here (both targets load it before reading it).
static __declspec(naked) void RebuildLoopStub()
{
	__asm
	{
		cmp   word ptr [ebp + 0xA6], 0
		je    skipUnit
		mov   eax, [ebp + 0x104]
		and   eax, 0x7FFFFFFF
		jz    unitBody
		cmp   eax, 0x7F800000
		ja    unitBody
		mov   dword ptr [ebp + 0x7A], 0x7FFF7FFF
		mov   byte ptr [ebp + 0xF8], 0
	skipUnit:
		jmp   dword ptr [g_rebuildNextUnit]
	unitBody:
		jmp   dword ptr [g_rebuildUnitBody]
	}
}

// S5, 0x486748: Unit_HandleKilledPacket's call AddEyeball(&unit->pos, sightdistance, height, 60), made when
// the dead unit's owner is the view player. esi = unit; the four arguments are pushed; the callee pops them.
static __declspec(naked) void DeathEyeStub()
{
	__asm
	{
		mov   eax, [esi + 0x104]
		and   eax, 0x7FFFFFFF
		jz    eye
		cmp   eax, 0x7F800000
		ja    eye
		add   esp, 0x10
		jmp   dword ptr [g_deathEyeResume]
	eye:
		call  dword ptr [g_addEyeball]
		jmp   dword ptr [g_deathEyeResume]
	}
}

namespace
{
	// Expected TotalA.exe bytes; identical in Escalation 10.2 and Cavedog's v3.1. Each span is longer than
	// its splice so that it also pins the register state the stub relies on.

	// 0x482AC0..0x482B44: Los_RevealNewUnit from its entry to its live-sight test.
	const BYTE kRevealNewUnitHead[] =
	{
		0x83, 0xEC, 0x24, 0x8B, 0x44, 0x24, 0x28, 0x53, 0x56, 0x57, 0x8B, 0x88, 0x96, 0x00, 0x00, 0x00,
		0x8D, 0x50, 0x7A, 0x89, 0x4C, 0x24, 0x0C, 0x8B, 0x88, 0x92, 0x00, 0x00, 0x00, 0x89, 0x54, 0x24,
		0x10, 0x8D, 0xB0, 0xF8, 0x00, 0x00, 0x00, 0x66, 0x8B, 0x91, 0x02, 0x02, 0x00, 0x00, 0x83, 0xC0,
		0x6A, 0x66, 0x89, 0x54, 0x24, 0x14, 0x89, 0x74, 0x24, 0x18, 0x8B, 0x10, 0x89, 0x54, 0x24, 0x1C,
		0x8B, 0x50, 0x04, 0x89, 0x54, 0x24, 0x20, 0x8B, 0x40, 0x08, 0x89, 0x44, 0x24, 0x24, 0x8A, 0x89,
		0x70, 0x01, 0x00, 0x00, 0x88, 0x4C, 0x24, 0x16, 0x8B, 0x0D, 0xE8, 0x1D, 0x51, 0x00, 0x33, 0xC0,
		0x8A, 0x81, 0x7F, 0x42, 0x01, 0x00, 0x40, 0xC1, 0xE0, 0x10, 0x3B, 0xD0, 0x7D, 0x04, 0x89, 0x44,
		0x24, 0x20, 0x8A, 0x91, 0x81, 0x42, 0x01, 0x00, 0x80, 0xE2, 0x02, 0x80, 0xFA, 0x02, 0x0F, 0x85,
		0xC9, 0x00, 0x00, 0x00,
	};

	// 0x4827B0..0x48282F: the whole of Los_UpdateUnitFromPosition (calls 0x4825B0).
	const BYTE kUpdateUnitSight[] =
	{
		0x83, 0xEC, 0x24, 0x8B, 0x44, 0x24, 0x28, 0x8B, 0x88, 0x96, 0x00, 0x00, 0x00, 0x8D, 0x50, 0x7A,
		0x89, 0x4C, 0x24, 0x00, 0x8B, 0x88, 0x92, 0x00, 0x00, 0x00, 0x89, 0x54, 0x24, 0x04, 0x66, 0x8B,
		0x91, 0x02, 0x02, 0x00, 0x00, 0x66, 0x89, 0x54, 0x24, 0x08, 0x8D, 0x90, 0xF8, 0x00, 0x00, 0x00,
		0x83, 0xC0, 0x6A, 0x89, 0x54, 0x24, 0x0C, 0x8B, 0x10, 0x89, 0x54, 0x24, 0x10, 0x8B, 0x50, 0x04,
		0x89, 0x54, 0x24, 0x14, 0x8B, 0x40, 0x08, 0x89, 0x44, 0x24, 0x18, 0x8A, 0x89, 0x70, 0x01, 0x00,
		0x00, 0x88, 0x4C, 0x24, 0x0A, 0x8B, 0x0D, 0xE8, 0x1D, 0x51, 0x00, 0x33, 0xC0, 0x8A, 0x81, 0x7F,
		0x42, 0x01, 0x00, 0x40, 0xC1, 0xE0, 0x10, 0x3B, 0xD0, 0x7D, 0x04, 0x89, 0x44, 0x24, 0x14, 0x8D,
		0x54, 0x24, 0x00, 0x52, 0xE8, 0x87, 0xFD, 0xFF, 0xFF, 0x83, 0xC4, 0x24, 0xC2, 0x04, 0x00,
	};

	// 0x482090..0x48210F: the whole of Los_DecUnitFogSightCounts (calls 0x481D50).
	const BYTE kRemoveUnitSight[] =
	{
		0x83, 0xEC, 0x24, 0x8B, 0x44, 0x24, 0x28, 0x8B, 0x88, 0x96, 0x00, 0x00, 0x00, 0x8D, 0x50, 0x7A,
		0x89, 0x4C, 0x24, 0x00, 0x8B, 0x88, 0x92, 0x00, 0x00, 0x00, 0x89, 0x54, 0x24, 0x04, 0x66, 0x8B,
		0x91, 0x02, 0x02, 0x00, 0x00, 0x66, 0x89, 0x54, 0x24, 0x08, 0x8D, 0x90, 0xF8, 0x00, 0x00, 0x00,
		0x83, 0xC0, 0x6A, 0x89, 0x54, 0x24, 0x0C, 0x8B, 0x10, 0x89, 0x54, 0x24, 0x10, 0x8B, 0x50, 0x04,
		0x89, 0x54, 0x24, 0x14, 0x8B, 0x40, 0x08, 0x89, 0x44, 0x24, 0x18, 0x8A, 0x89, 0x70, 0x01, 0x00,
		0x00, 0x88, 0x4C, 0x24, 0x0A, 0x8B, 0x0D, 0xE8, 0x1D, 0x51, 0x00, 0x33, 0xC0, 0x8A, 0x81, 0x7F,
		0x42, 0x01, 0x00, 0x40, 0xC1, 0xE0, 0x10, 0x3B, 0xD0, 0x7D, 0x04, 0x89, 0x44, 0x24, 0x14, 0x8D,
		0x54, 0x24, 0x00, 0x52, 0xE8, 0x47, 0xFC, 0xFF, 0xFF, 0x83, 0xC4, 0x24, 0xC2, 0x04, 0x00,
	};

	// 0x48177C..0x4817AD: the rebuild's loop setup (ebp = first unit from edx = ta), head and body start.
	const BYTE kRebuildLoopHead[] =
	{
		0x8B, 0xAA, 0x57, 0x43, 0x01, 0x00, 0x8B, 0x82, 0x5B, 0x43, 0x01, 0x00, 0x81, 0xC5, 0x18, 0x01,
		0x00, 0x00, 0x3B, 0xE8, 0x0F, 0x87, 0x6F, 0x01, 0x00, 0x00, 0x66, 0x83, 0xBD, 0xA6, 0x00, 0x00,
		0x00, 0x00, 0x0F, 0x84, 0x4D, 0x01, 0x00, 0x00, 0x8B, 0x85, 0x96, 0x00, 0x00, 0x00, 0x8D, 0x4D,
		0x7A,
	};

	// 0x4818EB..0x481905: edx reload, next unit, back to the loop head.
	const BYTE kRebuildLoopTail[] =
	{
		0x8B, 0x15, 0xE8, 0x1D, 0x51, 0x00, 0x8B, 0x82, 0x5B, 0x43, 0x01, 0x00, 0x81, 0xC5, 0x18, 0x01,
		0x00, 0x00, 0x3B, 0xE8, 0x0F, 0x86, 0x91, 0xFE, 0xFF, 0xFF,
	};

	// 0x486706..0x486753: Unit_HandleKilledPacket's alive test (esi = unit), view-player test, the
	// AddEyeball arguments and call, and the instruction after it.
	const BYTE kKilledDeathEye[] =
	{
		0xF7, 0x86, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0F, 0x84, 0x43, 0x07, 0x00, 0x00,
		0x8B, 0x86, 0x96, 0x00, 0x00, 0x00, 0x8A, 0x88, 0x46, 0x01, 0x00, 0x00, 0x8A, 0x82, 0x43, 0x2A,
		0x00, 0x00, 0x3A, 0xC8, 0x75, 0x27, 0x8B, 0x86, 0x92, 0x00, 0x00, 0x00, 0x6A, 0x3C, 0x8D, 0x4E,
		0x6A, 0x0F, 0xBF, 0x90, 0x70, 0x01, 0x00, 0x00, 0x0F, 0xBF, 0x80, 0x02, 0x02, 0x00, 0x00, 0x52,
		0x50, 0x51, 0xE8, 0xC3, 0xC1, 0xFF, 0xFF, 0x8B, 0x15, 0xE8, 0x1D, 0x51, 0x00,
	};

	struct Span
	{
		DWORD addr;
		const BYTE* bytes;
		size_t len;
	};

	const Span kSpans[] =
	{
		{ 0x00482AC0u, kRevealNewUnitHead, sizeof(kRevealNewUnitHead) },
		{ 0x004827B0u, kUpdateUnitSight,   sizeof(kUpdateUnitSight) },
		{ 0x00482090u, kRemoveUnitSight,   sizeof(kRemoveUnitSight) },
		{ 0x0048177Cu, kRebuildLoopHead,   sizeof(kRebuildLoopHead) },
		{ 0x004818EBu, kRebuildLoopTail,   sizeof(kRebuildLoopTail) },
		{ 0x00486706u, kKilledDeathEye,    sizeof(kKilledDeathEye) },
	};

	struct Splice
	{
		DWORD addr;
		DWORD len;          // whole instructions; the bytes after the jump are NOPs
		void (*stub)();
	};

	const Splice kSplices[] =
	{
		{ 0x00482AC0u, 7, &RevealNewUnitStub },     // sub esp,24h / mov eax,[esp+28h]
		{ 0x004827B0u, 7, &UpdateUnitSightStub },   // same two instructions
		{ 0x00482090u, 7, &RemoveUnitSightStub },   // same two instructions
		{ 0x00481796u, 8, &RebuildLoopStub },       // cmp word [ebp+0A6h],0 (the je after it is never reached)
		{ 0x00486748u, 5, &DeathEyeStub },          // call AddEyeball
	};
	const size_t kSpliceCount = sizeof(kSplices) / sizeof(kSplices[0]);

	bool g_installed = false;
	std::unique_ptr<SingleHook> g_hooks[kSpliceCount];

	bool ReadBytes(DWORD addr, BYTE* out, size_t len)
	{
		__try
		{
			std::memcpy(out, reinterpret_cast<const void*>(addr), len);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	bool SpanMatches(const Span& span)
	{
		BYTE bytes[256];
		return span.len <= sizeof(bytes) && ReadBytes(span.addr, bytes, span.len) &&
			std::memcmp(bytes, span.bytes, span.len) == 0;
	}

	bool SpliceReadsBack(const Splice& splice)
	{
		BYTE bytes[16];
		if (splice.len > sizeof(bytes) || !ReadBytes(splice.addr, bytes, splice.len) || bytes[0] != 0xE9)
			return false;
		DWORD rel = 0;
		std::memcpy(&rel, bytes + 1, sizeof(rel));
		if (rel != reinterpret_cast<DWORD>(splice.stub) - (splice.addr + 5))
			return false;
		for (DWORD i = 5; i < splice.len; ++i)
			if (bytes[i] != 0x90)
				return false;
		return true;
	}

	void RemoveAll()
	{
		for (size_t i = kSpliceCount; i-- > 0; )
			g_hooks[i].reset();
	}
}

namespace NanoframeNoLos
{
	void Install()
	{
		if (g_installed)
			return;

		for (const Span& span : kSpans)
		{
			if (!SpanMatches(span))
			{
				IDDrawSurface::OutptFmtTxt("[NanoframeNoLos] DISABLED: unexpected TotalA.exe bytes in 0x%08X..0x%08X",
					span.addr, span.addr + static_cast<DWORD>(span.len));
				return;
			}
		}

		for (size_t i = 0; i < kSpliceCount; ++i)
		{
			const Splice& splice = kSplices[i];
			g_hooks[i].reset(new SingleHook(splice.addr, splice.len, INLINE_SINGLEJMP,
				reinterpret_cast<LPBYTE>(splice.stub)));
			FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(splice.addr), splice.len);
		}

		// SingleHook::Hook() reports nothing: compare memory with what was asked for.
		for (const Splice& splice : kSplices)
		{
			if (!SpliceReadsBack(splice))
			{
				RemoveAll();
				IDDrawSurface::OutptFmtTxt("[NanoframeNoLos] DISABLED: the splice at 0x%08X did not read back; all restored",
					splice.addr);
				return;
			}
		}

		g_installed = true;
		IDDrawSurface::OutptTxt("[NanoframeNoLos] installed: 0x482AC0 0x4827B0 0x482090 0x481796 0x486748, read back OK");
	}

	void Shutdown()
	{
		RemoveAll();
		g_installed = false;
	}
}

#endif // NANOFRAME_NO_LOS_ENABLE
