#include "SelfHealRate.h"
#include "config.h"

#if SELFHEAL_RATE_ENABLE

#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "tamem.h"
#include "iddrawsurface.h"
#include "UnitDefExtensions.h"
#include "hook/hook.h"

// Every field this module reads, at the offset the engine's regen block reads it
// (TotalA.exe Escalation 10.2 GOLD, md5 1e677a7f92c79b5ab35440853d822c17).
static_assert(sizeof(UnitStruct) == 0x118, "UnitStruct stride");
static_assert(offsetof(UnitStruct, UnitType) == 0x92, "UnitStruct::UnitType (0x48AF37)");
static_assert(offsetof(UnitStruct, UnitInGameIndex) == 0xA8, "UnitStruct::UnitInGameIndex");
static_assert(offsetof(UnitStruct, Nanoframe) == 0x104, "UnitStruct::Nanoframe (0x48AF57)");
static_assert(offsetof(UnitStruct, Health) == 0x108, "UnitStruct::Health (0x48AF48)");
static_assert(offsetof(UnitDefStruct, damagemodifier) == 0x1AA, "UnitDefStruct::damagemodifier");
static_assert(offsetof(UnitDefStruct, buildtime) == 0x1EA, "UnitDefStruct::buildtime");
static_assert(offsetof(UnitDefStruct, nMaxHP) == 0x1FA, "UnitDefStruct::nMaxHP (0x48AF4F)");
static_assert(offsetof(UnitDefStruct, nHealTime) == 0x200, "UnitDefStruct::nHealTime (0x48AF3D)");
static_assert(offsetof(UnitDefStruct, UnitTypeID) == 0x21E, "UnitDefStruct::UnitTypeID");
static_assert(offsetof(TAdynmemStruct, UNITINFOCount) == 0x1438F, "TAdynmemStruct::UNITINFOCount");
static_assert(offsetof(TAdynmemStruct, UnitDef) == 0x1439B, "TAdynmemStruct::UnitDef");
static_assert(offsetof(TAdynmemStruct, GameTime) == 0x38A47, "TAdynmemStruct::GameTime (0x48AF67)");

static_assert(REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER >= 1 && REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER <= 100,
              "REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER out of range [1,100]");

namespace
{
	// ---- the engine block -------------------------------------------------------------------
	// 0x48AF37..0x48AF9A of Unit_TickAllPlayersScriptsAndWeapons (0x48AD30): HealTime != 0,
	// HP < nMaxHP (unsigned), Nanoframe == 0 (dword test of the float), (GameTime & HealTime &
	// 0xFF) == 0, t = floor(HealTime * 16 / 15), call 0x41BD10(esi, esi, (float)t). Esi is the
	// unit. The five branches into it (0x48AEE7, 0x48AEF1, 0x48AF04, 0x48AF17, 0x48AF28) land
	// on its first byte; no branch or absolute reference lands inside it (ta.inbound_branches,
	// three byte-scan hits at 0x48AF0F/0x48AF16/0x48AF71 are operand bytes); its four exits and
	// its fall-through land on kResume. kResume is `push esi; call 0x43B7C0`, and the engine
	// reaches it with four different eax/ecx/edx/flags states, so none of them is live there.
	constexpr DWORD  kBlock     = 0x0048AF37u;
	constexpr DWORD  kResume    = 0x0048AF9Bu;
	constexpr DWORD  kSpliceLen = 6u;               // mov ecx,[esi+0x92]

