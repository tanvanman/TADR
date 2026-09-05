#pragma once

// Classifies one chat-ring line into the six kinds ENGINE_NOTES.md §26.3b
// documents, from the fields the drawer already has in hand:
//
//   channel = entry[0x47] & 0x0F      (1 unit, 2 cmd, 4 event, 8 chat)
//   alert   = entry[0x44]  (u16)      (non-zero only for unit alerts)
//   slot    = entry[0x46]             (0..9 player, 10 = system, no logo)
//   first   = text[0]                 ('<' only for formatted player chat)
//
// Nothing here is new engine knowledge -- it is §26.3's verified taxonomy
// arranged as a lookup, for the Stage 4 channel decouple.
//
//   Chat   : chan 0 or 8, text starts '<'     -> player column   (has logo)
//            OR chan 4, text starts '<'          (ally chat)
//   Ping   : chan 1, alert == 0, slot != 10   -> player column   (has logo)
//   Unit   : chan 1, alert != 0               -> system column   (slot 10)
//   Event  : chan 4, text not '<'             -> system column   (elim/leave)
//   Notice : chan 8, text not '<'; OR         -> system column
//            chan 1, alert == 0, slot == 10      (pause/ready, LocalNotice...)
//   Cmd    : chan 2                           -> system column   (slot 10)
//   Other  : anything else                    -> system column, never filtered
//
// CHANNEL NUMBERS DIFFER BY OBSERVATION POINT. PlayerMute reads
// Net_PushChatHudMessage, where player chat is channel 8 with the real sender
// slot. By the time the same line is in the ring buffer this classifier walks,
// the engine has rewritten it to channel 0, slot 10 (verified 2026-08-31 from
// [ChatLayout] RING dumps: "<Player> ...", "<Arm> ..." all chan 0 slot 10).
// Ally chat and elimination/leave events share channel 4; the "<Name...>"
// wrapper is what tells them apart -- INFERRED from a single run, confirm with
// a targeted test (send ally chat AND get a player eliminated).
//
// "Chat vs Notice/Event" rests on the text-shape heuristic (§26.3 consequence
// 1: holds across every replay dump, formatter not disassembled); PlayerMute
// uses the same `text[0]=='<'` test. "Unit vs Ping" is the solid alert-payload
// discriminator (§26.3 consequence 2).
//
// The slot==10 arm of channel 1 was added after the Stage 4a dry run: the
// engine puts LocalNotice() lines ("No player called 'x'.", and the like)
// on channel 1 with alert 0 and slot 10, indistinguishable from a ping by
// channel+alert alone. Without the slot check they route to the movable
// player column; they belong with the system messages.

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

// Bit per kind, for the 4b `ChatSysGroups` routing mask. A line goes to the
// system column when its kind's bit is set, otherwise to the player column.
inline unsigned ChatKindBit(ChatKind k) { return 1u << (int)k; }

// The default system set: unit + cmd + event + notice. Chat and pings are the
// conversational pair and default to the player column.
const unsigned CHATGROUPS_DEFAULT_SYS =
	(1u << CK_Unit) | (1u << CK_Cmd) | (1u << CK_Event) | (1u << CK_Notice) | (1u << CK_Other);
