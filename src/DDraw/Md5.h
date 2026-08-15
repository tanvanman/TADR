#pragma once

// Minimal, dependency-free MD5 (RFC 1321).
//
// Present solely so tdraw can reproduce the demo compiler's "unitsHash" — the
// key under which faf.game_stats.replay_meta records a game's unit set. That
// value is an MD5 and nothing else will match it, so a hash we already trust
// (HmacSha256) is not a substitute here. Do not use MD5 for anything security
// bearing.

#include <cstdint>
#include <cstddef>
#include <string>

class Md5
{
public:
	Md5();
	void Update(const void* data, std::size_t len);
	// Returns the digest as lowercase hex. Finalises; do not Update afterwards.
	std::string HexDigest();

private:
	void Transform(const std::uint8_t block[64]);

	std::uint32_t m_state[4];
	std::uint64_t m_bitCount;
	std::uint8_t m_buffer[64];
	std::size_t m_bufferLen;
	bool m_finalised;
};