	const BYTE kBlockBytes[kResume - kBlock] =
	{
		0x8B, 0x8E, 0x92, 0x00, 0x00, 0x00, 0x0F, 0xBF, 0x81, 0x00, 0x02, 0x00, 0x00, 0x85, 0xC0, 0x74,
		0x53, 0x0F, 0xBF, 0x96, 0x08, 0x01, 0x00, 0x00, 0x3B, 0x91, 0xFA, 0x01, 0x00, 0x00, 0x73, 0x44,
		0x8B, 0x8E, 0x04, 0x01, 0x00, 0x00, 0x85, 0xC9, 0x75, 0x3A, 0x8B, 0x0D, 0xE8, 0x1D, 0x51, 0x00,
		0x84, 0x81, 0x47, 0x8A, 0x03, 0x00, 0x75, 0x2C, 0x90, 0xC1, 0xE0, 0x03, 0x8B, 0xC8, 0xB8, 0x89,
		0x88, 0x88, 0x88, 0xF7, 0xE9, 0x03, 0xD1, 0x51, 0xC1, 0xFA, 0x02, 0x8B, 0xC2, 0xC1, 0xE8, 0x1F,
		0x03, 0xD0, 0x89, 0x54, 0x24, 0x20, 0xDB, 0x44, 0x24, 0x20, 0xD9, 0x1C, 0x24, 0x56, 0x56, 0xE8,
		0x75, 0x0D, 0xF9, 0xFF
	};
	const BYTE kResumeBytes[6] = { 0x56, 0xE8, 0x1F, 0x08, 0xFB, 0xFF };   // push esi; call 0x43B7C0

	// Unit_ApplyDamageAndBroadcast 0x489BB0, type 0x0A: no scaling (0x489BC1), then
	// HP = min(HP + amount, nMaxHP) and nothing else (0x489D5F-0x489D8A). Not broadcast for a
	// local owner's unit (0x489C8E).
	typedef int (__stdcall* ApplyHeal_t)(void* src, void* target, int amount, int type, int unused);
	const ApplyHeal_t ApplyHeal = reinterpret_cast<ApplyHeal_t>(0x00489BB0u);

	constexpr int     kHealType     = 0x0A;
	constexpr int32_t kMaxRateMilli = 100000000;               // 100,000 HP/s; rate * Q stays below 2^59
	constexpr int64_t kRateDen      = 30LL * 65536LL * 1000LL; // ticks/s x Q16.16 one x milli: 1,966,080,000 < 2^31
	constexpr int64_t kMaxHealTick  = 65535;                   // the amount is a word in the packet (0x489C71)

	// ---- pure arithmetic (self-tested) ------------------------------------------------------
	// acc == nullptr only when the table is unavailable: round up then, never down, the same
	// invariant as RepairRateFix (a floor would stop slow regen entirely).

	// Rate key: raw HP this tick = (acc + rateMilli * Q) / (30 * 65536 * 1000); acc keeps the rest.
	int64_t RateStep(int32_t rateMilli, uint32_t q, int32_t* acc)
	{
		int64_t num = static_cast<int64_t>(rateMilli) * static_cast<int64_t>(q);
		if (!acc)
			return (num + kRateDen - 1) / kRateDen;
		int64_t a = *acc;
		if (a < 0 || a >= kRateDen)
			a = 0;                                          // stale value from another formula
		num += a;
		*acc = static_cast<int32_t>(num % kRateDen);
		return num / kRateDen;
	}

	// The key's text -> milli-HP per second, integer only (identical on every machine, no FPU).
	// Plain decimal: optional sign, digits, optional '.', at most 3 decimals (further digits are
	// ignored); parsing stops at the first other character, as atoi does. Negative -> 0; clamped
	// to kMaxRateMilli. "12.5" -> 12500, "0.001" -> 1, ".5" -> 500, "12,5" -> 12000, "1e3" -> 1000.
	int32_t ParseRateMilli(const char* s)
	{
		if (!s)
			return 0;
		while (*s == ' ' || *s == '\t')
			++s;
		bool neg = false;
		if (*s == '+' || *s == '-')
			neg = (*s++ == '-');
		bool any = false;
		int64_t whole = 0;
		for (; *s >= '0' && *s <= '9'; ++s)
		{
			any = true;
			if (whole <= kMaxRateMilli)
				whole = whole * 10 + (*s - '0');
		}
		int64_t frac = 0;
		int digits = 0;
		if (*s == '.')
		{
			for (++s; *s >= '0' && *s <= '9'; ++s)
			{
				any = true;
				if (digits < 3)
				{
					frac = frac * 10 + (*s - '0');
					++digits;
				}
			}
		}
		if (!any || neg)
			return 0;
		for (; digits < 3; ++digits)
			frac *= 10;
		const int64_t v = whole * 1000 + frac;
		return v > kMaxRateMilli ? kMaxRateMilli : static_cast<int32_t>(v);
	}

