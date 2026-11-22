#pragma once

// Classifies one chat-ring line into six kinds, from the fields the drawer
// already has:
//
//   channel = entry[0x47] & 0x0F      (1 unit, 2 cmd, 4 event, 8 chat)
//   alert   = entry[0x44]  (u16)      (non-zero only for unit alerts)
//   slot    = entry[0x46]             (0..9 player, 10 = system, no logo)
//   first   = text[0]                 ('<' only for formatted player chat)
//
//   Chat   : chan 0/8/4, text starts '<'      -> player column   (has logo)
//   Ping   : chan 1, alert == 0, slot != 10   -> player column   (has logo)
//   Unit   : chan 1, alert != 0               -> system column
//   Event  : chan 4, text not '<'             -> system column   (elim/leave)
//   Notice : chan 8, text not '<'; OR         -> system column
//            chan 1, alert == 0, slot == 10      (pause/ready, LocalNotice)
//   Cmd    : chan 2                           -> system column
//   Other  : anything else                    -> system column, never filtered
//
// Channel numbers differ by observation point: PlayerMute reads
// Net_PushChatHudMessage, where player chat is channel 8 with the real sender
// slot; by the time it reaches the ring this classifier walks, the engine has
// rewritten it to channel 0, slot 10. Ally chat and elimination/leave events
// both use channel 4 and are told apart only by the "<Name>" wrapper -- a
// heuristic, as is "chat vs notice" (text[0]=='<'). "Unit vs ping" is the
// solid alert-payload discriminator. The channel-1 slot==10 arm catches
// LocalNotice() lines, which are otherwise indistinguishable from a ping.

enum ChatKind
{
	CK_Chat = 0,
	CK_Ping,
	CK_Unit,
	CK_Event,
	CK_Notice,
	CK_Cmd,
	CK_Other,
	CK_COUNT
};

inline ChatKind ChatClassify(int channel, int alert, int slot, char first)
{
	switch (channel & 0x0F)
	{
	case 0:  return (first == '<') ? CK_Chat : CK_Other;   // all-chat, as the ring stores it
	case 8:  return (first == '<') ? CK_Chat : CK_Notice;  // push-point form (synthetic tests)
	case 1:
		if (alert != 0)  return CK_Unit;     // unit alert -- system column
		if (slot == 10)  return CK_Notice;   // LocalNotice() -- system column
		return CK_Ping;                      // real ping echo -- player column
	case 4:  return (first == '<') ? CK_Chat : CK_Event;   // ally chat OR elimination/leave
	case 2:  return CK_Cmd;
	default: return CK_Other;
	}
}

inline const char* ChatKindName(ChatKind k)
{
	switch (k)
	{
	case CK_Chat:   return "chat";
	case CK_Ping:   return "ping";
	case CK_Unit:   return "unit";
	case CK_Event:  return "event";
	case CK_Notice: return "notice";
	case CK_Cmd:    return "cmd";
	default:        return "other";
	}
}

// Bit per kind, for the ChatSysGroups routing mask: a line goes to the system
// column when its kind's bit is set, otherwise to the player column.
inline unsigned ChatKindBit(ChatKind k) { return 1u << (int)k; }

// Default system set; chat and pings default to the player column.
const unsigned CHATGROUPS_DEFAULT_SYS =
	(1u << CK_Unit) | (1u << CK_Cmd) | (1u << CK_Event) | (1u << CK_Notice) | (1u << CK_Other);
