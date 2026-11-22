#pragma once

// TAG_Venom: display-only, acknowledged queue transactions. No engine state.
// Kept independent of TA/Windows so the actual wire/cache logic can be tested.
#include <algorithm>
#include <cstdint>
#include <vector>

struct AlliedBuildQueueRecord
{
	std::uint16_t buildUnitId, x, y, z;
	std::uint8_t rotation;
};

namespace AlliedQueueProtocol
{
	const size_t MaxRecords = 1200;
	const size_t RecordsPerPacket = 5;
	const size_t MaxChunks = MaxRecords / RecordsPerPacket;
	enum Kind { Full = 1, Delta = 2, Interest = 3 };
	const unsigned char Remove = 0x80;

#pragma pack(push, 1)
	struct WireRecord
	{
		std::uint16_t buildUnitId, x, y, z;
		std::uint8_t rotation;
	};
	struct Packet
	{
		std::uint8_t chatByte, nullText, msgId, size, version, kind;
		// Full: base=0. Delta: base=last acknowledged sequence.
		// Interest: sequence=receive ACK; base bit 0=request, bit 1=serving.
		std::uint32_t sequence, base;
		std::uint16_t chunkIndex, chunkCount;
		std::uint8_t recordCount;
		WireRecord records[RecordsPerPacket];
		std::uint8_t reserved;
	};
#pragma pack(pop)
	static_assert(sizeof(Packet) == 65, "TA chat envelope");

