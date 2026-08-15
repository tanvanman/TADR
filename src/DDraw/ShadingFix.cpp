#include "ShadingFix.h"

#include <windows.h>

#include <memory>

#include "TAConfig.h"
#include "hook/hook.h"
#include "iddrawsurface.h"

namespace
{
	const DWORD kShadeLevelAddr = 0x0045A2ECu;
	const DWORD kDefaultDarkVisibleShadeLevel = 5u;
	const DWORD kMinimumShadeLevel = 0u;
	const DWORD kMaximumShadeLevel = 31u;

	DWORD g_darkVisibleShadeLevel = kDefaultDarkVisibleShadeLevel;

	std::unique_ptr<InlineSingleHook> g_shadeLevelHook;

	int __stdcall ShadeLevelProc(PInlineX86StackBuffer pBuf)
	{
		// Stock shade level zero resolves ordinary source colours to black.
		// Preserve TA's normal lighting and only replace that all-black case.
		if (pBuf->Eax == 0 && g_darkVisibleShadeLevel != 0)
			pBuf->Eax = g_darkVisibleShadeLevel;
		return 0;
	}
}

namespace ShadingFix
{
	void Install()
	{
		if (g_shadeLevelHook)
			return;

		int configuredLevel = MyConfig != NULL
			? MyConfig->GetIniInt("ShadingZeroFallbackLevel", kDefaultDarkVisibleShadeLevel)
			: kDefaultDarkVisibleShadeLevel;
		if (configuredLevel < static_cast<int>(kMinimumShadeLevel))
			configuredLevel = kMinimumShadeLevel;
		else if (configuredLevel > static_cast<int>(kMaximumShadeLevel))
			configuredLevel = kMaximumShadeLevel;
		g_darkVisibleShadeLevel = static_cast<DWORD>(configuredLevel);
		if (g_darkVisibleShadeLevel == 0)
		{
			IDDrawSurface::OutptTxt(
				"[ShadingFix] disabled; stock shade level zero preserved");
			return;
		}

		g_shadeLevelHook.reset(new InlineSingleHook(
			kShadeLevelAddr, 5u, INLINE_5BYTESLAGGERJMP, ShadeLevelProc));
		IDDrawSurface::OutptFmtTxt(
			"[ShadingFix] installed; shade level zero falls back to %u",
			g_darkVisibleShadeLevel);
	}

	void Shutdown()
	{
		g_shadeLevelHook.reset();
	}
}