	// HealTime: RepairRateFix's exact accumulator for one regen firing.
	int64_t LegacyStep(int32_t maxHp, int32_t t, int32_t buildTime, int32_t mult, int32_t* acc)
	{
		const int64_t num = static_cast<int64_t>(maxHp) * t * mult;
		int64_t whole = num / buildTime;
		const int64_t rem = num % buildTime;
		if (!acc)
			return whole + (rem > 0 ? 1 : 0);
		int64_t a = *acc;
		if (a < 0 || a >= buildTime)
			a = 0;                                          // stale value from another formula
		a += rem;
		if (a >= buildTime)
		{
			a -= buildTime;
			whole += 1;
		}
		*acc = static_cast<int32_t>(a);
		return whole;
	}

	// The whole regen decision for one unit and one tick. Returns raw HP to add (0 = none).
	int64_t ComputeRegen(const UnitStruct* unit, const UnitDefStruct* def, uint8_t tickLow,
	                     int32_t rateMilli, int32_t* acc)
	{
		const int16_t healTime = static_cast<int16_t>(def->nHealTime);   // movsx, 0x48AF3D
		if (rateMilli <= 0 && healTime == 0)
			return 0;

		const uint32_t maxHp = *reinterpret_cast<const uint32_t*>(&def->nMaxHP);          // dword
		const int32_t  hp    = unit->Health;                                              // movsx
		const uint32_t frame = *reinterpret_cast<const uint32_t*>(&unit->Nanoframe);      // dword test
		// Unsigned, as the engine (0x48AF4F jae): a negative HP never regenerates.
		if (static_cast<uint32_t>(hp) >= maxHp || frame != 0)
		{
			if (acc)
				*acc = 0;          // a full or unfinished unit starts its next repair from zero
			return 0;
		}

		if (rateMilli > 0)
			return RateStep(rateMilli, def->damagemodifier, acc);

		if ((tickLow & static_cast<uint8_t>(healTime)) != 0)               // 0x48AF67
			return 0;
		const int32_t t = (static_cast<int32_t>(healTime) * 16) / 15;     // 0x48AF70..0x48AF87, all int16
		const int32_t buildTime = static_cast<int32_t>(def->buildtime);
		if (t <= 0 || buildTime <= 0)
			return 0;
		return LegacyStep(static_cast<int32_t>(maxHp), t, buildTime,
		                  REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER, acc);
	}

	// ---- per-unit accumulator, keyed by UnitInGameIndex (Unit+0xA8) --------------------------
	// Same lifetime rules as RepairRateFix's table: sized lazily from the engine's unit array,
	// re-checked every call, index 0 invalid. Units at full HP or unfinished reset their cell,
	// and a value out of range for the formula reading it is discarded, so a recycled index
	// inherits less than 1 raw HP.
	int32_t*    g_acc = nullptr;
	int         g_accUnits = 0;
	UnitStruct* g_accBase = nullptr;
	DWORD       g_fallbackHits = 0;
	DWORD       g_heals = 0;

	TAdynmemStruct* Ta()
	{
		return *reinterpret_cast<TAdynmemStruct**>(0x00511DE8u);
	}

	int32_t* AccFor(const UnitStruct* unit)
	{
		TAdynmemStruct* ta = Ta();
		UnitStruct* begin = ta ? ta->BeginUnitsArray_p : nullptr;
		UnitStruct* end   = ta ? ta->EndOfUnitsArray_p : nullptr;
		if (!begin || !end || end <= begin)
		{
			++g_fallbackHits;
			return nullptr;
		}
		if (!g_acc || begin != g_accBase)
		{
			const int units = static_cast<int>((reinterpret_cast<const char*>(end) -
			                                    reinterpret_cast<const char*>(begin)) / sizeof(UnitStruct));
			std::free(g_acc);
			g_acc = units > 0 ? static_cast<int32_t*>(std::calloc(static_cast<size_t>(units), sizeof(int32_t)))
			                  : nullptr;
			g_accUnits = g_acc ? units : 0;
			g_accBase  = g_acc ? begin : nullptr;
			if (!g_acc)
			{
				++g_fallbackHits;
				return nullptr;
			}
		}
		const int idx = unit->UnitInGameIndex;
		if (idx <= 0 || idx >= g_accUnits)
		{
			++g_fallbackHits;
			return nullptr;
		}
		return &g_acc[idx];
	}