	inline bool Less(const AlliedBuildQueueRecord& a, const AlliedBuildQueueRecord& b)
	{
		if (a.x != b.x) return a.x < b.x;
		if (a.y != b.y) return a.y < b.y;
		if (a.z != b.z) return a.z < b.z;
		if (a.buildUnitId != b.buildUnitId) return a.buildUnitId < b.buildUnitId;
		return a.rotation < b.rotation;
	}
	inline bool Equal(const AlliedBuildQueueRecord& a, const AlliedBuildQueueRecord& b)
	{
		return !Less(a, b) && !Less(b, a);
	}
	inline void Canonicalize(std::vector<AlliedBuildQueueRecord>& records)
	{
		std::sort(records.begin(), records.end(), Less);
		records.erase(std::unique(records.begin(), records.end(), Equal), records.end());
	}
	inline bool Same(const std::vector<AlliedBuildQueueRecord>& a,
		const std::vector<AlliedBuildQueueRecord>& b)
	{
		return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), Equal);
	}
	inline bool Newer(std::uint32_t a, std::uint32_t b)
	{
		return a != b && static_cast<std::uint32_t>(a - b) < 0x80000000u;
	}
	inline Packet MakePacket(unsigned char id, Kind kind)
	{
		Packet p = {};
		p.chatByte = 5; p.msgId = id; p.size = sizeof(p); p.version = 2;
		p.kind = static_cast<std::uint8_t>(kind);
		return p;
	}
	inline bool Envelope(const Packet& p, unsigned char id)
	{
		return p.chatByte == 5 && p.nullText == 0 && p.msgId == id
			&& p.size == sizeof(p) && p.version == 2 && p.reserved == 0;
	}
	inline WireRecord Wire(const AlliedBuildQueueRecord& r, bool remove = false)
	{
		WireRecord w = { r.buildUnitId, r.x, r.y, r.z,
			static_cast<std::uint8_t>(r.rotation | (remove ? Remove : 0)) };
		return w;
	}
	inline AlliedBuildQueueRecord Record(const WireRecord& w)
	{
		AlliedBuildQueueRecord r = { w.buildUnitId, w.x, w.y, w.z,
			static_cast<std::uint8_t>(w.rotation & 3) };
		return r;
	}

	struct Transmitter
	{
		std::vector<AlliedBuildQueueRecord> acknowledged, target;
		std::vector<WireRecord> operations;
		std::uint32_t acknowledgedSequence = 0, sequence = 0, base = 0;
		size_t nextChunk = 0;
		bool active = false;
		Kind kind = Full;

		void Acknowledge(std::uint32_t ack)
		{
			if (active && ack == sequence)
			{
				acknowledged.swap(target);
				acknowledgedSequence = sequence;
				active = false;
			}
			else if (ack == 0 && (!active || kind == Delta))
			{
				// Receiver lost its base: only a full transaction can recover it.
				acknowledgedSequence = 0;
				active = false;
			}
		}
		bool Prepare(const std::vector<AlliedBuildQueueRecord>& current, std::uint32_t seq)
		{
			if (active || (acknowledgedSequence && Same(current, acknowledged))) return false;
			target = current;
			operations.clear();
			base = acknowledgedSequence;
			kind = Delta;
			if (base)
			{
				size_t a = 0, b = 0;
				while (a < acknowledged.size() || b < current.size())
				{
					if (b == current.size() || (a < acknowledged.size() && Less(acknowledged[a], current[b])))
						operations.push_back(Wire(acknowledged[a++], true));
					else if (a == acknowledged.size() || Less(current[b], acknowledged[a]))
						operations.push_back(Wire(current[b++]));
					else { ++a; ++b; }
				}
			}
			if (!base || operations.size() >= current.size())
			{
				kind = Full; base = 0; operations.clear();
				for (const auto& r : current) operations.push_back(Wire(r));
			}
			sequence = seq; nextChunk = 0; active = true;
			return true;
		}
		size_t ChunkCount() const
		{
			return operations.empty() ? 1 : (operations.size() + RecordsPerPacket - 1) / RecordsPerPacket;
		}
		Packet Chunk(unsigned char id, size_t index) const
		{
			Packet p = MakePacket(id, kind);
			p.sequence = sequence; p.base = base;
			p.chunkIndex = static_cast<std::uint16_t>(index);
			p.chunkCount = static_cast<std::uint16_t>(ChunkCount());
			const size_t first = index * RecordsPerPacket;
			for (size_t i = first; i < operations.size() && i < first + RecordsPerPacket; ++i)
				p.records[p.recordCount++] = operations[i];
			return p;
		}
	};

	struct Receiver
	{
		std::vector<AlliedBuildQueueRecord> records;
		std::vector<WireRecord> pending;
		std::vector<unsigned char> received;
		std::uint32_t sequence = 0, pendingSequence = 0, pendingBase = 0;
		size_t pendingSize = 0;
		unsigned char pendingKind = 0;
		bool needFull = false;
		std::uint32_t Acknowledgement() const { return needFull ? 0 : sequence; }

		// True means a complete transaction (or a duplicate) should be ACKed.
		bool Accept(const Packet& p)
		{
			if ((p.kind != Full && p.kind != Delta) || !p.sequence
				|| !p.chunkCount || p.chunkCount > MaxChunks || p.chunkIndex >= p.chunkCount
				|| p.recordCount > RecordsPerPacket
				|| (p.chunkIndex + 1 < p.chunkCount && p.recordCount != RecordsPerPacket)
				|| (!p.recordCount && p.chunkCount != 1)
				|| (p.kind == Full && p.base != 0)) return false;
			for (size_t i = 0; i < p.recordCount; ++i)
			{
				if (!p.records[i].buildUnitId || (p.records[i].rotation & ~(Remove | 3))
					|| (p.kind == Full && (p.records[i].rotation & Remove))) return false;
			}
			if (sequence && !Newer(p.sequence, sequence)) return p.sequence == sequence;
			if (p.kind == Delta && (needFull || !sequence || p.base != sequence))
			{ needFull = true; return true; }
			if (pendingSequence && p.sequence != pendingSequence && !Newer(p.sequence, pendingSequence))
				return false;
			if (pendingSequence != p.sequence)
			{
				pendingSequence = p.sequence; pendingBase = p.base; pendingKind = p.kind;
				pending.assign(p.chunkCount * RecordsPerPacket, WireRecord{});
				received.assign(p.chunkCount, 0); pendingSize = 0;
			}
			if (received.size() != p.chunkCount || pendingKind != p.kind || pendingBase != p.base)
				return false;
			if (!received[p.chunkIndex])
			{
				for (size_t i = 0; i < p.recordCount; ++i)
					pending[p.chunkIndex * RecordsPerPacket + i] = p.records[i];
				received[p.chunkIndex] = 1;
				if (p.chunkIndex + 1 == p.chunkCount)
					pendingSize = p.chunkIndex * RecordsPerPacket + p.recordCount;
			}
			if (std::find(received.begin(), received.end(), 0) != received.end()) return false;
			std::vector<AlliedBuildQueueRecord> next = p.kind == Full
				? std::vector<AlliedBuildQueueRecord>() : records;
			for (size_t i = 0; i < pendingSize; ++i)
			{
				const auto r = Record(pending[i]);
				if (pending[i].rotation & Remove)
					next.erase(std::remove_if(next.begin(), next.end(),
						[&r](const AlliedBuildQueueRecord& v) { return Equal(r, v); }), next.end());
				else next.push_back(r);
			}
			Canonicalize(next);
			if (next.size() > MaxRecords) { needFull = true; return true; }
			records.swap(next); sequence = p.sequence; needFull = false;
			pendingSequence = 0; pending.clear(); received.clear();
			return true;
		}
	};

	// Wall-time token bucket; no saved idle credit and no catch-up bursts.
	struct Budget
	{
		std::uint32_t last = 0;
		unsigned credit = 0;
		unsigned Take(std::uint32_t now, unsigned packetsPerSecond, unsigned burst)
		{
			const unsigned elapsed = static_cast<std::uint32_t>(now - last);
			last = now;
			credit = (std::min)(burst * 1000u, credit + (std::min)(elapsed, 1000u) * packetsPerSecond);
			const unsigned packets = credit / 1000;
			credit -= packets * 1000;
			return packets;
		}
	};
}
