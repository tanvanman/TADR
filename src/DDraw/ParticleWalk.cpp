#include "config.h"

#if FAST_SIMULATION_ENABLE

#include "ParticleWalk.h"

#include "FunctionPatch.h"
#include "iddrawsurface.h"
#include "tamem.h"
#include "tafunctions.h"

#if TDRAW_PARTICLE_WALK_VERIFY
#include <vector>
#endif

namespace
{
	const DWORD kFunctionAddr = 0x00471EB0u;

	// The whole walker, 0x471EB0 through `ret` at 0x471F3E, as the stock TotalA.exe has it.
	const BYTE kFunctionBytes[] = {
		0x83, 0xEC, 0x08, 0xA1, 0xE8, 0x1D, 0x51, 0x00, 0x53, 0x55, 0x56, 0x8B, 0x98, 0x77, 0x8D, 0x03,
		0x00, 0x57, 0x83, 0xC3, 0x08, 0xC7, 0x44, 0x24, 0x10, 0x0A, 0x00, 0x00, 0x00, 0x8B, 0x6B, 0xFC,
		0x8B, 0x03, 0x3B, 0xE8, 0x74, 0x53, 0x8D, 0x7D, 0x04, 0x8B, 0x75, 0x00, 0x8B, 0xCE, 0x8B, 0x16,
		0xFF, 0x52, 0x0C, 0x85, 0xC0, 0x74, 0x31, 0x85, 0xF6, 0x74, 0x08, 0x8B, 0x06, 0x6A, 0x01, 0x8B,
		0xCE, 0xFF, 0x10, 0x8B, 0x0B, 0x8B, 0xC7, 0x3B, 0xF9, 0x74, 0x10, 0x8B, 0xD5, 0x2B, 0xD7, 0x8B,
		0x30, 0x89, 0x34, 0x10, 0x83, 0xC0, 0x04, 0x3B, 0xC1, 0x75, 0xF4, 0x8B, 0x03, 0x83, 0xC0, 0xFC,
		0x89, 0x44, 0x24, 0x14, 0x89, 0x03, 0xEB, 0x0D, 0x8B, 0x16, 0x8B, 0xCE, 0xFF, 0x52, 0x04, 0x83,
		0xC5, 0x04, 0x83, 0xC7, 0x04, 0x3B, 0x2B, 0x75, 0xB0, 0x8B, 0x44, 0x24, 0x10, 0x83, 0xC3, 0x10,
		0x48, 0x89, 0x44, 0x24, 0x10, 0x75, 0x96, 0x5F, 0x5E, 0x5D, 0x5B, 0x83, 0xC4, 0x08, 0xC3,
	};

	const unsigned kLayerTable = 0x38D77u;      // main+: the particle layers, read once a walk
	const unsigned kLayers     = 10u;

	// One particle layer, 16 bytes: a vector of object pointers.
	struct Layer
	{
		DWORD  unused;
		void** begin;
		void** end;
		void** capacity;
	};

	typedef int  (__thiscall *IsDoneFn)(void* object);               // vtable slot 3
	typedef void (__thiscall *DestroyFn)(void* object, int flags);   // vtable slot 0
	typedef void (__thiscall *UpdateFn)(void* object);               // vtable slot 1

	FunctionPatch g_patch("[ParticleWalk]", kFunctionAddr, kFunctionBytes, sizeof(kFunctionBytes));

	__forceinline Layer* Layers()
	{
		return *reinterpret_cast<Layer* const*>(reinterpret_cast<const char*>(*TAmainStruct_PtrPtr) + kLayerTable);
	}

	__forceinline void* const* Vtable(void* object)
	{
		return *static_cast<void* const* const*>(object);
	}

	__forceinline bool IsDone(void* object)
	{
		return reinterpret_cast<IsDoneFn>(Vtable(object)[3])(object) != 0;
	}

	__forceinline void Destroy(void* object)
	{
		reinterpret_cast<DestroyFn>(Vtable(object)[0])(object, 1);
	}

	__forceinline void Update(void* object)
	{
		reinterpret_cast<UpdateFn>(Vtable(object)[1])(object);
	}