	// ---- FBI key -> per-type rate table ------------------------------------------------------
	// The key is registered as a string with UnitDefExtensions (read from each FBI at load) and
	// parsed once into milli-HP/s per UnitTypeID. The table is rebuilt if the engine's unit-def
	// array or count changes; each build logs one line per self-healing unit type.
	UnitDefExtensions* g_ude = nullptr;
	unsigned           g_keyIdx = 0;
	int32_t*           g_rates = nullptr;
	unsigned           g_ratesSize = 0;
	bool               g_ratesBuilt = false;
	const void*        g_ratesDefs = nullptr;
	unsigned           g_ratesCount = 0;

	void BuildRates(TAdynmemStruct* ta)
	{
		std::free(g_rates);
		g_rates = nullptr;
		g_ratesSize = 0;
		g_ratesBuilt = true;
		g_ratesDefs = ta->UnitDef;
		g_ratesCount = ta->UNITINFOCount;
		if (!ta->UnitDef || ta->UNITINFOCount == 0)
			return;
		unsigned maxId = 0;
		for (unsigned i = 0; i < ta->UNITINFOCount; ++i)
			if (ta->UnitDef[i].UnitTypeID > maxId)
				maxId = ta->UnitDef[i].UnitTypeID;
		g_rates = static_cast<int32_t*>(std::calloc(maxId + 1u, sizeof(int32_t)));
		if (!g_rates)
			return;
		g_ratesSize = maxId + 1u;

		int nRate = 0, nLegacy = 0;
		for (unsigned i = 0; i < ta->UNITINFOCount; ++i)
		{
			const UnitDefStruct* d = &ta->UnitDef[i];
			const int32_t rate = ParseRateMilli(g_ude->getString(d->UnitTypeID, g_keyIdx).c_str());
			g_rates[d->UnitTypeID] = rate;
			const int16_t ht = static_cast<int16_t>(d->nHealTime);
			if (rate <= 0 && ht == 0)
				continue;
			char name[0x21];
			std::memcpy(name, d->UnitName, 0x20);
			name[0x20] = '\0';
			const uint32_t q = d->damagemodifier;
			if (rate > 0)
			{
				++nRate;
				const int64_t rawMilli = static_cast<int64_t>(rate) * q / 65536;   // raw HP/s x1000, log only
				IDDrawSurface::OutptFmtTxt("[SelfHealRate] %-16s %s=%d.%03d HP/s  Q=%u  raw=%lld.%03lld HP/s",
				                           name, SELFHEAL_RATE_FBI_KEY, rate / 1000, rate % 1000, q,
				                           rawMilli / 1000, rawMilli % 1000);
			}
			else
			{
				++nLegacy;
				IDDrawSurface::OutptFmtTxt("[SelfHealRate] %-16s HealTime=%d (HealTime formula, x%d, free)",
				                           name, ht, REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER);
			}
		}
		IDDrawSurface::OutptFmtTxt("[SelfHealRate] %d unit types on key %s, %d on the HealTime formula",
		                           nRate, SELFHEAL_RATE_FBI_KEY, nLegacy);
	}

	int32_t RateFor(TAdynmemStruct* ta, const UnitDefStruct* def)
	{
		if (!g_ratesBuilt || g_ratesDefs != ta->UnitDef || g_ratesCount != ta->UNITINFOCount)
			BuildRates(ta);
		const unsigned id = def->UnitTypeID;
		return id < g_ratesSize ? g_rates[id] : 0;
	}

	// ---- the splice target ------------------------------------------------------------------
	int __stdcall RegenTick(UnitStruct* unit)
	{
		const UnitDefStruct* def = unit ? unit->UnitType : nullptr;
		TAdynmemStruct* ta = Ta();
		if (!def || !ta)
			return 0;                        // the engine block would fault here
		const int32_t rate = RateFor(ta, def);
		if (rate <= 0 && def->nHealTime == 0)
			return 0;                        // no self-heal: the common case

		const uint8_t tickLow = static_cast<uint8_t>(ta->GameTime & 0xFF);
		int64_t whole = ComputeRegen(unit, def, tickLow, rate, AccFor(unit));
		if (whole <= 0)
			return 0;
		if (whole > kMaxHealTick)
			whole = kMaxHealTick;
		ApplyHeal(unit, unit, static_cast<int>(whole), kHealType, 0);
		++g_heals;
		return 1;
	}

