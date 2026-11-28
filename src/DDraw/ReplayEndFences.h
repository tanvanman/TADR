#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

// Output-stream tokens are recorded colour IDs, NOT indices into TA's live
// player array. Resolve native senders by DirectPlay ID, then bind each token
// to that sender so another stream cannot satisfy its completion fence.
struct ReplayEndFences
{
	unsigned required = 0;
	unsigned received = 0;
	unsigned announced = 0;
	unsigned finalTick = 0;
	unsigned lastFenceGameTime = 0;
	bool lastFenceInsideSimulation = false;
	int viewerIsRecordedPlayer = -1; // Unknown in version 1; explicit in version 2.
	std::array<unsigned, 10> sources{};

	bool accept(unsigned tick, unsigned mask, unsigned bit, unsigned fromDpid,
		const std::array<unsigned, 10>& knownOutputs, unsigned gameTime, int recordedViewer = -1,
		bool insideSimulation = false)
	{
		if (announced && announced != mask) return false;
		if (recordedViewer < -1 || recordedViewer > 1) return false;
		if (viewerIsRecordedPlayer >= 0 && recordedViewer >= 0 && viewerIsRecordedPlayer != recordedViewer) return false;
		if (!mask || (mask & ~0x3ffu) || !bit || (bit & (bit - 1)) || !(mask & bit)) return false;
		if (!fromDpid || std::find(knownOutputs.begin(), knownOutputs.end(), fromDpid) == knownOutputs.end()) return false;
		if (required && (required != mask || finalTick != tick)) return false;
		unsigned token = 0;
		while ((1u << token) != bit) ++token;
		if (sources[token] && sources[token] != fromDpid) return false;
		for (unsigned i = 0; i < sources.size(); ++i)
			if (i != token && sources[i] == fromDpid) return false;
		required = mask;
		finalTick = tick;
		if (recordedViewer >= 0) viewerIsRecordedPlayer = recordedViewer;
		sources[token] = fromDpid;
		if (!(received & bit)) {
			lastFenceGameTime = gameTime;
			lastFenceInsideSimulation = insideSimulation;
		}
		received |= bit;
		return true;
	}

	bool announce(unsigned mask, unsigned bit, unsigned fromDpid,
		const std::array<unsigned, 10>& knownOutputs, int recordedViewer)
	{
		// Reuse the same identity/role checks without counting a BEGIN as EOF.
		ReplayEndFences candidate = *this;
		candidate.required = 0;
		if (!candidate.accept(0, mask, bit, fromDpid, knownOutputs, 0, recordedViewer)) return false;
		// During loading the raw funnel can still use the host's identity.
		// Bind tokens only at END, after the native sender rewrite/queue.
		viewerIsRecordedPlayer = candidate.viewerIsRecordedPlayer;
		announced = mask;
		return true;
	}

	bool ready(unsigned gameTime, bool completedSimulation = false) const
	{
		// Recorded ticks and the viewer's local simulation clock diverge during
		// accelerated playback. Require a completed LOCAL simulation pass after
		// the last new fence, not catch-up to the recording's absolute clock.
		return required && received == required && (gameTime > lastFenceGameTime ||
			(gameTime == lastFenceGameTime && lastFenceInsideSimulation && completedSimulation));
	}

	bool excludeLocalViewer(bool localHuman, bool knownFunnel) const
	{
		// POV selection can change the viewing index. Use the controller's role,
		// never its name/index. A ten-player funnel is a recorded participant,
		// even before its first packet switches the local controller to remote.
		return localHuman && (viewerIsRecordedPlayer >= 0 ? !viewerIsRecordedPlayer : !knownFunnel);
	}
};

// A recorded resignation/disconnect can clear a live native slot before its
// ordered completion fence is dispatched. Retain its validated transport ID.
struct ReplayEndSources
{
	std::array<unsigned, 10> ids{};
	void observe(unsigned dpid)
	{
		if (!dpid || dpid == ~0u || std::find(ids.begin(),ids.end(),dpid)!=ids.end()) return;
		auto empty = std::find(ids.begin(),ids.end(),0u);
		if (empty!=ids.end()) *empty = dpid;
	}
};
