#include "EngineLimits.h"

#include <windows.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <vector>

#include "hook/etc.h"
#include "iddrawsurface.h"
#include "tamem.h"

namespace
{
	const DWORD STOCK_PROJECTILE_LIMIT = 300;
	const DWORD STOCK_EXPLOSION_LIMIT = 300;
	const DWORD STOCK_MODEL_EFFECT_LIMIT = 100;
	const DWORD STOCK_MODEL_EFFECT_POOL_BYTES = 0x000186A0;
	const DWORD PROJECTILE_SIZE = 0x6B;
	const DWORD PROJECTILE_CLEANUP_STACK_BYTES =
		EngineLimits::PROJECTILE_LIMIT * 4 + 16;
	const DWORD MODEL_EFFECT_POOL_BYTES = STOCK_MODEL_EFFECT_POOL_BYTES
		* EngineLimits::MODEL_EFFECT_LIMIT / STOCK_MODEL_EFFECT_LIMIT;
	const int AUX_EFFECT_VERTEX_COUNT = 8;
	const int AUX_EFFECT_FACE_COUNT = 6;

	struct ExpandedExplosionPool
	{
		int count;
		ExplosionStruct entries[EngineLimits::EXPLOSION_LIMIT];
	};

	ExpandedExplosionPool s_explosionPool = {};
	DWORD s_modelEffectSlots[EngineLimits::MODEL_EFFECT_LIMIT] = {};
	DebrisStruct s_auxEffectSlots[EngineLimits::AUX_EFFECT_LIMIT] = {};
	Point3 s_auxEffectVertices
		[EngineLimits::AUX_EFFECT_LIMIT][AUX_EFFECT_VERTEX_COUNT] = {};
	Unk1Struct s_auxEffectFaces
		[EngineLimits::AUX_EFFECT_LIMIT][AUX_EFFECT_FACE_COUNT] = {};
	int s_nextAuxEffectSlot = 0;
	bool s_installed = false;
	bool s_installFailed = false;
	char s_installFailure[256] = {};
	DWORD s_projectileCleanupContinue = 0x0049AE26;

	void WriteDword(BYTE* destination, DWORD value)
	{
		memcpy(destination, &value, sizeof(value));
	}

	void InitializeAuxEffectPool()
	{
		memset(s_auxEffectSlots, 0, sizeof(s_auxEffectSlots));
		memset(s_auxEffectVertices, 0, sizeof(s_auxEffectVertices));
		memset(s_auxEffectFaces, 0, sizeof(s_auxEffectFaces));

		for (int i = 0; i < EngineLimits::AUX_EFFECT_LIMIT; ++i)
		{
			DebrisStruct& debris = s_auxEffectSlots[i];
			BYTE* slot = reinterpret_cast<BYTE*>(&debris);
			slot[0] = 0xFF;
			WriteDword(slot + 0x04, AUX_EFFECT_VERTEX_COUNT);
			WriteDword(slot + 0x08, AUX_EFFECT_FACE_COUNT);
			WriteDword(slot + 0x0C, 0xFFFFFFFF);
			WriteDword(slot + 0x1C, 0x00502C28);
			WriteDword(slot + 0x20, 0);
			debris.Vertices = s_auxEffectVertices[i];
			debris.Unk = s_auxEffectFaces[i];

			for (int record = 0; record < AUX_EFFECT_FACE_COUNT; ++record)
			{
				BYTE* companion = reinterpret_cast<BYTE*>(
					&s_auxEffectFaces[i][record]);
				WriteDword(companion + 0x00, 0xC8);
				WriteDword(companion + 0x04, 4);
				WriteDword(companion + 0x0C, 0x00502BF8 + record * 8);
				WriteDword(companion + 0x1C, 1);
			}
		}

		s_nextAuxEffectSlot = 0;
	}

	void ResetEffectPoolsForGame()
	{
		memset(s_modelEffectSlots, 0, sizeof(s_modelEffectSlots));
		InitializeAuxEffectPool();
	}