	int  (__stdcall* g_pRegenTick)(UnitStruct*) = &RegenTick;
	DWORD g_resume = kResume;

	// Entered by the jmp at 0x48AF37 with the engine's stack and esi = unit. RegenTick is
	// stdcall (pops its argument, keeps ebx/esi/edi/ebp); eax/ecx/edx/flags are dead at kResume.
	__declspec(naked) void RegenBlockEntry()
	{
		__asm
		{
			push esi
			call dword ptr [g_pRegenTick]
			jmp  dword ptr [g_resume]
		}
	}

	// ---- install helpers --------------------------------------------------------------------
	bool ReadBytes(DWORD addr, void* out, size_t len)
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

	bool Expect(DWORD addr, const BYTE* expected, size_t len, const char* what)
	{
		BYTE cur[128];
		if (len <= sizeof(cur) && ReadBytes(addr, cur, len) && std::memcmp(cur, expected, len) == 0)
			return true;
		IDDrawSurface::OutptFmtTxt("[SelfHealRate] DISABLED: unexpected TotalA.exe bytes at 0x%08X (%s)", addr, what);
		return false;
	}

	bool SpliceReadsBack()
	{
		BYTE code[kSpliceLen];
		if (!ReadBytes(kBlock, code, sizeof(code)))
			return false;
		DWORD rel = 0;
		std::memcpy(&rel, code + 1, 4);
		return code[0] == 0xE9 && rel == reinterpret_cast<DWORD>(&RegenBlockEntry) - (kBlock + 5) && code[5] == 0x90;
	}

	// ---- self-test (pure arithmetic and the decision; touches no engine memory) -------------
	struct TestUnit
	{
		UnitStruct    unit;
		UnitDefStruct def;
	};

	void SetupTest(TestUnit& t, int16_t healTime, int32_t maxHp, int16_t hp, uint32_t q, int32_t buildTime)
	{
		std::memset(&t, 0, sizeof(t));
		t.unit.UnitType = &t.def;
		t.unit.Health = hp;
		*reinterpret_cast<uint32_t*>(&t.def.nMaxHP) = static_cast<uint32_t>(maxHp);
		t.def.nHealTime = static_cast<unsigned short>(healTime);
		t.def.damagemodifier = q;
		t.def.buildtime = static_cast<unsigned long>(buildTime);
	}

	int64_t RunTicks(TestUnit& t, int32_t rate, int32_t* acc, int ticks, int firstTick)
	{
		int64_t total = 0;
		for (int i = 0; i < ticks; ++i)
			total += ComputeRegen(&t.unit, &t.def, static_cast<uint8_t>((firstTick + i) & 0xFF), rate, acc);
		return total;
	}

	bool Check(bool ok, const char* what)
	{
		if (!ok)
			IDDrawSurface::OutptFmtTxt("[SelfHealRate][selftest] FAIL: %s", what);
		return ok;
	}

