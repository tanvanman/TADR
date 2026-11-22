#pragma once

// Per-player, local-only mute for chat, map pings and whiteboard drawings.
//
// Desync-safe: nothing here touches simulation state.
//   * The chat filter cancels Net_PushChatHudMessage @0x463CA0 (writes the HUD
//     ring and plays a UI sound; the packet is already processed by then).
//   * The whiteboard filter drops overlay elements on TADR's recorder channel
//     (0xFB), which no client simulates.
//   * `.mute` is cancelled at Chat_FormatAndSend @0x463E50 -- upstream of
//     formatting, of Chat_SendOutgoingMessage @0x453360, and of the local echo
//     that follows. (Hooking 0x453360 directly does not work: it only sees the
//     already-formatted string and its echo fires regardless -- see .cpp.)
//
// Never muted, deliberately: playerIndex 10 (system sentinel -- unit alerts,
// TADR notices, self-chat); channel 4 (eliminations/leaves); channel 1 with a
// non-zero alert payload (unit alerts -- the alert camera walks the ring and
// reads entry+0x44); non-chat channel-8 lines (pause/ready).
//
// Channel facts:
//   chan 8, text '<...'  -> player chat         (mutable: Chat)
//   chan 8, other        -> pause/ready         (never)
//   chan 1, alert == 0   -> marker/ping echo    (mutable: Pings)
//   chan 1, alert != 0   -> unit alert          (never)
//   chan 4               -> elimination/leave   (never)
//   chan 2               -> never seen live     (never)
//
// Known limits: drawing DELETEs carry no colour byte, so a muted player can
// still erase your drawings; whiteboard sender identity is a colour heuristic,
// so a mid-game colour change leaks past a mute (both inherited from the
// existing whiteboard code). Mutes are session-scoped, keyed by slot, and
// cleared automatically when the player set changes.

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
