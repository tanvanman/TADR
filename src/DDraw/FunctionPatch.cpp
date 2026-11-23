#include "FunctionPatch.h"

#include <cstring>

#include "hook/hook.h"
#include "iddrawsurface.h"

namespace
{
	const DWORD kJumpLen = 5u;
}

FunctionPatch::FunctionPatch(const char* tag, DWORD address, const BYTE* stock, DWORD length)
	: m_tag(tag), m_address(address), m_stock(stock), m_length(length), m_engine(nullptr)
{
}

FunctionPatch::~FunctionPatch()
{
	Remove();
}

bool FunctionPatch::Install(const void* target, const char* installed, DWORD enginePrologue)
{
	if (m_hook)
		return true;

	const BYTE* const at = reinterpret_cast<const BYTE*>(m_address);
	if (std::memcmp(at, m_stock, m_length) != 0)
	{
		DWORD i = 0;
		while (at[i] == m_stock[i])
			++i;
		IDDrawSurface::OutptFmtTxt("%s 0x%08X is not the stock function (first difference at +0x%X); not installed",
			m_tag, m_address, i);
		return false;
	}

	if (enginePrologue && !MakeEngineEntry(enginePrologue))
	{
		IDDrawSurface::OutptFmtTxt("%s no executable memory for the engine's entry; not installed", m_tag);
		return false;
	}

	m_hook = std::make_unique<SingleHook>(m_address, kJumpLen, INLINE_SINGLEJMP,
		reinterpret_cast<LPBYTE>(const_cast<void*>(target)));
	FlushInstructionCache(GetCurrentProcess(), at, kJumpLen);

	// SingleHook reports nothing: check the bytes it meant to write are the ones there.
	if (std::memcmp(m_hook->m_AddrToHook, m_hook->m_NewBytes_Pbyte, kJumpLen) != 0)
	{
		Remove();
		IDDrawSurface::OutptFmtTxt("%s the jump did not read back; not installed", m_tag);
		return false;
	}
	IDDrawSurface::OutptFmtTxt("%s %s", m_tag, installed);
	return true;
}

void FunctionPatch::Remove()
{
	if (m_hook)
	{
		m_hook.reset();   // SingleHook::~SingleHook() restores the engine's 5 bytes
		FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(m_address), kJumpLen);
	}
	if (m_engine)
	{
		VirtualFree(m_engine, 0, MEM_RELEASE);
		m_engine = nullptr;
	}
}

// Copied from the stock bytes, which Install() has just found at the function's entry.
bool FunctionPatch::MakeEngineEntry(DWORD prologue)
{
	const DWORD size = prologue + kJumpLen;
	BYTE* const entry = static_cast<BYTE*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	if (!entry)
		return false;
	std::memcpy(entry, m_stock, prologue);
	BYTE* const jump = entry + prologue;
	jump[0] = 0xE9;
	*reinterpret_cast<DWORD*>(jump + 1) = (m_address + prologue) - reinterpret_cast<DWORD>(jump + kJumpLen);
	DWORD old;
	if (!VirtualProtect(entry, size, PAGE_EXECUTE_READ, &old))
	{
		VirtualFree(entry, 0, MEM_RELEASE);
		return false;
	}
	FlushInstructionCache(GetCurrentProcess(), entry, size);
	m_engine = entry;
	return true;
}