	bool RunSelfTest()
	{
		bool ok = true;
		TestUnit t;
		int32_t acc;

		// No self-heal at all: nothing, accumulator untouched.
		SetupTest(t, 0, 1000, 500, 65536, 1000);
		acc = 123;
		ok = Check(RunTicks(t, 0, &acc, 300, 0) == 0 && acc == 123, "no key, HealTime 0 -> nothing") && ok;

		// Full HP, unfinished, negative HP: nothing, accumulator reset.
		SetupTest(t, 3, 1000, 1000, 65536, 1000);
		acc = 7;
		ok = Check(RunTicks(t, 50, &acc, 30, 0) == 0 && acc == 0, "full HP -> nothing, acc reset") && ok;
		SetupTest(t, 3, 1000, 500, 65536, 1000);
		t.unit.Nanoframe = 0.5f;
		acc = 7;
		ok = Check(RunTicks(t, 50, &acc, 30, 0) == 0 && acc == 0, "nanoframe -> nothing, acc reset") && ok;
		SetupTest(t, 3, 1000, -5, 65536, 1000);
		ok = Check(RunTicks(t, 50, &acc, 30, 0) == 0, "negative HP -> nothing (unsigned gate)") && ok;

		// Parser: exact decimal, integer only.
		struct ParseCase { const char* text; int32_t milli; };
		static const ParseCase kParse[] =
		{
			{ "12.5", 12500 }, { "7", 7000 }, { "0.001", 1 }, { "0.0009", 0 }, { " \t3.25", 3250 }, { ".5", 500 },
			{ "5.", 5000 }, { "", 0 }, { "abc", 0 }, { "-2", 0 }, { "-0.5", 0 }, { "+4.2", 4200 }, { "1e3", 1000 },
			{ "12,5", 12000 }, { "99.9999", 99999 }, { "100000", kMaxRateMilli }, { "100000.001", kMaxRateMilli },
			{ "99999999999999999999", kMaxRateMilli }, { "0", 0 }, { "007.050", 7050 },
		};
		for (const ParseCase& c : kParse)
		{
			if (ParseRateMilli(c.text) != c.milli)
			{
				IDDrawSurface::OutptFmtTxt("[SelfHealRate][selftest] FAIL: parse \"%s\" -> %d, expected %d",
				                           c.text, ParseRateMilli(c.text), c.milli);
				ok = false;
			}
		}
		ok = Check(ParseRateMilli(nullptr) == 0, "parse null -> 0") && ok;

		// Rate key, DamageModifier 1: 30 HP/s = 1 HP every tick; 1 HP/s first HP on tick 30; 0.5 on tick 60.
		SetupTest(t, 0, 30000, 100, 65536, 1000);
		acc = 0;
		ok = Check(RunTicks(t, 30000, &acc, 30, 0) == 30 && acc == 0, "rate 30, Q 1.0 -> 30 HP in 30 ticks") && ok;
		acc = 0;
		ok = Check(RunTicks(t, 1000, &acc, 29, 0) == 0 && RunTicks(t, 1000, &acc, 1, 29) == 1,
		            "rate 1, Q 1.0 -> first HP on tick 30") && ok;
		acc = 0;
		ok = Check(RunTicks(t, 500, &acc, 59, 0) == 0 && RunTicks(t, 500, &acc, 1, 59) == 1,
		            "rate 0.5, Q 1.0 -> first HP on tick 60") && ok;
		acc = 0;
		ok = Check(RunTicks(t, 12345, &acc, 3000, 0) == 1234, "rate 12.345, 100 s -> 1234 HP (1234.5 owed)") && ok;

		// Same SelfHealRate, CORKROG (Q 8388) vs CORMS (Q 373), 10 minutes: raw totals are exact,
		// effective HP (raw * 65536 / Q) is the same within one raw HP of each.
		SetupTest(t, 3, 24480, 100, 8388, 1560065);
		acc = 0;
		const int64_t krog = RunTicks(t, 100000, &acc, 18000, 0);
		SetupTest(t, 3, 23256, 100, 373, 24994360);
		acc = 0;
		const int64_t ms = RunTicks(t, 100000, &acc, 18000, 0);
		ok = Check(krog == (100000LL * 8388 * 18000) / kRateDen && ms == (100000LL * 373 * 18000) / kRateDen,
		            "rate 100 over 18000 ticks: exact raw totals (HealTime ignored)") && ok;
		ok = Check(krog == 7679 && ms == 341, "rate 100: CORKROG 7679 raw, CORMS 341 raw") && ok;

		// HealTime 3 (gate every 4 ticks, t 3), stock CORKROG, x multiplier: exact accumulator.
		SetupTest(t, 3, 24480, 100, 8388, 1560065);
		acc = 0;
		const int64_t ht3 = RunTicks(t, 0, &acc, 25600, 0);   // 6400 firings
		ok = Check(ht3 == (24480LL * 3 * REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER * 6400) / 1560065,
		            "HealTime 3: 6400 firings, exact total") && ok;
		ok = Check(ComputeRegen(&t.unit, &t.def, 1, 0, &acc) == 0 && ComputeRegen(&t.unit, &t.def, 2, 0, &acc) == 0 &&
		            ComputeRegen(&t.unit, &t.def, 3, 0, &acc) == 0, "HealTime 3: ticks 1-3 do not fire") && ok;

		// HealTime 16: gate every 2nd tick (bit 4 only), t = 17.
		SetupTest(t, 16, 1000, 100, 65536, 1000);
		acc = 0;
		ok = Check(RunTicks(t, 0, &acc, 32, 0) == 16LL * (1000 * 17 * REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER) / 1000,
		            "HealTime 16: 16 firings in 32 ticks, t 17") && ok;

		// Table unavailable: round up, never down.
		SetupTest(t, 0, 30000, 100, 65536, 1000);
		ok = Check(RunTicks(t, 1000, nullptr, 30, 0) == 30, "no table, rate 1 -> rounds up to 1 HP/tick") && ok;
		SetupTest(t, 3, 24480, 100, 8388, 1560065);
		ok = Check(ComputeRegen(&t.unit, &t.def, 0, 0, nullptr) == 1, "no table, HealTime 3 -> rounds up to 1") && ok;

		// A stale accumulator from the other formula is discarded, not paid out.
		SetupTest(t, 0, 30000, 100, 65536, 1000);
		acc = 2100000000;
		ok = Check(RunTicks(t, 1000, &acc, 1, 0) == 0 && acc == 65536000, "stale acc on rate key -> discarded") && ok;
		SetupTest(t, 3, 24480, 100, 8388, 1560065);
		acc = 2000000000;
		ok = Check(ComputeRegen(&t.unit, &t.def, 0, 0, &acc) == 0, "stale acc on HealTime -> discarded") && ok;

		// Rate clamp keeps the product in range.
		ok = Check(RateStep(kMaxRateMilli, 0xFFFFFFFFu, nullptr) > 0, "max rate x max Q does not overflow") && ok;

		return ok;
	}