	__declspec(noinline) DebrisStruct* AllocateAuxEffectSlot()
	{
		// Round-robin avoids rescanning a busy prefix; this pool is visual-only,
		// so allocation order cannot affect simulation determinism.
		int index = s_nextAuxEffectSlot;
		for (int checked = 0; checked < EngineLimits::AUX_EFFECT_LIMIT; ++checked)
		{
			DebrisStruct* debris = &s_auxEffectSlots[index];
			BYTE* slot = reinterpret_cast<BYTE*>(debris);
			if (slot[0] == 0xFF)
			{
				slot[0] = 0;
				++index;
				if (index == EngineLimits::AUX_EFFECT_LIMIT)
					index = 0;
				s_nextAuxEffectSlot = index;
				return debris;
			}

			++index;
			if (index == EngineLimits::AUX_EFFECT_LIMIT)
				index = 0;
		}

		return NULL;
	}

	// ProjectileEarsing originally reserves less than one stack page.  The
	// expanded index arrays need 0x2EF0 bytes, so probe each intervening page
	// before applying the new stack pointer.  The entry patch jumps here instead
	// of running the stock SUB ESP,0x4C0 instruction.
	__declspec(naked) void ProbeProjectileCleanupStack()
	{
		__asm
		{
			push eax
			mov eax, esp
			sub eax, 0x1000
			test dword ptr [eax], eax
			sub eax, 0x1000
			test dword ptr [eax], eax
			sub eax, 0x0EF0
			test dword ptr [eax], eax
			pop eax
			sub esp, 0x2EF0
			jmp dword ptr [s_projectileCleanupContinue]
		}
	}

	__declspec(naked) void LoadExplosionModelTableEntry()
	{
		__asm
		{
			push edx
			mov edx, dword ptr ds:[0x00511DE8]
			mov eax, dword ptr [edx + eax * 4 + 0x0001AB8F]
			pop edx
			ret
		}
	}

	struct Patch
	{
		DWORD address;
		std::vector<BYTE> expected;
		std::vector<BYTE> replacement;
		const char* name;
	};
	std::vector<Patch> s_appliedPatches;

	void RecordInstallFailure(const char* format, ...)
	{
		if (s_installFailed)
			return;

		va_list args;
		va_start(args, format);
		_vsnprintf_s(s_installFailure, sizeof(s_installFailure), _TRUNCATE,
			format, args);
		va_end(args);
		s_installFailed = true;
		IDDrawSurface::OutptFmtTxt("[EngineLimits] %s", s_installFailure);
	}

	std::vector<BYTE> DwordBytes(DWORD value)
	{
		std::vector<BYTE> bytes(sizeof(value));
		memcpy(&bytes[0], &value, sizeof(value));
		return bytes;
	}

	std::vector<BYTE> BytesWithDword(const BYTE* prefix, size_t prefixSize,
		DWORD value, const BYTE* suffix = NULL, size_t suffixSize = 0)
	{
		std::vector<BYTE> bytes(prefix, prefix + prefixSize);
		std::vector<BYTE> valueBytes = DwordBytes(value);
		bytes.insert(bytes.end(), valueBytes.begin(), valueBytes.end());
		if (suffix && suffixSize)
			bytes.insert(bytes.end(), suffix, suffix + suffixSize);
		return bytes;
	}

	void AddDwordPatch(std::vector<Patch>& patches, DWORD address,
		DWORD expected, DWORD replacement, const char* name)
	{
		Patch patch = { address, DwordBytes(expected), DwordBytes(replacement), name };
		patches.push_back(patch);
	}

	void AddBytesPatch(std::vector<Patch>& patches, DWORD address,
		const std::vector<BYTE>& expected, const std::vector<BYTE>& replacement,
		const char* name)
	{
		Patch patch = { address, expected, replacement, name };
		patches.push_back(patch);
	}