	// What the release walk checks: nothing. The check build passes a Checker (below) through the
	// same walk, so the walk it checks is the one that ships.
	struct NoCheck
	{
		void Begin(const Layer*, Layer*) {}
		void Answer(bool) {}
		void AfterCall() {}
		void End() {}
	};

	// The engine visits each object at the place its earlier removals have shifted it down to, which
	// is where `keep` stands, so a survivor is written there before its update. The end is read once:
	// by the invariant (ParticleWalk.h) no slot call moves it.
	template <class Check>
	__forceinline void WalkLayer(Layer* layer, Check& check)
	{
		void** keep = layer->begin;
		void** const end = layer->end;
		for (void** read = layer->begin; read != end; ++read)
		{
			void* const object = *read;
			const bool done = IsDone(object);
			check.Answer(done);
			check.AfterCall();
			if (done)
			{
				Destroy(object);
				check.AfterCall();
				continue;
			}
			*keep++ = object;
			Update(object);
			check.AfterCall();
		}
		layer->end = keep;
	}

	template <class Check>
	__forceinline void Walk(Check& check)
	{
		Layer* const layers = Layers();
		for (unsigned i = 0; i < kLayers; ++i)
		{
			check.Begin(layers, &layers[i]);
			WalkLayer(&layers[i], check);
			check.End();
		}
	}

#if TDRAW_PARTICLE_WALK_VERIFY
	typedef void (__cdecl *WalkFn)();

	const DWORD    kPrologueLen   = 8u;     // sub esp,8; mov eax,[0x511DE8]: position-independent
	const unsigned kBlock         = 64u;    // ticks a block
	const unsigned kProgressEvery = 64u * kBlock;

	enum Phase { kEngine, kMine, kChecked, kMineAgain };

	unsigned g_ticks, g_checkedTicks, g_differ, g_broken, g_peak;
	unsigned __int64 g_checkedObjects, g_removed;
	double g_usPerCount;

	// The timed blocks: running totals for each walker, and the block being timed.
	struct Totals { double us; unsigned ticks; unsigned __int64 objects; };
	Totals g_engineTotals, g_mineTotals, g_block;
	// An engine block is compared with the mean of the timed blocks either side of it: the last
	// one before it, and the next one, which closes the comparison.
	double g_lastMineRate = -1, g_pendingEngineRate = -1, g_pendingBefore = -1;
	unsigned g_slower, g_compared;

	unsigned Count()
	{
		const Layer* const layers = Layers();
		unsigned n = 0;
		for (unsigned i = 0; i < kLayers; ++i)
			n += static_cast<unsigned>(layers[i].end - layers[i].begin);
		return n;
	}

