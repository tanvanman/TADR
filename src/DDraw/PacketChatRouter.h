#pragma once

#include <functional>
#include <map>
#include <memory>
#include <vector>
#include "hook/hook.h"

// Owns the hook at Packet_Chat_0x05 (0x45522E, 5 bytes)
// and dispatches incoming chat packets in two modes:
//
//   1. Custom empty-chat packets (chatByte=0x05, nullText=0x00) — routed by msgId byte.
//      Usage: PacketChatRouter::GetInstance()->RegisterHandler(0x2b,
//                 [](unsigned fromDpid, const void* buf) { ... });
//
//   2. Regular chat messages (chatByte=0x05, non-null text) — passed to chat handlers.
//      Usage: PacketChatRouter::GetInstance()->RegisterChatHandler(
//                 [](unsigned fromDpid, const char* text) { ... });
//
// All handlers are called only when:
//   - local player is active, non-spectator, not in demo playback
//   - packet starts with chatByte=0x05
class PacketChatRouter
{
public:
	static PacketChatRouter* GetInstance();

	using Handler     = std::function<void(unsigned fromDpid, const void* buf)>;
	using ChatHandler = std::function<void(unsigned fromDpid, const char* text)>;

	void RegisterHandler(unsigned char msgId, Handler handler);
	void RegisterChatHandler(ChatHandler handler);

private:
	PacketChatRouter();
	static int __stdcall PacketChatProc(PInlineX86StackBuffer pBuf);

	static PacketChatRouter* m_instance;
	std::unique_ptr<InlineSingleHook> m_hook;
	std::map<unsigned char, Handler>  m_handlers;
	std::vector<ChatHandler>          m_chatHandlers;
};