	bool        g_installed = false;
	SingleHook* g_hook = nullptr;
}

namespace SelfHealRate
{
	void Install()
	{
		if (g_installed)
			return;

		if (!Expect(kBlock, kBlockBytes, sizeof(kBlockBytes), "passive regen block") ||
			!Expect(kResume, kResumeBytes, sizeof(kResumeBytes), "regen block exit"))
			return;

		if (!RunSelfTest())
		{
			IDDrawSurface::OutptTxt("[SelfHealRate] DISABLED: self-test failed (see [selftest] lines); engine regen unchanged");
			return;
		}

		g_ude = UnitDefExtensions::GetInstance();
		g_keyIdx = g_ude->registerStringKey(SELFHEAL_RATE_FBI_KEY, "");

		g_hook = new SingleHook(kBlock, kSpliceLen, INLINE_SINGLEJMP, reinterpret_cast<LPBYTE>(&RegenBlockEntry));
		FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(kBlock), kSpliceLen);
		if (!SpliceReadsBack())
		{
			delete g_hook;
			g_hook = nullptr;
			FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(kBlock), kSpliceLen);
			IDDrawSurface::OutptTxt("[SelfHealRate] DISABLED: splice did not read back; engine bytes restored");
			return;
		}

		g_installed = true;
		IDDrawSurface::OutptFmtTxt(
			"[SelfHealRate] installed: regen block @0x%08X -> resume 0x%08X; FBI key %s = HP/s (3 decimals) x DamageModifier, "
			"HealTime formula x%d for units without it; no energy cost",
			kBlock, kResume, SELFHEAL_RATE_FBI_KEY, REPAIR_RATE_FIX_SELFHEAL_MULTIPLIER);
	}

	void Shutdown()
	{
		if (g_hook)
		{
			IDDrawSurface::OutptFmtTxt("[SelfHealRate] counters: %lu heals applied, %lu table fallbacks",
			                           g_heals, g_fallbackHits);
			delete g_hook;
			g_hook = nullptr;
		}
		std::free(g_acc);
		g_acc = nullptr;
		g_accUnits = 0;
		g_accBase = nullptr;
		std::free(g_rates);
		g_rates = nullptr;
		g_ratesSize = 0;
		g_ratesBuilt = false;
		g_installed = false;
	}
}

#else  // !SELFHEAL_RATE_ENABLE

namespace SelfHealRate
{
	void Install() {}
	void Shutdown() {}
}

#endif