	double Now()
	{
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);
		return static_cast<double>(t.QuadPart) * g_usPerCount;
	}

	// After every slot call: the layer table, and this layer's begin, end and capacity, are as they
	// were when the walk of the layer began. That catches a slot that moves the layer; a slot that
	// only reads the layer (and could meet the stale places between `keep` and `read`) it cannot
	// catch -- the call-graph walk in ParticleWalk.h is what covers reads. At the end of the layer:
	// it holds what the engine's walker leaves given the same answers, the objects slot 3 did not
	// call finished, in their order.
	struct Checker
	{
		const Layer* table;
		Layer* layer;
		Layer shape;
		std::vector<void*> before;
		std::vector<bool> done;

		void Begin(const Layer* t, Layer* l)
		{
			table = t;
			layer = l;
			shape = *l;
			before.assign(l->begin, l->end);
			done.clear();
		}

		void Answer(bool finished)
		{
			done.push_back(finished);
			g_removed += finished;
		}

		void AfterCall()
		{
			if (Layers() == table && layer->begin == shape.begin && layer->end == shape.end && layer->capacity == shape.capacity)
				return;
			if (g_broken++ < 8)
				IDDrawSurface::OutptFmtTxt("[ParticleWalk] VERIFY: layer %u changed during a slot call", static_cast<unsigned>(layer - table));
		}

		void End()
		{
			const size_t n = static_cast<size_t>(layer->end - layer->begin);
			size_t kept = 0;
			bool same = done.size() == before.size();
			for (size_t j = 0; same && j < before.size(); ++j)
				if (!done[j])
					same = kept < n && layer->begin[kept++] == before[j];
			g_checkedObjects += done.size();
			if (!(same && kept == n) && g_differ++ < 8)
				IDDrawSurface::OutptFmtTxt("[ParticleWalk] VERIFY: layer %u differs from the engine's walk", static_cast<unsigned>(layer - table));
		}
	};

	void LogTimes(const char* when)
	{
		const Totals& e = g_engineTotals;
		const Totals& m = g_mineTotals;
		IDDrawSurface::OutptFmtTxt("[ParticleWalk] VERIFY %s: %u ticks checked, %llu objects checked, %llu removed, %u differed, %u invariant breaks; "
			"engine %.1f us a tick over %u ticks (%.0f objects a tick), this %.1f us a tick over %u ticks (%.0f objects a tick); "
			"engine slower than its neighbours in %u of %u blocks; peak %u objects",
			when, g_checkedTicks, g_checkedObjects, g_removed, g_differ, g_broken,
			e.ticks ? e.us / e.ticks : 0.0, e.ticks, e.ticks ? static_cast<double>(e.objects) / e.ticks : 0.0,
			m.ticks ? m.us / m.ticks : 0.0, m.ticks, m.ticks ? static_cast<double>(m.objects) / m.ticks : 0.0,
			g_slower, g_compared, g_peak);
	}

	void CloseBlock(Phase phase)
	{
		const double rate = g_block.us / g_block.ticks;
		Totals& totals = phase == kEngine ? g_engineTotals : g_mineTotals;
		totals.us += g_block.us;
		totals.ticks += g_block.ticks;
		totals.objects += g_block.objects;
		g_block = Totals();
		if (phase == kEngine)
		{
			g_pendingEngineRate = rate;
			g_pendingBefore = g_lastMineRate;
			return;
		}
		if (g_pendingEngineRate >= 0 && g_pendingBefore >= 0)
		{
			++g_compared;
			g_slower += g_pendingEngineRate > (g_pendingBefore + rate) / 2;
		}
		g_pendingEngineRate = -1;
		g_lastMineRate = rate;
	}

	// One tick: the engine's walker, this one, this one checked, this one, 64 ticks each in turn.
	void __cdecl WalkAndMeasure()
	{
		const Phase phase = static_cast<Phase>((g_ticks / kBlock) % 4);
		++g_ticks;
		const unsigned objects = Count();
		if (objects > g_peak)
			g_peak = objects;

		if (phase == kChecked)
		{
			Checker checker;
			Walk(checker);
			++g_checkedTicks;
		}
		else
		{
			const double start = Now();
			if (phase == kEngine)
				reinterpret_cast<WalkFn>(g_patch.Engine())();
			else
			{
				NoCheck none;
				Walk(none);
			}
			g_block.us += Now() - start;
			g_block.objects += objects;
			++g_block.ticks;
		}

		if (g_ticks % kBlock == 0)
		{
			if (phase != kChecked)
				CloseBlock(phase);
			if (g_ticks % kProgressEvery == 0)
				LogTimes("so far");
		}
	}
#endif
}

namespace ParticleWalk
{
	// The release walk. External linkage so that the link map lists it.
	void __cdecl WalkLayers()
	{
		NoCheck none;
		Walk(none);
	}

	void Install()
	{
#if TDRAW_PARTICLE_WALK_VERIFY
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		g_usPerCount = 1e6 / static_cast<double>(frequency.QuadPart);
		g_patch.Install(&WalkAndMeasure, "installed with VERIFY: the engine's walker and this one alternate, every fourth block checked", kPrologueLen);
#else
		g_patch.Install(&WalkLayers, "installed: particle layers' tick at 0x471EB0");
#endif
	}

	void Shutdown()
	{
		if (!g_patch.Installed())
			return;
		g_patch.Remove();
#if TDRAW_PARTICLE_WALK_VERIFY
		LogTimes("total");
#endif
	}
}

#endif // FAST_SIMULATION_ENABLE
