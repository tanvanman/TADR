#include "Md5.h"

#include <cstring>

namespace
{
	inline std::uint32_t RotL(std::uint32_t x, int c)
	{
		return (x << c) | (x >> (32 - c));
	}

	const std::uint32_t K[64] = {
		0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
		0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
		0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
		0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
		0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
		0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
		0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
		0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
		0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
		0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
		0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
		0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
		0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
		0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
		0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
		0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
	};

	const int S[64] = {
		7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
		5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
		4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
		6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
	};
}

Md5::Md5() :
	m_bitCount(0u),
	m_bufferLen(0u),
	m_finalised(false)
{
	m_state[0] = 0x67452301;
	m_state[1] = 0xefcdab89;
	m_state[2] = 0x98badcfe;
	m_state[3] = 0x10325476;
	std::memset(m_buffer, 0, sizeof(m_buffer));
}

void Md5::Transform(const std::uint8_t block[64])
{
	std::uint32_t M[16];
	for (int i = 0; i < 16; ++i)
	{
		// MD5 is little-endian; read byte-wise so this is host-endian agnostic.
		M[i] = std::uint32_t(block[i * 4])
			| (std::uint32_t(block[i * 4 + 1]) << 8)
			| (std::uint32_t(block[i * 4 + 2]) << 16)
			| (std::uint32_t(block[i * 4 + 3]) << 24);
	}

	std::uint32_t a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3];

	for (int i = 0; i < 64; ++i)
	{
		std::uint32_t f;
		int g;
		if (i < 16)      { f = (b & c) | (~b & d);          g = i; }
		else if (i < 32) { f = (d & b) | (~d & c);          g = (5 * i + 1) % 16; }
		else if (i < 48) { f = b ^ c ^ d;                   g = (3 * i + 5) % 16; }
		else             { f = c ^ (b | ~d);                g = (7 * i) % 16; }

		std::uint32_t tmp = d;
		d = c;
		c = b;
		b = b + RotL(a + f + K[i] + M[g], S[i]);
		a = tmp;
	}

	m_state[0] += a;
	m_state[1] += b;
	m_state[2] += c;
	m_state[3] += d;
}

void Md5::Update(const void* data, std::size_t len)
{
	if (m_finalised)
	{
		return;
	}

	const std::uint8_t* p = (const std::uint8_t*)data;
	m_bitCount += std::uint64_t(len) * 8u;

	while (len > 0u)
	{
		std::size_t take = 64u - m_bufferLen;
		if (take > len)
		{
			take = len;
		}
		std::memcpy(m_buffer + m_bufferLen, p, take);
		m_bufferLen += take;
		p += take;
		len -= take;

		if (m_bufferLen == 64u)
		{
			Transform(m_buffer);
			m_bufferLen = 0u;
		}
	}
}

std::string Md5::HexDigest()
{
	if (!m_finalised)
	{
		const std::uint64_t bitCount = m_bitCount;

		std::uint8_t pad = 0x80;
		Update(&pad, 1);			// Update() advances m_bitCount, hence the copy above
		pad = 0x00;
		while (m_bufferLen != 56u)
		{
			Update(&pad, 1);
		}

		std::uint8_t lengthLe[8];
		for (int i = 0; i < 8; ++i)
		{
			lengthLe[i] = std::uint8_t((bitCount >> (8 * i)) & 0xff);
		}
		Update(lengthLe, 8);

		m_finalised = true;
	}

	static const char* hex = "0123456789abcdef";
	std::string out;
	out.reserve(32);
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			std::uint8_t byte = std::uint8_t((m_state[i] >> (8 * j)) & 0xff);
			out.push_back(hex[byte >> 4]);
			out.push_back(hex[byte & 0x0f]);
		}
	}
	return out;
}
