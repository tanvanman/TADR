#include "StartPositions.h"
#include "tamem.h"
#include "iddrawsurface.h"
#include "hook/hook.h"

// Hook at the point where multiplayer fixed positions are assigned.
// TotalA.exe behaviour is to assign positions sequentialy.
// But if the positions have been set in our shared memory, we'll make it do our own thing ...
static unsigned int StartPositionsHookAddr = 0x4569da;
static unsigned int StartPositionsHookProc(PInlineX86StackBuffer X86StrackBuffer)
{
	if (!StartPositions::GetInstance())
	{
		return 0;
	}
	StartPositionsShare* sm = StartPositions::GetInstance()->GetSharedMemory();
	if (!sm || !sm->positionCount)
	{
		return 0;
	}
	X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x4569e5;

	PlayerInfoStruct* playerInfo = (PlayerInfoStruct*)X86StrackBuffer->Ecx;
	PlayerStruct* player = (PlayerStruct*)(X86StrackBuffer->Edx + X86StrackBuffer->Eax + 0x1b63);

	for (int n = 0; n < 10; ++n)
	{
		if (sm->orderedDirectplayIds[n] == player->DirectPlayID && !sm->usedPositions[n])
		{
			*(char*)X86StrackBuffer->Esi = n;
			sm->usedPositions[n] = 1;
			IDDrawSurface::OutptTxt("assigning %s to startpos %d because assigned by dpid", player->Name, n);
			return X86STRACKBUFFERCHANGE;
		}
		else if (n >= sm->positionCount && !sm->usedPositions[n])
		{
			// reached the last position without finding the ID ...
			*(char*)X86StrackBuffer->Esi = n;
			sm->usedPositions[n] = 1;
			IDDrawSurface::OutptTxt("assigning %s to startpos %d because next available", player->Name, n);
			return X86STRACKBUFFERCHANGE;
		}
	}

	// dpid not found and we reached the end of the unassigned positions.
	// Now check for unclaimed positions that were assigend a dpid.
	for (int n = 0; n < 10; ++n)
	{
		if (!sm->usedPositions[n])
		{
			*(char*)X86StrackBuffer->Esi = n;
			sm->usedPositions[n] = 1;
			IDDrawSurface::OutptTxt("assigning %s to startpos %d because next unused", player->Name, n);
			return X86STRACKBUFFERCHANGE;
		}
	}

	// we ran out of start positions ... what to do?
	*(char*)X86StrackBuffer->Esi = player->PlayerAryIndex;
	IDDrawSurface::OutptTxt("assigning %s to startpos %d because none available", player->Name, int(player->PlayerAryIndex));
	return X86STRACKBUFFERCHANGE;
}

std::unique_ptr<StartPositions> StartPositions::m_instance;

StartPositions* StartPositions::GetInstance()
{
	if (!m_instance)
	{
		m_instance.reset(new StartPositions());
	}
	return m_instance.get();
}

StartPositions::StartPositions():
	m_startPositionsShare(NULL),
	m_hMemMap(NULL)
{
	CreateSharedMemory();
	if (m_hMemMap && m_startPositionsShare)
	{
		m_hook.reset(new InlineSingleHook(StartPositionsHookAddr, 5, INLINE_5BYTESLAGGERJMP, StartPositionsHookProc));
	}
}

StartPositions::~StartPositions()
{
	if (m_hMemMap != NULL)
	{
		UnmapViewOfFile(m_hMemMap);
		CloseHandle(m_hMemMap);
	}
	m_hMemMap = m_startPositionsShare = NULL;
}

void StartPositions::CreateSharedMemory()
{
	m_hMemMap = CreateFileMapping((HANDLE)0xFFFFFFFF,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(StartPositionsShare),
		"TADemo-StartPositions");

	bool bExists = (GetLastError() == ERROR_ALREADY_EXISTS);

	void* mem = MapViewOfFile(m_hMemMap,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(StartPositionsShare));

	if (!bExists)
	{
		memset(mem, 0, sizeof(StartPositionsShare));
	}

	m_startPositionsShare = static_cast<StartPositionsShare*>(mem);
}

StartPositionsShare* StartPositions::GetSharedMemory()
{
	return m_startPositionsShare;
}
