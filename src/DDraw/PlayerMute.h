#pragma once

// Per-player, local-only mute for chat, map pings and whiteboard drawings.
//
// WHY THIS IS DESYNC-SAFE
// -----------------------
// Nothing here touches simulation state. Two clients with different mute
// settings compute byte-identical simulation state. Specifically:
//
//   * The chat filter cancels Net_PushChatHudMessage @0x463CA0, which only
//     writes the HUD ring and plays a UI sound. The packet has already been
//     received and processed by the time it is reached.
//   * The whiteboard filter drops overlay elements delivered on TADR's
//     recorder-to-recorder channel (0xFB), which no client simulates.
//   * The `.mute` command is cancelled at Chat_FormatAndSend @0x463E50 —
//     upstream of formatting, of Chat_SendOutgoingMessage @0x453360 (and
//     therefore Net_BroadcastGuaranteed), AND of the unconditional local-echo
//     call to Net_PushChatHudMessage that follows it. See PlayerMute.cpp for
//     why the first implementation (hooking 0x453360 directly) didn't work:
//     that site only ever sees the already-formatted "<Name> text" string,
//     and its local echo does not depend on the send succeeding.
//
// WHAT IS NEVER MUTED (deliberate)
// --------------------------------
//   * playerIndex 10 — the system sentinel. Unit alerts ("Rocko: Under
//     Attack"), TADR's own local messages and self-typed chat all carry 10.
//   * Channel 4 — eliminations and leaves ("X has been eradicated"). You
//     need to know a player died even if you muted them.
//   * Channel 1 with a non-zero alert payload — unit alerts. The alert
//     camera (Camera_CenterOnNextChatAlertUnit @0x463F60) walks the ring
//     and reads entry+0x44; suppressing these would break jump-to-alert.
//   * Non-chat channel-8 lines ("X paused the game", "*** X ready"). Those
//     share channel 8 with chat but are game state you must still see.
//
// CHANNEL FACTS (verified from live replay ring dumps, 2026-08-14)
// ---------------------------------------------------------------
//   chan 8, text starts '<'  -> player chat            (mutable: Chat)
//   chan 8, other            -> pause/ready notices    (never muted)
//   chan 1, alert == 0       -> marker/ping echo       (mutable: Pings)
//   chan 1, alert != 0       -> unit alert, slot 10    (never muted)
//   chan 4                   -> elimination / leave    (never muted)
//   chan 2                   -> never observed live; treated as never muted
//
// KNOWN LIMITS (tell the player, do not pretend otherwise)
// -------------------------------------------------------
//   * Drawing DELETES cannot be attributed. PacketDeleteOn / PacketDeleteArea
//     carry no colour byte, so a muted player can still erase your drawings.
//     Fixing that needs a whiteboard wire-format change.
//   * Whiteboard sender identity is a COLOUR heuristic, not a player id.
//     A player who changes colour mid-game can leak markers past a mute.
//     This is inherited from the existing whiteboard code, not introduced here.
//   * Mutes are session-scoped and keyed by slot. They are cleared
//     automatically when the set of players in the game changes, because a
//     slot number means a different person in the next game.

struct PlayerStruct;

namespace PlayerMute
{
	enum Category
	{
		CatChat  = 1,   // <Name> ... chat lines, and their arrival sound
		CatPings = 2,   // map markers: the marker itself, its minimap flash,
		                // and the "*Name added a new marker" chat echo
		CatDraw  = 4,   // freehand whiteboard lines
		CatAll   = 7
	};

	// Slot 10 is the system sentinel and can never be muted.
	const int kMaxPlayers  = 10;
	const int kSystemSlot  = 10;

	// Installs the chat-ring filter (0x463CA0) and the local command
	// interceptor (0x463E50). Call once during DLL init.
	void Install();
	void Shutdown();

	// Query. `slot` outside [0,10) always returns false.
	bool IsMuted(int slot, Category cat);

	// True when any category is muted for this slot.
	bool IsAnyMuted(int slot);

	void SetMuted(int slot, Category cat, bool on);

	// Toggles `cat` and returns the new state.
	bool Toggle(int slot, Category cat);

	// Clears every mute. Called automatically on a player-set change.
	void ResetAll();

	// Clears the mask if the players in the game are not the ones the mask
	// was built against. Cheap (a 300-byte compare); safe to call often.
	void SyncSession();

	// Case-insensitive lookup over active players' names. Accepts an exact
	// match first, then a unique prefix match. Returns -1 for "not found"
	// and -2 for "ambiguous prefix".
	int FindSlotByName(const char* name);

	// Writes a line to the local chat HUD only. Never broadcasts.
	// (Equivalent to NewChatText(msg, 1, 0, 10).)
	void LocalNotice(const char* msg);
}
