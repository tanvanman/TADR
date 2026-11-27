#pragma once

#include <windows.h>

#include <memory>

class SingleHook;

// One engine function replaced whole by ours: a 5-byte JMP at its entry, placed only when every
// byte of the function is the stock one our replacement mirrors. A function that differs (another
// exe, or another patch already in it) is left alone, so the replacement never runs in place of
// code it was not written against.
class FunctionPatch
{
public:
	// `tag` starts every log line ("[ParticleWalk]"); `stock` is the whole function, `length`
	// bytes from `address`, as the stock TotalA.exe has it.
	FunctionPatch(const char* tag, DWORD address, const BYTE* stock, DWORD length);
	~FunctionPatch();

	// Compares the function with the stock bytes and, only on a match, places the JMP to `target`
	// and reads it back. Logs one line either way: "<tag> <installed>" on success, otherwise the
	// first differing byte or the failed readback. With `enginePrologue` (check builds) it first
	// builds Engine(). Does nothing once installed.
	bool Install(const void* target, const char* installed, DWORD enginePrologue = 0);

	// Restores the engine's bytes and frees Engine(). Safe when nothing was installed.
	void Remove();

	bool Installed() const { return m_hook != nullptr; }

	// The engine's own function, for a check build to run beside ours: a copy of its first
	// `enginePrologue` bytes, then a jump back past them. Those bytes must be whole instructions,
	// at least the JMP's 5, and position-independent. Null unless Install() was asked for it.
	const void* Engine() const { return m_engine; }

private:
	bool MakeEngineEntry(DWORD prologue);

	const char* const m_tag;
	const DWORD m_address;
	const BYTE* const m_stock;
	const DWORD m_length;
	std::unique_ptr<SingleHook> m_hook;
	void* m_engine;
};
