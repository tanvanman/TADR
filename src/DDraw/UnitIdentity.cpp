#include "UnitIdentity.h"

#include "config.h"
#include "tamem.h"
#include "tafunctions.h"
#include "hook/hook.h"
#include "TABugFix.h"
#include "GameTickHook.h"
#include "PacketChatRouter.h"
#include "ChatHijackIds.h"
#include "iddrawsurface.h"

#include <cstring>
#include <cstdio>
#include <cstdarg>

namespace {

// ---------------------------------------------------------------------------
// Engine addresses
// ---------------------------------------------------------------------------

// ReceiveWeaponFired's projectile-kind dispatch:
//   0049D42A  MOV ECX,[ESP+0x1c]          ; &WeaponsTypedefArray[ pkt[0x19] ]
//   0049D42E  MOV ECX,[ECX+0x111]         ; ->WeaponTypeMask
//   0049D434  MOV EDX,ECX                 ; <- we resume here
// Ten bytes of whole instructions, so a 5-byte LAGGERJMP fits with room spare.
// We do NOT let the displaced bytes re-execute (they would clobber our ECX);
// we set the mask ourselves and redirect to 0x0049D434.
const DWORD kWeaponDispatchAddr   = 0x0049D42Au;
const DWORD kWeaponDispatchLen    = 10u;
const DWORD kWeaponDispatchResume = 0x0049D434u;

// UNITS_CreateFromNetwork entry:
//   004861D0  SUB ESP,0xc                 (3)
//   004861D3  MOV EAX,[ESP+0x10]          (4)
const DWORD kCreateFromNetworkAddr = 0x004861D0u;
const DWORD kCreateFromNetworkLen  = 7u;

// Return addresses of the three UNITS_CreateFromNetwork call sites. These
// discriminate which path gave a slot its identity, which is the whole point
// of the MORF breadcrumb.
// RETURN addresses, i.e. the CALL site + 5 (all three are E8 rel32). Ghidra's xrefs_to gives the
// CALL address; using that directly here made every MORF report "via ?" (game 2026-09-08).
// These are the only three callers of UNITS_CreateFromNetwork @0x004861D0.
const DWORD kRetFromPacketDispatcher = 0x004553E9u;  // CALL @004553E4, Packet_Dispatcher (0x09)
const DWORD kRetFrom2CDirtyLoop      = 0x0048BA05u;  // CALL @0048BA00, Receive_UnitStatAndMove_2C
const DWORD kRetFrom2CRoundRobin     = 0x0048B49Cu;  // CALL @0048B497, UnitMove_DeserializeAndUpdate

// UnitMove_DeserializeAndUpdate, the "owner says this slot is empty but I have
// a live unit here" arm -- the engine's own ghost detector:
//   0048B426  MOV ECX,[EAX+0x110]         (6)  <- hook here, EAX = the unit
//   0048B42C  OR  CH,0x40                      ; UnitStateMask |= PENDING_DEATH
//   0048B42F  MOV [EAX+0x110],ECX
const DWORD kGhostFlagAddr = 0x0048B426u;
const DWORD kGhostFlagLen  = 6u;

// UnitStruct weapon slots: bases at unit+0x04, +0x20, +0x3C (stride 0x1C), so
// the WeaponStruct* members land at +0x10, +0x2C, +0x48 (tamem.h Weapon1..3).
const int kWeaponSlotBase[3] = { 0x04, 0x20, 0x3C };

// Receive_UnitStatAndMove_2C dirty-loop entry validation. Hook site:
//   0048b9a3  PUSH ECX                     ; sigBits
//   0048b9a4  LEA  ECX,[ESP+0x14]          ; &bitArray  (so bitArray = ESP+0x10 at the CMP)
//   0048b9a8  CALL SerialBitArrayRead      ; AX = typeID
//   0048b9ad  CMP  word ptr [ESI+0xa6],AX  ; <- 7 bytes, we hook here. ESI = candidate unit
//
// Per entry rather than one end-of-packet assert: the 5 fatal AVs here carry ESI ~0x65A4B1C8
// against a ~0x03xxxxxx heap, which a 16-bit slotDelta (max 0x7FFF*0x118, ~557KB) cannot reach,
// so a range check alone would not have fired. Per-entry says which field goes bad first and on
// which iteration; the bit cursor alongside separates one bad field from a misframed stream.
const DWORD k2cEntryAddr = 0x0048B9ADu;
const DWORD k2cEntryLen  = 7u;
const unsigned char k2cEntryBytes[7] = { 0x66, 0x39, 0x86, 0xA6, 0x00, 0x00, 0x00 };

const unsigned k2cReasonBadUnitPtr = 1;
const unsigned k2cReasonBadTypeId  = 2;
const unsigned k2cReasonCursorOver = 4;

// ---------------------------------------------------------------------------
// Byte guards
// ---------------------------------------------------------------------------
// Mod exes are hex-edited from patch lists, and 0x0049D42A is a site we REDIRECT past rather than
// re-execute: over someone else's patch we would corrupt it and jump into the middle of it. Verify
// first, and skip the hook loudly on a mismatch.
const unsigned char kWeaponDispatchBytes[10] =
	{ 0x8B, 0x4C, 0x24, 0x1C, 0x8B, 0x89, 0x11, 0x01, 0x00, 0x00 };
const unsigned char kCreateFromNetworkBytes[7] =
	{ 0x83, 0xEC, 0x0C, 0x8B, 0x44, 0x24, 0x10 };
const unsigned char kGhostFlagBytes[6] =
	{ 0x8B, 0x88, 0x10, 0x01, 0x00, 0x00 };

bool BytesMatch(DWORD addr, const unsigned char* expect, size_t n)
{
	return std::memcmp((const void*)addr, expect, n) == 0;
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

// Soak setting: 150 ticks (~5 s). TA's own identity sweep is MaxUnits ticks (~33 s), so a
// divergence from a brief outage is created AND healed inside one 900-tick window and a slower
// audit would never see it; 150 samples such a transient ~6 times. Cost is a ~10k-slot walk (tens
// of microseconds) and a 65-byte packet per player. Raise it once the base rate is known.
//
// The sample tick is the SAME on every client (GameTime % kAuditPeriodTicks == 0), deliberately
// NOT staggered -- staggering compares snapshots taken seconds apart, and ordinary unit churn
// would then make every player disagree every time. Only the broadcast is staggered.
const int kAuditPeriodTicks = 150;

// Log the counters every this many audits even when nothing is wrong. Without
// it, a probe whose only output is silence cannot be distinguished from a probe
// that never ran -- which is exactly what happened on the first soak.
const int kHeartbeatEveryAudits = 12;   // ~60 s at 150-tick audits

// How many past audits to keep per slot, so a peer's digest can be matched to
// OUR snapshot of the same GameTime rather than to whatever we sampled last.
// 4 x 900 ticks is ~2 minutes of tolerance for packet delay and tick drift.
const int kViewHistory = 4;

// Even at equal GameTime the two clients are not lockstep: a create or death
// applied a few ticks apart shows up as a difference. So a single disagreement
// proves nothing. Only one that survives this many consecutive audits -- which
// are kAuditPeriodTicks apart, and whose transient causes are independent -- is
// a real divergence.
const int kMismatchStreakAlarm = 3;

bool g_installed = false;
InlineSingleHook* g_weaponDispatchHook = nullptr;
InlineSingleHook* g_createFromNetworkHook = nullptr;
InlineSingleHook* g_ghostFlagHook = nullptr;
InlineSingleHook* g_2cEntryHook = nullptr;

UnitIdentity::Stats g_stats = { 0, 0, 0, 0, 0, 0 };

int g_lastTick = -1;      // GameTickHook fires many times per sim tick; gate on this
int g_lastAuditTick = -1;
int g_lastBroadcastTick = -1;
unsigned g_auditCount = 0;

struct BlockView
{
	unsigned live;      // units with UnitID != 0 in the block
	unsigned digest;    // FNV-1a over (slot index, UnitID) in array order
	int      tick;      // GameTime the view was taken at
	bool     valid;
};
// [slot][history] -- our snapshots of every player's block, keyed by the audit
// tick so an arriving digest is compared against the same instant.
BlockView g_localView[10][kViewHistory];
int g_viewCursor = 0;

const BlockView* FindView(int slot, int tick)
{
	for (int i = 0; i < kViewHistory; ++i)
	{
		const BlockView& v = g_localView[slot][i];
		if (v.valid && v.tick == tick)
			return &v;
	}
	return nullptr;
}

struct PeerCheck
{
	int  streak;        // consecutive audits disagreeing with the owner
	bool alarmed;       // already logged this run of disagreements
};
PeerCheck g_peerCheck[10];

#pragma pack(push, 1)
// CHAT_05 hijack: the owner of a block states its own live count and identity
// digest. TA has never had an "am I in sync?" signal; this is it.
struct IdentityDigestPacket
{
	unsigned char  chatByte;      // 0x05
	unsigned char  nullText;      // 0x00
	unsigned char  msgId;         // ChatHijackId::UnitIdentityDigest
	unsigned char  size;          // sizeof(*this)
	unsigned char  version;
	unsigned char  ownerSlot;     // sender's Players[] index
	unsigned short live;          // live units in the sender's own block
	unsigned int   digest;
	unsigned int   gameTime;
	unsigned char  pad[49];       // MUST stay zero: byte 64 terminates the chat
};
#pragma pack(pop)
static_assert(sizeof(IdentityDigestPacket) == 65,
	"IdentityDigestPacket must fit one TA chat packet");

const unsigned char kProtocolVersion = 1;

// Rate limiter: first kBurst events, then every kThereafter'th. Keeps a burst
// visible (the shape that identifies a 0x2C framing desync) without letting it
// flood the log.
struct RateLimiter { unsigned seen; };
RateLimiter g_morphLog  = { 0 };
RateLimiter g_ghostLog  = { 0 };
RateLimiter g_weaponLog = { 0 };

bool RateLimit(RateLimiter& rl)
{
	const unsigned kBurst = 20, kThereafter = 500;
	++rl.seen;
	return rl.seen <= kBurst || (rl.seen % kThereafter) == 0;
}

const char* CallSiteName(unsigned retAddr)
{
	switch (retAddr)
	{
	case kRetFromPacketDispatcher: return "0x09pkt";
	case kRetFrom2CDirtyLoop:      return "2C-dirty";
	case kRetFrom2CRoundRobin:     return "2C-roundrobin";
	default:                       return "?";
	}
}

// Two attempts at naming the call site have now failed -- first with the CALL addresses, then with
// CALL+5 -- and Ghidra says those three are the only callers of UNITS_CreateFromNetwork. So stop
// guessing the frame layout: search a small window of the stack for any known return address and
// report the offset it was found at, and always log the raw dword so an unknown caller (an
// indirect call, or a tdraw hook thunk, which lives well above the exe image) identifies itself.
// Returns the matched value, or 0 when nothing in the window is recognised.
DWORD FindKnownCallSite(const DWORD* frame, int* foundOffset)
{
	for (int i = 0; i <= 6; ++i)
	{
		const DWORD v = frame[i];
		if (v == kRetFromPacketDispatcher || v == kRetFrom2CDirtyLoop || v == kRetFrom2CRoundRobin)
		{
			if (foundOffset) *foundOffset = i * 4;
			return v;
		}
	}
	if (foundOffset) *foundOffset = -1;
	return 0;
}

void Log(const char* fmt, ...)
{
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
	va_end(ap);
	buf[sizeof(buf) - 1] = '\0';
	IDDrawSurface::OutptTxt(buf);
}

// ---------------------------------------------------------------------------
// 1. ReceiveWeaponFired dispatch source  (+ the WPNX mismatch breadcrumb)
// ---------------------------------------------------------------------------

int __stdcall WeaponDispatchProc(PInlineX86StackBuffer pBuf)
{
	// At 0x0049D42A the prologue has run and nothing has been pushed since, so
	// the function's locals are addressable off ESP:
	//   [ESP+0x10] = pUnitWeapon (arg1 of the FireProjectile_* call below)
	//   [ESP+0x1c] = &WeaponsTypedefArray[ pkt[0x19] ]  (stored at 0x0049D2A9)
	// and EBP holds the shooter UnitStruct* (loaded at 0x0049D33E).
	const DWORD* frame = (const DWORD*)pBuf->Esp;
	const char* slot = (const char*)frame[0x10 / 4];
	WeaponStruct* packetWeapon = (WeaponStruct*)frame[0x1C / 4];
	WeaponStruct* slotWeapon = slot ? *(WeaponStruct**)(slot + 0x0C) : nullptr;

	if (slotWeapon != packetWeapon)
	{
		++g_stats.weaponMismatch;

		// Recover the shooter's slot index for the breadcrumb. EBP is the unit;
		// validate it by checking the weapon slot really belongs to it, so a
		// stale EBP can never make us dereference garbage.
		unsigned unitIdx = 0xFFFFFFFFu;
		unsigned weaponSlot = 0xFFFFFFFFu;
		UnitStruct* shooter = (UnitStruct*)pBuf->Ebp;
		if (shooter && slot)
		{
			const int delta = (int)(slot - (const char*)shooter);
			for (int i = 0; i < 3; ++i)
			{
				if (delta == kWeaponSlotBase[i])
				{
					unitIdx = (unsigned short)shooter->UnitInGameIndex;
					weaponSlot = (unsigned)i;
					break;
				}
			}
		}

		TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
		// a = shooter unit index | slot<<16   b = packet weapon def
		// c = slot weapon def                 d = GameTime
		CrashTrace_RecordEvent(TRACE_CAT_WPNX,
			(unitIdx & 0xFFFFu) | (weaponSlot << 16),
			(unsigned)packetWeapon, (unsigned)slotWeapon,
			ta ? (unsigned)ta->GameTime : 0u);
		if (RateLimit(g_weaponLog))
			Log("[UnitIdentity] WPNX unit=%u slot=%u pktWeaponVel=%u slotWeaponVel=%u t=%d",
				unitIdx & 0xFFFFu, weaponSlot,
				packetWeapon ? packetWeapon->weaponvelocity : 0u,
				slotWeapon ? slotWeapon->weaponvelocity : 0u,
				ta ? ta->GameTime : 0);
	}

#if WEAPONFIRE_DISPATCH_FROM_SLOT
	// A null p_Weapon means we have nothing better to offer than the engine's
	// own choice (UNITINFOArray[0].weapon1 can be null), so leave it alone.
	if (!slotWeapon)
		return 0;
	pBuf->Ecx = slotWeapon->WeaponTypeMask;
	pBuf->rtnAddr_Pvoid = (LPVOID)kWeaponDispatchResume;
	return X86STRACKBUFFERCHANGE;
#else
	return 0;
#endif
}

// ---------------------------------------------------------------------------
// 2CBD -- validate each 0x2C dirty-loop entry as it is consumed
// ---------------------------------------------------------------------------

RateLimiter g_2cLog = { 0 };
unsigned g_2cIteration = 0;      // reset per packet; see the terminator check below

int __stdcall TwoCEntryProc(PInlineX86StackBuffer pBuf)
{
	TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
	if (!ta)
		return 0;

	UnitStruct* candidate = (UnitStruct*)pBuf->Esi;
	const unsigned typeId = pBuf->Eax & 0xFFFFu;

	// bitArray lives at ESP+0x10 here: the PUSH at 0048b9a3 made it ESP+0x14, and
	// SerialBitArrayRead cleans that argument, so ESP is back to pre-push.
	const DWORD* bitArray = (const DWORD*)(pBuf->Esp + 0x10);
	const unsigned char* pkt = (const unsigned char*)bitArray[0];
	const unsigned cursorBytes = bitArray[1] * 4u + ((bitArray[2] + 7u) / 8u);
	// 0x2C header: [8] code, [16] byteLen, [32] GameTime -- byteLen is bytes 1..2.
	const unsigned declaredLen = pkt ? (unsigned)(pkt[1] | (pkt[2] << 8)) : 0u;

	// The sender's own block. Only entries inside it can be legitimate.
	unsigned reason = 0;
	const int ownerSlot = candidate ? (int)candidate->cOwnerID : -1;
	if (ownerSlot < 0 || ownerSlot >= 10)
	{
		reason |= k2cReasonBadUnitPtr;   // cOwnerID unreadable/insane => pointer is wild
	}
	else
	{
		PlayerStruct& p = ta->Players[ownerSlot];
		if (!p.Units || candidate < p.Units || candidate > p.UnitsAry_End)
			reason |= k2cReasonBadUnitPtr;
	}
	if (typeId >= ta->UNITINFOCount)
		reason |= k2cReasonBadTypeId;
	if (declaredLen && cursorBytes > declaredLen)
		reason |= k2cReasonCursorOver;

	++g_2cIteration;
	if (!reason)
		return 0;

	++g_stats.twoCBad;
	CrashTrace_RecordEvent(TRACE_CAT_2CBD,
		(g_2cIteration & 0xFFFFu) | (reason << 16),
		(unsigned)candidate, typeId,
		(cursorBytes & 0xFFFFu) | (declaredLen << 16));
	if (RateLimit(g_2cLog))
		Log("[UnitIdentity] 2CBD iter=%u reason=%u unit=%08X type=%u cursor=%u/%u t=%d",
			g_2cIteration, reason, (unsigned)candidate, typeId,
			cursorBytes, declaredLen, ta->GameTime);

#if TDRAW_2C_ENTRY_BAILOUT
	// Compile-time only, default OFF. Abandons the rest of this packet's dirty list by
	// jumping to 0x0048BA28, the fall-through the engine itself takes on the 0xFFFF
	// end-of-list marker, so the per-unit movement pass and trailing skim still run.
	if (reason & (k2cReasonBadUnitPtr | k2cReasonCursorOver))
	{
		pBuf->rtnAddr_Pvoid = (LPVOID)0x0048BA28;
		return X86STRACKBUFFERCHANGE;
	}
#endif
	return 0;
}

// ---------------------------------------------------------------------------
// 2. MORF -- who changed this slot's identity, and via which path
// ---------------------------------------------------------------------------

int __stdcall CreateFromNetworkProc(PInlineX86StackBuffer pBuf)
{
	// Entry hook: the prologue has not run, so __stdcall args are off ESP.
	const DWORD* args = (const DWORD*)pBuf->Esp;
	const DWORD retAddr = args[0];
	const unsigned ownerSlot = args[1] & 0xFFu;
	const unsigned char* rec = (const unsigned char*)args[2];
	if (!rec)
		return 0;

	TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
	if (!ta || !ta->BeginUnitsArray_p)
		return 0;

	const unsigned short newType = *(const unsigned short*)(rec + 1);
	const unsigned short unitIdx = *(const unsigned short*)(rec + 3);
	if (unitIdx == 0)
		return 0;   // index 0 is TA's sentinel slot

	const unsigned short oldType =
		(unsigned short)ta->BeginUnitsArray_p[unitIdx].UnitID;

	// A plain create onto a free slot is the common case and would flush the
	// ring in seconds. Only a MORPH -- reusing an occupied slot -- is
	// interesting, and morphs are rare by construction.
	if (oldType == 0)
		return 0;

	// Same type onto an occupied slot is a RE-CREATE, not an identity change: the client ends up
	// with the unit it already had, so it cannot produce the wrong p_Weapon. Counting those as
	// morphs badly overstated the danger -- a 15s outage produced 25 "morphs" of which every one
	// was type N->N. Only oldType != newType is the divergence that kills ReceiveWeaponFired.
	if (oldType == newType)
	{
		++g_stats.recreates;
		return 0;
	}

	++g_stats.morphs;
	// a = unit index | ownerSlot<<16   b = oldType | newType<<16
	// c = call site (see kRetFrom*)    d = GameTime
	CrashTrace_RecordEvent(TRACE_CAT_MORF,
		(unsigned)unitIdx | (ownerSlot << 16),
		(unsigned)oldType | ((unsigned)newType << 16),
		retAddr,
		(unsigned)ta->GameTime);

	// A breadcrumb is only readable in a crash report, and a session that does
	// not crash is precisely the one we are trying to learn from. Log it too,
	// rate-limited so a 0x2C bit-framing desync (which produces a BURST of
	// nonsense morphs inside one packet) cannot flood tdrawlog.
	if (RateLimit(g_morphLog))
	{
		int foundAt = -1;
		const DWORD known = FindKnownCallSite(args, &foundAt);
		Log("[UnitIdentity] MORF unit=%u owner=%u type %u->%u via %s (ret=%08X esp[0..6] match=%08X@+%d) t=%d",
			(unsigned)unitIdx, ownerSlot, (unsigned)oldType, (unsigned)newType,
			CallSiteName(known ? known : retAddr), retAddr, known, foundAt, ta->GameTime);
	}
	return 0;
}

// ---------------------------------------------------------------------------
// 3. GHST -- the engine's own "I have a unit the owner does not" detection
// ---------------------------------------------------------------------------

int __stdcall GhostFlagProc(PInlineX86StackBuffer pBuf)
{
	UnitStruct* u = (UnitStruct*)pBuf->Eax;
	if (!u)
		return 0;

	TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
	++g_stats.ghosts;
	// a = unit index  b = local unit type  c = owner slot  d = GameTime
	CrashTrace_RecordEvent(TRACE_CAT_GHST,
		(unsigned)(unsigned short)u->UnitInGameIndex,
		(unsigned)(unsigned short)u->UnitID,
		(unsigned)u->cOwnerID,
		ta ? (unsigned)ta->GameTime : 0u);
	if (RateLimit(g_ghostLog))
		Log("[UnitIdentity] GHST unit=%u type=%u owner=%u t=%d (owner says slot empty)",
			(unsigned)(unsigned short)u->UnitInGameIndex,
			(unsigned)(unsigned short)u->UnitID,
			(unsigned)u->cOwnerID, ta ? ta->GameTime : 0);
	return 0;
}

// ---------------------------------------------------------------------------
// 4. SYNC -- periodic block audit, locally and against the block's owner
// ---------------------------------------------------------------------------

// FNV-1a over (array index, unit type) in array order. Both clients walk the
// same block in the same order, so equal identity sets give equal digests.
unsigned WalkBlock(PlayerStruct& p, unsigned& liveOut)
{
	unsigned live = 0;
	unsigned digest = 2166136261u;
	for (UnitStruct* u = p.Units; u <= p.UnitsAry_End; ++u)
	{
		const unsigned short type = (unsigned short)u->UnitID;
		if (type == 0)
			continue;
		++live;
		const unsigned key =
			((unsigned)(unsigned short)u->UnitInGameIndex << 16) ^ (unsigned)type;
		digest = (digest ^ (key & 0xFFu)) * 16777619u;
		digest = (digest ^ ((key >> 8) & 0xFFu)) * 16777619u;
		digest = (digest ^ ((key >> 16) & 0xFFu)) * 16777619u;
		digest = (digest ^ ((key >> 24) & 0xFFu)) * 16777619u;
	}
	liveOut = live;
	return digest;
}

void BroadcastOwnDigest(TAdynmemStruct* ta, int localSlot, int tick)
{
	PlayerStruct& local = ta->Players[localSlot];
	if (!local.PlayerActive || local.DirectPlayID == 0 || !local.PlayerInfo)
		return;
	if ((local.PlayerInfo->PropertyMask & WATCH) != 0)
		return;   // watchers own no units and must not claim to

	const BlockView* view = FindView(localSlot, tick);
	if (!view)
		return;
	const BlockView& v = *view;

	IdentityDigestPacket pkt;
	std::memset(&pkt, 0, sizeof(pkt));
	pkt.chatByte  = 0x05;
	pkt.nullText  = 0x00;
	pkt.msgId     = ChatHijackId::UnitIdentityDigest;
	pkt.size      = sizeof(pkt);
	pkt.version   = kProtocolVersion;
	pkt.ownerSlot = (unsigned char)localSlot;
	pkt.live      = (unsigned short)v.live;
	pkt.digest    = v.digest;
	pkt.gameTime  = (unsigned)v.tick;

	HAPI_BroadcastMessage(local.DirectPlayID, (const char*)&pkt, sizeof(pkt));
}

void HandleIdentityDigest(unsigned fromDpid, const void* buf)
{
	const IdentityDigestPacket* pkt = (const IdentityDigestPacket*)buf;
	if (!pkt || pkt->version != kProtocolVersion || pkt->size != sizeof(*pkt))
		return;

	TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
	if (!ta)
		return;

	// Trust the DirectPlay source, not the claimed slot: a client may only
	// speak for its own block.
	int senderSlot = -1;
	for (int i = 0; i < 10; ++i)
	{
		if (ta->Players[i].PlayerActive && (unsigned)ta->Players[i].DirectPlayID == fromDpid)
		{
			senderSlot = i;
			break;
		}
	}
	if (senderSlot < 0 || senderSlot != (int)pkt->ownerSlot)
		return;

	// Compare against OUR snapshot of the same GameTime, not our latest one --
	// see the kAuditPeriodTicks comment. If we have no snapshot for that tick
	// (we joined late, or the packet is older than our history) say nothing
	// rather than guess.
	const BlockView* view = FindView(senderSlot, (int)pkt->gameTime);
	if (!view)
		return;
	const BlockView& mine = *view;

	PeerCheck& chk = g_peerCheck[senderSlot];
	const bool agrees = (mine.live == pkt->live) && (mine.digest == pkt->digest);
	if (agrees)
	{
		chk.streak = 0;
		chk.alarmed = false;
		return;
	}

	++chk.streak;

	// Report the FIRST disagreement as an observation, immediately. A transient
	// -- a create or death applied a few ticks apart -- is exactly what we want
	// to see when probing with an induced outage, and the streak rule below
	// would filter it out. The two are different findings and want different
	// weight, so keep them separate rather than weakening the alarm.
	if (chk.streak == 1)
		Log("[UnitIdentity] transient mismatch vs %.30s (slot %d) at t=%u: "
			"mine %u/%08X owner %u/%08X",
			ta->Players[senderSlot].Name, senderSlot, (unsigned)pkt->gameTime,
			mine.live, mine.digest, (unsigned)pkt->live, pkt->digest);

	if (chk.streak < kMismatchStreakAlarm || chk.alarmed)
		return;

	// Persistent disagreement: this is a real divergence, not a sampling skew.
	chk.alarmed = true;
	++g_stats.peerMismatch;
	CrashTrace_RecordEvent(TRACE_CAT_SYNC,
		(unsigned)senderSlot | (0x8000u),          // 0x8000 marks a peer mismatch
		((unsigned)mine.live << 16) | (unsigned)pkt->live,
		mine.digest ^ pkt->digest,
		(unsigned)ta->GameTime);
	Log("[UnitIdentity] DESYNC vs %.30s (slot %d): my view %u units digest %08X, "
		"owner says %u / %08X (t=%u, %d consecutive audits)",
		ta->Players[senderSlot].Name, senderSlot,
		mine.live, mine.digest, (unsigned)pkt->live, pkt->digest,
		(unsigned)pkt->gameTime, chk.streak);
}

void RunAudit(TAdynmemStruct* ta, int gameTime)
{
	const int cursor = g_viewCursor;
	g_viewCursor = (g_viewCursor + 1) % kViewHistory;

	for (int slot = 0; slot < 10; ++slot)
	{
		PlayerStruct& p = ta->Players[slot];
		BlockView& view = g_localView[slot][cursor];
		view.valid = false;
		if (!p.PlayerActive || !p.Units || !p.UnitsAry_End || p.Units > p.UnitsAry_End)
			continue;

		unsigned live = 0;
		const unsigned digest = WalkBlock(p, live);

		view.live   = live;
		view.digest = digest;
		view.tick   = gameTime;
		view.valid  = true;

		// Local half: nNumUnits is maintained by increments in
		// UNITS_Create{Unit,FromNetwork} and a decrement in
		// UNITS_ReceiveUnitDeath. It should equal the occupied slots in the
		// block. A drift needs no network at all to detect and is exactly the
		// "+7 in one player's block" signature from game 190441.
		const unsigned counted = (unsigned)(unsigned short)p.UnitsNumber;
		if (counted != live)
		{
			++g_stats.countDrift;
			CrashTrace_RecordEvent(TRACE_CAT_SYNC,
				(unsigned)slot, live, counted, (unsigned)gameTime);
			Log("[UnitIdentity] nNumUnits drift for %.30s (slot %d): counter=%u walked=%u (t=%d)",
				p.Name, slot, counted, live, gameTime);
		}
	}
}

void OnGameTick(int /*unused*/)
{
	if (!g_installed)
		return;
	TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
	if (!ta)
		return;

	// GameTickHook fires 9-15 times per SIM tick, so gate on GameTime itself.
	const int gameTime = ta->GameTime;
	if (gameTime == g_lastTick)
		return;
	if (gameTime < g_lastTick)      // new game in the same process
	{
		std::memset(g_localView, 0, sizeof(g_localView));
		std::memset(g_peerCheck, 0, sizeof(g_peerCheck));
		g_viewCursor = 0;
		g_lastAuditTick = -1;
		g_lastBroadcastTick = -1;
	}
	g_lastTick = gameTime;
	if (gameTime <= 0)
		return;

	const int localSlot = ta->LocalHumanPlayer_PlayerID;
	if (localSlot < 0 || localSlot >= 10)
		return;

	// Sample on the same tick as everybody else, so the digests we exchange
	// describe the same instant.
	if ((gameTime % kAuditPeriodTicks) == 0 && gameTime != g_lastAuditTick)
	{
		g_lastAuditTick = gameTime;
		RunAudit(ta, gameTime);

		// Heartbeat: prove the probes are alive even when they find nothing.
		if (++g_auditCount % kHeartbeatEveryAudits == 0)
		{
			int agreeing = 0, blocks = 0;
			for (int i = 0; i < 10; ++i)
			{
				if (!FindView(i, gameTime)) continue;
				++blocks;
				if (g_peerCheck[i].streak == 0) ++agreeing;
			}
			Log("[UnitIdentity] audit #%u t=%d blocks=%d agreeing=%d | "
				"morph=%u recreate=%u ghost=%u wpnx=%u drift=%u desync=%u 2cbd=%u",
				g_auditCount, gameTime, blocks, agreeing,
				g_stats.morphs, g_stats.recreates, g_stats.ghosts, g_stats.weaponMismatch,
				g_stats.countDrift, g_stats.peerMismatch, g_stats.twoCBad);
		}
	}

	// Stagger only the send, so ten clients don't broadcast on one tick. The
	// packet still carries the shared audit tick it describes.
	const int sendOffset = 1 + localSlot * 3;
	if (g_lastAuditTick > 0
		&& gameTime == g_lastAuditTick + sendOffset
		&& gameTime != g_lastBroadcastTick)
	{
		g_lastBroadcastTick = gameTime;
		BroadcastOwnDigest(ta, localSlot, g_lastAuditTick);
	}
}

} // namespace

namespace UnitIdentity {

void Install()
{
	if (g_installed)
		return;
	g_installed = true;

	std::memset(g_localView, 0, sizeof(g_localView));
	std::memset(g_peerCheck, 0, sizeof(g_peerCheck));

	if (BytesMatch(kWeaponDispatchAddr, kWeaponDispatchBytes, sizeof(kWeaponDispatchBytes)))
		g_weaponDispatchHook = new InlineSingleHook(
			kWeaponDispatchAddr, kWeaponDispatchLen,
			INLINE_5BYTESLAGGERJMP, (InlineX86HookRouter)WeaponDispatchProc);
	else
		Log("[UnitIdentity] byte guard FAILED at %08X (ReceiveWeaponFired dispatch) "
			"- hook not installed", kWeaponDispatchAddr);

	if (BytesMatch(kCreateFromNetworkAddr, kCreateFromNetworkBytes, sizeof(kCreateFromNetworkBytes)))
		g_createFromNetworkHook = new InlineSingleHook(
			kCreateFromNetworkAddr, kCreateFromNetworkLen,
			INLINE_5BYTESLAGGERJMP, (InlineX86HookRouter)CreateFromNetworkProc);
	else
		Log("[UnitIdentity] byte guard FAILED at %08X (UNITS_CreateFromNetwork) "
			"- MORF breadcrumb disabled", kCreateFromNetworkAddr);

	if (BytesMatch(kGhostFlagAddr, kGhostFlagBytes, sizeof(kGhostFlagBytes)))
		g_ghostFlagHook = new InlineSingleHook(
			kGhostFlagAddr, kGhostFlagLen,
			INLINE_5BYTESLAGGERJMP, (InlineX86HookRouter)GhostFlagProc);
	else
		Log("[UnitIdentity] byte guard FAILED at %08X (ghost flag) "
			"- GHST breadcrumb disabled", kGhostFlagAddr);

	if (BytesMatch(k2cEntryAddr, k2cEntryBytes, sizeof(k2cEntryBytes)))
		g_2cEntryHook = new InlineSingleHook(
			k2cEntryAddr, k2cEntryLen,
			INLINE_5BYTESLAGGERJMP, (InlineX86HookRouter)TwoCEntryProc);
	else
		Log("[UnitIdentity] byte guard FAILED at %08X (2C dirty entry) "
			"- 2CBD breadcrumb disabled", k2cEntryAddr);

#if UNIT_IDENTITY_AUDIT_ENABLE
	PacketChatRouter::GetInstance()->RegisterHandler(
		ChatHijackId::UnitIdentityDigest, HandleIdentityDigest, /*fireInDemo=*/true);
	GameTickHook::GetInstance()->addCallback(OnGameTick);
#endif

	Log("[UnitIdentity] installed (dispatch-from-slot=%d, audit=%d, 2c-bailout=%d)",
		(int)WEAPONFIRE_DISPATCH_FROM_SLOT, (int)UNIT_IDENTITY_AUDIT_ENABLE,
		(int)TDRAW_2C_ENTRY_BAILOUT);
}

void Shutdown()
{
	g_installed = false;
	delete g_weaponDispatchHook;    g_weaponDispatchHook = nullptr;
	delete g_createFromNetworkHook; g_createFromNetworkHook = nullptr;
	delete g_ghostFlagHook;         g_ghostFlagHook = nullptr;
	delete g_2cEntryHook;           g_2cEntryHook = nullptr;
}

const Stats& GetStats()
{
	return g_stats;
}

} // namespace UnitIdentity
