#include "HealthBarColors.h"

#if HEALTH_BAR_COLORS_ENABLE

#include <windows.h>

#include <cstddef>
#include <cstring>
#include <memory>

#include "hook/hook.h"
#include "iddrawsurface.h"
#include "tamem.h"

static_assert(offsetof(UnitStruct, UnitType) == 0x92,  "UnitStruct::UnitType moved");
static_assert(offsetof(UnitStruct, Health)   == 0x108, "UnitStruct::Health moved");

namespace
{
	// Unit_DrawHealthBar; its only caller is the call at 0x469CB9.
	const DWORD kDrawHealthBarAddr = 0x0046A430u;
	const DWORD kSpliceLen         = 5u;
	const BYTE  kPrologue[19] =
	{
		0x83, 0xEC, 0x10, 0x53, 0x55, 0x56, 0x8B, 0x74, 0x24, 0x24, 0x57, 0x66, 0x83, 0xBE, 0x08, 0x01, 0x00, 0x00, 0x00
	};

	// Gfx_FillRect: __stdcall(OFFSCREEN*, RECT*, unsigned char); rectangles are inclusive.
	const DWORD kFillRectAddr = 0x004BF6F0u;
	typedef int (__stdcall *FillRectFn)(OFFSCREEN*, RECT*, unsigned char);

	const DWORD kGameStatePtrAddr   = 0x00511DE8u;   // ta
	const DWORD kColorTableOffset   = 0x0DCBu;       // ta + 0xDCB

	// Colour-table slots, in order of falling health.
	const int kSlotBackground = 0;
	const int kSlotGreen      = 10;
	const int kSlotLime       = 90;
	const int kSlotYellow     = 14;
	const int kSlotOrange     = 59;
	const int kSlotRed        = 32;

	bool g_installed = false;
	std::unique_ptr<SingleHook> g_hook;

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
}

namespace HealthBarColors
{
	// External linkage so that the link map lists it.
	void __stdcall DrawHealthBar(OFFSCREEN* surface, UnitStruct* unit, int centerX, int y)
	{
		const int health = unit->Health;
		if (health <= 0 || !unit->UnitType)
			return;
		// The engine reads nMaxHP as a dword (nMaxHP + data8), not the unsigned short tamem.h declares.
		const DWORD maxHp = *reinterpret_cast<const DWORD*>(&unit->UnitType->nMaxHP);
		if (maxHp == 0)
			return;

		const BYTE* colors = *reinterpret_cast<const BYTE* const*>(kGameStatePtrAddr) + kColorTableOffset;
		const FillRectFn fillRect = reinterpret_cast<FillRectFn>(kFillRectAddr);

		RECT rect = { centerX - 17, y - 2, centerX + 17, y + 2 };
		fillRect(surface, &rect, colors[kSlotBackground]);

		// health <= 32767, so health * 100 and health << 5 cannot overflow.
		const DWORD percent = static_cast<DWORD>(health) * 100u / maxHp;
		int slot;
		if (percent >= 80u)      slot = kSlotGreen;
		else if (percent >= 60u) slot = kSlotLime;
		else if (percent >= 40u) slot = kSlotYellow;
		else if (percent >= 20u) slot = kSlotOrange;
		else                     slot = kSlotRed;

		rect.left += 1;
		rect.top += 1;
		rect.bottom -= 1;
		rect.right = rect.left + static_cast<int>((static_cast<DWORD>(health) << 5) / maxHp);
		fillRect(surface, &rect, colors[slot]);
	}

	void Install()
	{
		if (g_installed)
			return;

		BYTE bytes[sizeof(kPrologue)];
		if (!ReadBytes(kDrawHealthBarAddr, bytes, sizeof(bytes)) || std::memcmp(bytes, kPrologue, sizeof(bytes)) != 0)
		{
			IDDrawSurface::OutptFmtTxt("[HealthBarColors] DISABLED: unexpected TotalA.exe bytes at 0x%08X", kDrawHealthBarAddr);
			return;
		}

		const DWORD target = reinterpret_cast<DWORD>(&DrawHealthBar);
		g_hook.reset(new SingleHook(kDrawHealthBarAddr, kSpliceLen, INLINE_SINGLEJMP, reinterpret_cast<LPBYTE>(&DrawHealthBar)));
		FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(kDrawHealthBarAddr), sizeof(kPrologue));

		// SingleHook::Hook() reports nothing: compare memory with what was asked for.
		DWORD rel = 0;
		bool ok = ReadBytes(kDrawHealthBarAddr, bytes, sizeof(bytes));
		if (ok)
			std::memcpy(&rel, bytes + 1, sizeof(rel));
		ok = ok && bytes[0] == 0xE9 && rel == target - (kDrawHealthBarAddr + kSpliceLen) &&
			std::memcmp(bytes + kSpliceLen, kPrologue + kSpliceLen, sizeof(kPrologue) - kSpliceLen) == 0;
		if (!ok)
		{
			g_hook.reset();
			IDDrawSurface::OutptTxt("[HealthBarColors] DISABLED: patched bytes did not read back; restored");
			return;
		}

		g_installed = true;
		IDDrawSurface::OutptFmtTxt("[HealthBarColors] installed: 0x%08X -> 0x%08X, read back OK", kDrawHealthBarAddr, target);
	}

	void Shutdown()
	{
		g_hook.reset();
		g_installed = false;
	}
}

#endif // HEALTH_BAR_COLORS_ENABLE