	bool MemoryEquals(DWORD address, const BYTE* expected, size_t size)
	{
		__try
		{
			return memcmp(reinterpret_cast<const void*>(address), expected, size) == 0;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	bool ValidatePatches(const std::vector<Patch>& patches)
	{
		for (size_t i = 0; i < patches.size(); ++i)
		{
			const Patch& patch = patches[i];
			if (patch.expected.size() != patch.replacement.size() ||
				!MemoryEquals(patch.address, &patch.expected[0], patch.expected.size()))
			{
				RecordInstallFailure(
					"address validation failed for %s at 0x%08X",
					patch.name, patch.address);
				return false;
			}
		}
		return true;
	}

	bool ApplyPatch(const Patch& patch)
	{
		if (!MemWriteWithBackup(reinterpret_cast<void*>(patch.address),
			patch.replacement.size(), NULL,
			const_cast<BYTE*>(&patch.replacement[0])))
		{
			return false;
		}
		FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(patch.address),
			patch.replacement.size());
		return true;
	}

	bool RestorePatch(const Patch& patch)
	{
		Patch restore = patch;
		restore.replacement = patch.expected;
		return ApplyPatch(restore);
	}

	void RestoreAppliedPatches()
	{
		for (size_t i = s_appliedPatches.size(); i > 0; --i)
		{
			if (!RestorePatch(s_appliedPatches[i - 1]))
			{
				IDDrawSurface::OutptFmtTxt(
					"[EngineLimits] restore failed for %s at 0x%08X",
					s_appliedPatches[i - 1].name,
					s_appliedPatches[i - 1].address);
			}
		}
		s_appliedPatches.clear();
	}

	void AddProjectilePatches(std::vector<Patch>& patches)
	{
		const DWORD allocationBytes = EngineLimits::PROJECTILE_LIMIT * PROJECTILE_SIZE;
		const DWORD clearDwords = allocationBytes / sizeof(DWORD);
		const DWORD cleanupSecondArrayOffset = 0x20 + EngineLimits::PROJECTILE_LIMIT * 2;

		AddDwordPatch(patches, 0x00499A32, 0x00007D64, allocationBytes,
			"projectile allocation size");
		AddDwordPatch(patches, 0x00499A56, 0x00001F59, clearDwords,
			"projectile allocation clear size");

		const DWORD capOperands[] = {
			0x0049B6F0, 0x0049B80A, 0x0049C9D2, 0x0049CC34, 0x0049CDF3,
			0x0049D011, 0x0049D2BE, 0x0049D4B5, 0x0049DD96, 0x0049DF24
		};
		for (size_t i = 0; i < sizeof(capOperands) / sizeof(capOperands[0]); ++i)
			AddDwordPatch(patches, capOperands[i], STOCK_PROJECTILE_LIMIT,
				EngineLimits::PROJECTILE_LIMIT, "projectile creation cap");

		const BYTE cleanupStackExpected[] = {
			0x81, 0xEC, 0xC0, 0x04, 0x00, 0x00
		};
		const BYTE cleanupStackPrefix[] = { 0xE9 };
		const BYTE cleanupStackSuffix[] = { 0x90 };
		const DWORD cleanupStackRelative = static_cast<DWORD>(
			reinterpret_cast<DWORD_PTR>(&ProbeProjectileCleanupStack))
			- (0x0049AE20 + 5);
		AddBytesPatch(patches, 0x0049AE20,
			std::vector<BYTE>(cleanupStackExpected,
				cleanupStackExpected + sizeof(cleanupStackExpected)),
			BytesWithDword(cleanupStackPrefix, sizeof(cleanupStackPrefix),
				cleanupStackRelative, cleanupStackSuffix,
				sizeof(cleanupStackSuffix)),
			"projectile cleanup stack probe");
		AddDwordPatch(patches, 0x0049AEB8, 0x00000278, cleanupSecondArrayOffset,
			"projectile cleanup moved-index write");
		AddDwordPatch(patches, 0x0049AF39, 0x00000278, cleanupSecondArrayOffset,
			"projectile cleanup moved-index read");
		AddDwordPatch(patches, 0x0049AF7F, 0x000004C0,
			PROJECTILE_CLEANUP_STACK_BYTES,
			"projectile cleanup stack release");
	}

	void AddExplosionPatches(std::vector<Patch>& patches)
	{
		const DWORD poolAddress = static_cast<DWORD>(
			reinterpret_cast<DWORD_PTR>(&s_explosionPool));

		const BYTE resetPrefix[] = { 0xC7, 0x05 };
		const BYTE resetSuffix[] = { 0x00, 0x00, 0x00, 0x00 };
		const BYTE resetExpected[] = {
			0xC7, 0x80, 0x1B, 0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00
		};
		AddBytesPatch(patches, 0x00420630,
			std::vector<BYTE>(resetExpected, resetExpected + sizeof(resetExpected)),
			BytesWithDword(resetPrefix, sizeof(resetPrefix), poolAddress,
				resetSuffix, sizeof(resetSuffix)),
			"explosion pool reset");

		const BYTE loadCountPrefix[] = { 0x8B, 0x0D };
		const BYTE loadCountExpected[] = { 0x8B, 0x88, 0x1B, 0x49, 0x01, 0x00 };
		AddBytesPatch(patches, 0x00420A36,
			std::vector<BYTE>(loadCountExpected, loadCountExpected + sizeof(loadCountExpected)),
			BytesWithDword(loadCountPrefix, sizeof(loadCountPrefix), poolAddress),
			"explosion add count");

		const BYTE movEdiPrefix[] = { 0xBF };
		const BYTE nopSuffix[] = { 0x90 };
		const BYTE leaEdiExpected[] = { 0x8D, 0xB8, 0x1B, 0x49, 0x01, 0x00 };
		AddBytesPatch(patches, 0x00420A3C,
			std::vector<BYTE>(leaEdiExpected, leaEdiExpected + sizeof(leaEdiExpected)),
			BytesWithDword(movEdiPrefix, sizeof(movEdiPrefix), poolAddress,
				nopSuffix, sizeof(nopSuffix)),
			"explosion add pool");

		const BYTE loadModelExpected[] = {
			0x8B, 0x84, 0x87, 0x74, 0x62, 0x00, 0x00
		};
		// Stock indexes this table relative to the inline pool, so relocation must
		// reload TA's current dynamic-memory pointer instead of using the new pool.
		const BYTE loadModelPrefix[] = { 0xE8 };
		const BYTE loadModelSuffix[] = { 0x90, 0x90 };
		const DWORD loadModelRelative = static_cast<DWORD>(
			reinterpret_cast<DWORD_PTR>(&LoadExplosionModelTableEntry))
			- (0x00420AA2 + 5);
		AddBytesPatch(patches, 0x00420AA2,
			std::vector<BYTE>(loadModelExpected,
				loadModelExpected + sizeof(loadModelExpected)),
			BytesWithDword(loadModelPrefix, sizeof(loadModelPrefix),
				loadModelRelative, loadModelSuffix, sizeof(loadModelSuffix)),
			"explosion model table");

		const BYTE loadEdxPrefix[] = { 0x8B, 0x15 };
		const BYTE loadEdxExpected[] = { 0x8B, 0x90, 0x1B, 0x49, 0x01, 0x00 };
		AddBytesPatch(patches, 0x00420B35,
			std::vector<BYTE>(loadEdxExpected, loadEdxExpected + sizeof(loadEdxExpected)),
			BytesWithDword(loadEdxPrefix, sizeof(loadEdxPrefix), poolAddress),
			"explosion update count");

		const BYTE movEcxPrefix[] = { 0xB9 };
		const BYTE leaEcxExpected[] = { 0x8D, 0x88, 0x1B, 0x49, 0x01, 0x00 };
		AddBytesPatch(patches, 0x00420B3B,
			std::vector<BYTE>(leaEcxExpected, leaEcxExpected + sizeof(leaEcxExpected)),
			BytesWithDword(movEcxPrefix, sizeof(movEcxPrefix), poolAddress,
				nopSuffix, sizeof(nopSuffix)),
			"explosion update pool");

		const BYTE movEbxPrefix[] = { 0xBB };
		const BYTE leaEbxExpected[] = { 0x8D, 0x98, 0x1B, 0x49, 0x01, 0x00 };
		AddBytesPatch(patches, 0x00420F66,
			std::vector<BYTE>(leaEbxExpected, leaEbxExpected + sizeof(leaEbxExpected)),
			BytesWithDword(movEbxPrefix, sizeof(movEbxPrefix), poolAddress,
				nopSuffix, sizeof(nopSuffix)),
			"explosion draw pool");

		const BYTE movEsiPrefix[] = { 0xBE };
		const BYTE leaEsiExpected[] = { 0x8D, 0xB0, 0x1B, 0x49, 0x01, 0x00 };
		AddBytesPatch(patches, 0x00421738,
			std::vector<BYTE>(leaEsiExpected, leaEsiExpected + sizeof(leaEsiExpected)),
			BytesWithDword(movEsiPrefix, sizeof(movEsiPrefix), poolAddress,
				nopSuffix, sizeof(nopSuffix)),
			"piece explosion pool");

		AddDwordPatch(patches, 0x00420A44, STOCK_EXPLOSION_LIMIT,
			EngineLimits::EXPLOSION_LIMIT, "explosion creation cap");
		AddDwordPatch(patches, 0x00421771, STOCK_EXPLOSION_LIMIT,
			EngineLimits::EXPLOSION_LIMIT, "piece explosion creation cap");
	}

	void AddModelEffectPatches(std::vector<Patch>& patches)
	{
		const DWORD baseAddress = static_cast<DWORD>(
			reinterpret_cast<DWORD_PTR>(&s_modelEffectSlots[0]));
		const DWORD endAddress = static_cast<DWORD>(
			reinterpret_cast<DWORD_PTR>(&s_modelEffectSlots[EngineLimits::MODEL_EFFECT_LIMIT]));
		const BYTE poolSizePrefix[] = { 0x68 };
		const BYTE poolSizeExpected[] = { 0x68, 0xA0, 0x86, 0x01, 0x00 };
		AddBytesPatch(patches, 0x004208FB,
			std::vector<BYTE>(poolSizeExpected,
				poolSizeExpected + sizeof(poolSizeExpected)),
			BytesWithDword(poolSizePrefix, sizeof(poolSizePrefix),
				MODEL_EFFECT_POOL_BYTES),
			"model effect backing pool size");

		const BYTE resetExpected[] = {
			0xB9, 0x64, 0x00, 0x00, 0x00,
			0x33, 0xC0,
			0xBF, 0xF0, 0x1D, 0x51, 0x00,
			0xF3, 0xAB
		};
		std::vector<BYTE> resetReplacement;
		resetReplacement.push_back(0xE8);
		const DWORD resetRelative = static_cast<DWORD>(
			reinterpret_cast<DWORD_PTR>(&ResetEffectPoolsForGame))
			- (0x0042090A + 5);
		std::vector<BYTE> resetBytes = DwordBytes(resetRelative);
		resetReplacement.insert(resetReplacement.end(),
			resetBytes.begin(), resetBytes.end());
		while (resetReplacement.size() < sizeof(resetExpected))
			resetReplacement.push_back(0x90);
		AddBytesPatch(patches, 0x0042090A,
			std::vector<BYTE>(resetExpected, resetExpected + sizeof(resetExpected)),
			resetReplacement, "effect pools reset");

		const DWORD baseOperands[] = {
			0x00420B08, 0x00420F38, 0x00421153,
			0x00421172, 0x004211A7, 0x0042165F, 0x00421680
		};
		for (size_t i = 0; i < sizeof(baseOperands) / sizeof(baseOperands[0]); ++i)
			AddDwordPatch(patches, baseOperands[i], 0x00511DF0, baseAddress,
				"model effect slots base");

		const DWORD endOperands[] = {
			0x00420B28, 0x00420F53, 0x00421162,
			0x0042118D, 0x004211C3, 0x0042166D
		};
		for (size_t i = 0; i < sizeof(endOperands) / sizeof(endOperands[0]); ++i)
			AddDwordPatch(patches, endOperands[i], 0x00511F80, endAddress,
				"model effect slots end");
	}

	void AddAuxEffectPatches(std::vector<Patch>& patches)
	{
		// These control-flow patches intentionally replace the stock allocation
		// loops wholesale.  A normal trampoline hook would execute the displaced
		// loop again and continue using TA's fixed 300-entry array.
		const BYTE allocatorExpected[] = {
			0x8B, 0x15, 0xE8, 0x1D, 0x51, 0x00
		};
		const DWORD allocatorRelative = static_cast<DWORD>(
			reinterpret_cast<DWORD_PTR>(&AllocateAuxEffectSlot))
			- (0x00420920 + 5);
		const BYTE jumpPrefix[] = { 0xE9 };
		const BYTE jumpSuffix[] = { 0x90 };
		AddBytesPatch(patches, 0x00420920,
			std::vector<BYTE>(allocatorExpected,
				allocatorExpected + sizeof(allocatorExpected)),
			BytesWithDword(jumpPrefix, sizeof(jumpPrefix), allocatorRelative,
				jumpSuffix, sizeof(jumpSuffix)),
			"auxiliary effect allocator");

		const BYTE inlineExpected[] = {
			0x8B, 0x15, 0xE8, 0x1D, 0x51, 0x00,
			0x33, 0xF6,
			0x33, 0xC0,
			0x8D, 0x8A, 0x9F, 0xAB, 0x01, 0x00
		};
		std::vector<BYTE> inlineReplacement;
		inlineReplacement.push_back(0x33);
		inlineReplacement.push_back(0xF6);
		inlineReplacement.push_back(0xE8);
		const DWORD callRelative = static_cast<DWORD>(
			reinterpret_cast<DWORD_PTR>(&AllocateAuxEffectSlot))
			- (0x004217E0 + 5);
		std::vector<BYTE> callBytes = DwordBytes(callRelative);
		inlineReplacement.insert(inlineReplacement.end(),
			callBytes.begin(), callBytes.end());
		inlineReplacement.push_back(0xE9);
		const DWORD continueRelative = 0x00421804 - (0x004217E5 + 5);
		std::vector<BYTE> continueBytes = DwordBytes(continueRelative);
		inlineReplacement.insert(inlineReplacement.end(),
			continueBytes.begin(), continueBytes.end());
		while (inlineReplacement.size() < sizeof(inlineExpected))
			inlineReplacement.push_back(0x90);

		AddBytesPatch(patches, 0x004217DE,
			std::vector<BYTE>(inlineExpected,
				inlineExpected + sizeof(inlineExpected)),
			inlineReplacement,
			"piece auxiliary effect allocator");
	}
}

void EngineLimits::Install()
{
	if (s_installed || s_installFailed)
		return;
	TAdynmemStruct* taPtr = *reinterpret_cast<TAdynmemStruct**>(0x00511DE8);
	if (taPtr && taPtr->Projectiles)
	{
		RecordInstallFailure(
			"installation was attempted after TA allocated its projectile pool");
		return;
	}

	static_assert(PROJECTILE_CLEANUP_STACK_BYTES == 0x2EF0,
		"Update ProbeProjectileCleanupStack when the projectile limit changes");
	static_assert(MODEL_EFFECT_POOL_BYTES == 1000000,
		"Model effect backing pool must scale with its slot count");
	static_assert(sizeof(ProjectileStruct) == PROJECTILE_SIZE,
		"ProjectileStruct size must match TotalA.exe");
	static_assert(sizeof(ExplosionStruct) == 0x54,
		"ExplosionStruct size must match TotalA.exe");
	static_assert(sizeof(DebrisStruct) == 0x34,
		"DebrisStruct size must match TotalA.exe");
	static_assert(sizeof(Point3) == 0x0C,
		"Point3 size must match TotalA.exe");
	static_assert(sizeof(Unk1Struct) == 0x20,
		"Unk1Struct size must match TotalA.exe");
	static_assert(sizeof(void*) == 4, "Engine limits require a 32-bit build");

	InitializeAuxEffectPool();

	std::vector<Patch> patches;
	AddProjectilePatches(patches);
	AddExplosionPatches(patches);
	AddModelEffectPatches(patches);
	AddAuxEffectPatches(patches);

	if (!ValidatePatches(patches))
		return;

	s_appliedPatches.reserve(patches.size());
	for (size_t i = 0; i < patches.size(); ++i)
	{
		if (!ApplyPatch(patches[i]))
		{
			RestoreAppliedPatches();
			RecordInstallFailure("write failed for %s at 0x%08X",
				patches[i].name, patches[i].address);
			return;
		}
		s_appliedPatches.push_back(patches[i]);
	}

	s_installed = true;
	IDDrawSurface::OutptFmtTxt(
		"[EngineLimits] installed: projectiles=%d explosions=%d model-effects=%d aux-effects=%d",
		PROJECTILE_LIMIT, EXPLOSION_LIMIT, MODEL_EFFECT_LIMIT, AUX_EFFECT_LIMIT);
}

void EngineLimits::Uninstall()
{
	if (!s_installed)
		return;

	RestoreAppliedPatches();
	s_installed = false;
	IDDrawSurface::OutptTxt("[EngineLimits] uninstalled");
}

void EngineLimits::AbortIfInstallFailed()
{
	if (!s_installFailed)
		return;

	char message[512] = {};
	_snprintf_s(message, sizeof(message), _TRUNCATE,
		"TADR could not safely install its engine-limit patches and Total "
		"Annihilation must close to prevent multiplayer desynchronization.\n\n%s\n\n"
		"See tdrawlog.txt for details.", s_installFailure);
	MessageBoxA(NULL, message, "TADR engine-limit error",
		MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
	ExitProcess(ERROR_BAD_EXE_FORMAT);
}

bool EngineLimits::IsInstalled()
{
	return s_installed;
}
