#include "PacketChatRouter.h"
#include "iddrawsurface.h"
#include "tamem.h"

PacketChatRouter* PacketChatRouter::m_instance = nullptr;

PacketChatRouter* PacketChatRouter::GetInstance()
{
	if (!m_instance)
		m_instance = new PacketChatRouter();
	return m_instance;
}

PacketChatRouter::PacketChatRouter()
{
	// Hook at Packet_Chat_0x05 entry @ 0x45522E, 5 bytes:
	//   83 3B 00 0F 84  =  cmp [ebx+PlayerStruct.PlayerActive], 0; jz near
	m_hook.reset(new InlineSingleHook(0x45522e, 5, INLINE_5BYTESLAGGERJMP, PacketChatProc));
}

void PacketChatRouter::RegisterHandler(unsigned char msgId, Handler handler, bool fireInDemo)
{
	m_handlers[msgId] = HandlerEntry{ std::move(handler), fireInDemo };
}

void PacketChatRouter::RegisterChatHandler(ChatHandler handler)
{
	m_chatHandlers.push_back(std::move(handler));
}

int __stdcall PacketChatRouter::PacketChatProc(PInlineX86StackBuffer pBuf)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	PlayerStruct* me = &taPtr->Players[taPtr->LocalHumanPlayer_PlayerID];
	const bool playingDemo = DataShare->PlayingDemo != 0;

	// Live-play guard. Spectators and inactive players never dispatch.
	// The PlayingDemo branch is handled below per-handler so passive
	// dispatchers (e.g. WeaponFiredExt) can still run during replays.
	if (!playingDemo
	    && (!me->PlayerActive || me->DirectPlayID == 0
	        || (me->PlayerInfo->PropertyMask & WATCH)))
	{
		return 0;
	}

	const unsigned char* pkt = (const unsigned char*)taPtr->PacketBuffer_p;
	if (!pkt || pkt[0] != 0x05)
		return 0;

	if (pkt[1] == 0x00)
	{
		// Custom empty-chat packet: route by msgId
		unsigned char msgId = pkt[2];
		auto& handlers = GetInstance()->m_handlers;
		auto it = handlers.find(msgId);
		if (it != handlers.end() && (!playingDemo || it->second.fireInDemo))
			it->second.handler(taPtr->hapinet.fromDpid, pkt);
	}
	else if (!playingDemo)
	{
		// Regular chat message: text starts at pkt+1.
		// Chat handlers don't currently distinguish demo vs live; suppress
		// them in demo to preserve historical behaviour.
		const char* text = (const char*)(pkt + 1);
		unsigned fromDpid = taPtr->hapinet.fromDpid;
		for (auto& h : GetInstance()->m_chatHandlers)
			h(fromDpid, text);
	}

	return 0;
}
