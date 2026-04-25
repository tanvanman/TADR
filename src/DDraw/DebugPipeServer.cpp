#include "DebugPipeServer.h"

#ifdef TADR_DEBUG_PIPE

#include "VoteReject.h"
#include "iddrawsurface.h"
#include "tamem.h"
#include "tahook.h"
#include "UnitDefExtensions.h"
#include "unitrotate.h"

#include <windows.h>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>

// -----------------------------------------------------------------------
// Pending command: written by pipe thread, executed by render thread.
// -----------------------------------------------------------------------
struct PendingCmd
{
    std::string line;
    std::string response;
    HANDLE      doneEvent;
};

static CRITICAL_SECTION       g_queueCs;
static std::vector<PendingCmd*> g_queue;

static volatile bool g_running = false;
static HANDLE        g_thread  = NULL;

// -----------------------------------------------------------------------
// DrainQueue: called from the render thread (VoteReject::Tick).
//   Executes all queued commands and signals the pipe thread when done.
// -----------------------------------------------------------------------
void DebugPipeServer::DrainQueue()
{
    EnterCriticalSection(&g_queueCs);
    std::vector<PendingCmd*> batch;
    batch.swap(g_queue);
    LeaveCriticalSection(&g_queueCs);

    for (PendingCmd* cmd : batch)
    {
        cmd->response = ExecuteCommand(cmd->line);
        SetEvent(cmd->doneEvent);
    }
}

// -----------------------------------------------------------------------
// Commands that are safe to execute directly on the pipe thread (no
// render-thread synchronisation needed — they only read state that
// changes slowly or not at all during a frame).
// -----------------------------------------------------------------------
static bool IsDirectCommand(const std::string& cmd)
{
    return cmd == "get_progress"
        || cmd == "find_unit_idx"
        || cmd == "dump_unit_name"
        || cmd == "dump_bytes"
        || cmd == "get_unit_ext_string"
        || cmd == "get_unit_ext_int"
        || cmd == "get_rotate_state"
        || cmd == "set_mouse_map_pos"
        || cmd == "press_key"
        || cmd == "post_key"
        || cmd == "click"
        || cmd == "send_input_key"
        || cmd == "send_char"
        || cmd == "inject_slash_key"
        || cmd == "ta_key_down"
        || cmd == "ta_key_press"
        || cmd == "get_submenu"
        || cmd == "list_ude_keys"
        || cmd == "set_build_unit"
        || cmd == "check_rotatable"
        || cmd == "mash_s"
        || cmd == "drive_to_game";
}

// -----------------------------------------------------------------------
// PostAndWait: enqueue a command from the pipe thread and block until
//   the render thread executes it (timeout 5 s).
// -----------------------------------------------------------------------
static std::string PostAndWait(const std::string& line)
{
    // Peek the command name. If it's in the direct-execution set, run it
    // straight here — the render-thread drain loop isn't reliably called
    // during TA's title / loading / menu states, so the queue would just
    // time out.
    std::istringstream peek(line);
    std::string cmdName;
    peek >> cmdName;
    if (IsDirectCommand(cmdName))
    {
        return DebugPipeServer::ExecuteCommand(line);
    }

    PendingCmd* cmd = new PendingCmd;
    cmd->line       = line;
    cmd->doneEvent  = CreateEventA(NULL, FALSE, FALSE, NULL);

    EnterCriticalSection(&g_queueCs);
    g_queue.push_back(cmd);
    LeaveCriticalSection(&g_queueCs);

    WaitForSingleObject(cmd->doneEvent, 5000);
    std::string resp = cmd->response;
    CloseHandle(cmd->doneEvent);
    delete cmd;
    return resp;
}

// -----------------------------------------------------------------------
// ExecuteCommand: parse and run one command on the render thread.
// -----------------------------------------------------------------------
std::string DebugPipeServer::ExecuteCommand(const std::string& line)
{
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;
    if (cmd.empty())
        return "OK";

    // setup_player <slot> <dpid> <name>
    if (cmd == "setup_player")
    {
        int      slot;
        unsigned dpid;
        std::string name;
        if (!(iss >> slot >> dpid)) return "ERR bad args";
        std::getline(iss >> std::ws, name);
        if (slot < 0 || slot >= 10) return "ERR slot out of range";

        TAdynmemStruct* ta = *(TAdynmemStruct**)0x00511de8;
        if (!ta) return "ERR taPtr null";

        PlayerStruct& p = ta->Players[slot];
        p.PlayerActive  = 1;
        p.DirectPlayID  = (int)dpid;
        p.My_PlayerType = Player_RemoteHuman;
        strncpy_s(p.Name, sizeof(p.Name), name.c_str(), _TRUNCATE);
        return "OK";
    }

    // clear_player <slot>
    if (cmd == "clear_player")
    {
        int slot;
        if (!(iss >> slot)) return "ERR bad args";
        if (slot < 0 || slot >= 10) return "ERR slot out of range";

        TAdynmemStruct* ta = *(TAdynmemStruct**)0x00511de8;
        if (!ta) return "ERR taPtr null";

        memset(&ta->Players[slot], 0, sizeof(PlayerStruct));
        return "OK";
    }

    // set_local <slot>
    if (cmd == "set_local")
    {
        int slot;
        if (!(iss >> slot)) return "ERR bad args";
        if (slot < 0 || slot >= 10) return "ERR slot out of range";

        TAdynmemStruct* ta = *(TAdynmemStruct**)0x00511de8;
        if (!ta) return "ERR taPtr null";

        ta->LocalHumanPlayer_PlayerID = (char)slot;
        return "OK";
    }

    // set_progress <n>   (1=TALobby 2=TALoading 3=TAInGame 4=TAExiting)
    if (cmd == "set_progress")
    {
        int n;
        if (!(iss >> n)) return "ERR bad args";
        DataShare->TAProgress  = n;
        DataShare->PlayingDemo = 0;
        return "OK";
    }

    // suppress_broadcast <0|1>
    if (cmd == "suppress_broadcast")
    {
        int v;
        if (!(iss >> v)) return "ERR bad args";
        VoteReject::GetInstance()->SetSuppressBroadcast(v != 0);
        return "OK";
    }

    // inject_propose <fromDpid> <targetDpid> <rejectMask>
    if (cmd == "inject_propose")
    {
        unsigned fromDpid, targetDpid;
        int mask;
        if (!(iss >> fromDpid >> targetDpid >> mask)) return "ERR bad args";

        VoteRejectMessage msg;
        memset(&msg, 0, sizeof(msg));
        msg.chatByte   = 0x05;
        msg.nullText   = 0x00;
        msg.msgId      = 0x2c;
        msg.size       = sizeof(VoteRejectMessage);
        msg.command    = VoteRejectCommand::ProposeVote;
        msg.targetDpid = targetDpid;
        msg.rejectMask = (char)mask;

        VoteReject::GetInstance()->InjectReceive(fromDpid, msg);
        return "OK";
    }

    // inject_yes <fromDpid> <targetDpid>
    if (cmd == "inject_yes")
    {
        unsigned fromDpid, targetDpid;
        if (!(iss >> fromDpid >> targetDpid)) return "ERR bad args";

        VoteRejectMessage msg;
        memset(&msg, 0, sizeof(msg));
        msg.chatByte   = 0x05;
        msg.nullText   = 0x00;
        msg.msgId      = 0x2c;
        msg.size       = sizeof(VoteRejectMessage);
        msg.command    = VoteRejectCommand::CastVote;
        msg.targetDpid = targetDpid;
        msg.rejectMask = 1;

        VoteReject::GetInstance()->InjectReceive(fromDpid, msg);
        return "OK";
    }

    // inject_no <fromDpid> <targetDpid>
    if (cmd == "inject_no")
    {
        unsigned fromDpid, targetDpid;
        if (!(iss >> fromDpid >> targetDpid)) return "ERR bad args";

        VoteRejectMessage msg;
        memset(&msg, 0, sizeof(msg));
        msg.chatByte   = 0x05;
        msg.nullText   = 0x00;
        msg.msgId      = 0x2c;
        msg.size       = sizeof(VoteRejectMessage);
        msg.command    = VoteRejectCommand::CastNoVote;
        msg.targetDpid = targetDpid;
        msg.rejectMask = 1;

        VoteReject::GetInstance()->InjectReceive(fromDpid, msg);
        return "OK";
    }

    // local_yes <targetDpid>
    if (cmd == "local_yes")
    {
        unsigned targetDpid;
        if (!(iss >> targetDpid)) return "ERR bad args";
        VoteReject::GetInstance()->CastLocalYesVote(targetDpid);
        return "OK";
    }

    // local_no <targetDpid>
    if (cmd == "local_no")
    {
        unsigned targetDpid;
        if (!(iss >> targetDpid)) return "ERR bad args";
        VoteReject::GetInstance()->CastLocalNoVote(targetDpid);
        return "OK";
    }

    // expire_vote <targetDpid>
    //   Sets the vote's expiry time to now-1 so it fires on the next Tick().
    if (cmd == "expire_vote")
    {
        unsigned targetDpid;
        if (!(iss >> targetDpid)) return "ERR bad args";
        VoteReject::GetInstance()->ExpireVote(targetDpid);
        return "OK";
    }

    // reset_votes
    //   Clears all vote state (votes, transient notices, take windows).
    if (cmd == "reset_votes")
    {
        VoteReject::GetInstance()->ResetAllVotes();
        return "OK";
    }

    // dump_votes
    //   Returns a single-line JSON snapshot of current vote state.
    if (cmd == "dump_votes")
    {
        return VoteReject::GetInstance()->DumpVotes();
    }

    // set_last_msg_ts <slot> <value>
    //   Write PlayerStruct.LastMsgTimeStamp (in GameRunSec units).
    //   Used in tests to simulate a timed-out player without waiting 30s.
    if (cmd == "set_last_msg_ts")
    {
        int slot, value;
        if (!(iss >> slot >> value)) return "ERR bad args";
        if (slot < 0 || slot >= 10) return "ERR slot out of range";
        TAdynmemStruct* ta = *(TAdynmemStruct**)0x00511de8;
        if (!ta) return "ERR taPtr null";
        ta->Players[slot].LastMsgTimeStamp = value;
        return "OK";
    }

    // set_game_time <value>
    //   Write the GameTimeSec global (0x00512C7C) used for dropout gap calculation.
    if (cmd == "set_game_time")
    {
        int value;
        if (!(iss >> value)) return "ERR bad args";
        *((int*)0x00512C7C) = value;
        return "OK";
    }

    // get_game_time
    //   Read the current GameRunSec() value so callers can compute relative timestamps.
    if (cmd == "get_game_time")
    {
        typedef int(__cdecl* _GameRunSec)(void);
        static _GameRunSec GameRunSec_fn = (_GameRunSec)0x004b6340;
        char buf[32];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "OK %d", GameRunSec_fn());
        return buf;
    }

    // force_dropout_check
    //   Run the dropout detection scan immediately, ignoring TAProgress.
    //   Uses the same gap logic as Tick() (calls GameRunSec() directly).
    //   Returns "OK <count>" where count = number of players ProposeReject was called for.
    if (cmd == "force_dropout_check")
    {
        static const int GAME_DROP_THRESHOLD = 900;
        typedef int(__cdecl* _GameRunSec)(void);
        static _GameRunSec GameRunSec_fn = (_GameRunSec)0x004b6340;
        int gameNow = GameRunSec_fn();
        TAdynmemStruct* ta = *(TAdynmemStruct**)0x00511de8;
        if (!ta) return "ERR taPtr null";
        unsigned myDpid = ta->Players[ta->LocalHumanPlayer_PlayerID].DirectPlayID;
        int count = 0;
        for (int i = 0; i < 10; ++i)
        {
            PlayerStruct& p = ta->Players[i];
            if (!p.PlayerActive || p.DirectPlayID == 0 || p.DirectPlayID == myDpid) continue;
            if (p.My_PlayerType != Player_RemoteHuman) continue;
            if (p.LastMsgTimeStamp == 0) continue;
            int gap = gameNow - p.LastMsgTimeStamp;
            if (gap > GAME_DROP_THRESHOLD)
            {
                VoteReject::GetInstance()->ProposeTimeoutReject(p.DirectPlayID);
                ++count;
            }
        }
        char buf[32];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "OK %d", count);
        return buf;
    }

    // dump_player_state <slot>
    //   Return key dropout-detection fields for a player slot (for test diagnostics).
    if (cmd == "dump_player_state")
    {
        int slot;
        if (!(iss >> slot)) return "ERR bad args";
        if (slot < 0 || slot >= 10) return "ERR slot out of range";
        TAdynmemStruct* ta = *(TAdynmemStruct**)0x00511de8;
        if (!ta) return "ERR taPtr null";
        PlayerStruct& p = ta->Players[slot];
        int gameNow = *((int*)0x00512C7C);
        int gap = gameNow - p.LastMsgTimeStamp;
        char buf[256];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE,
            "OK active=%d dpid=%u type=%d ts=%d gameNow=%d gap=%d progress=%d",
            p.PlayerActive, p.DirectPlayID, (int)(unsigned char)p.My_PlayerType,
            p.LastMsgTimeStamp, gameNow, gap, (int)DataShare->TAProgress);
        return buf;
    }

    // set_ally <slotA> <slotB> <0|1>
    //   Set AllyFlagAry[slotA][slotB] and [slotB][slotA] symmetrically.
    if (cmd == "set_ally")
    {
        int slotA, slotB, val;
        if (!(iss >> slotA >> slotB >> val)) return "ERR bad args";
        if (slotA < 0 || slotA >= 10 || slotB < 0 || slotB >= 10) return "ERR slot out of range";
        TAdynmemStruct* ta = *(TAdynmemStruct**)0x00511de8;
        if (!ta) return "ERR taPtr null";
        ta->Players[slotA].AllyFlagAry[slotB] = (char)val;
        ta->Players[slotB].AllyFlagAry[slotA] = (char)val;
        return "OK";
    }

    // send_input_key <vk>
    //   Use SendInput to inject a real hardware-level keystroke. Required
    //   when the target polls GetAsyncKeyState rather than consuming window
    //   messages (e.g. TA's menu navigation).
    //   The target window must be the FOREGROUND window; the command will
    //   SetForegroundWindow first.
    if (cmd == "send_input_key")
    {
        std::string arg;
        if (!(iss >> arg)) return "ERR bad args";
        unsigned vk = 0;
        if (arg.size() > 2 && arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X'))
            vk = (unsigned)strtoul(arg.c_str() + 2, nullptr, 16);
        else
            vk = (unsigned)strtoul(arg.c_str(), nullptr, 10);
        CTAHook* tahook = (CTAHook*)LocalShare->TAHook;
        HWND hwnd = tahook ? tahook->TAhWnd : NULL;
        if (!hwnd) return "ERR no TAhWnd";
        SetForegroundWindow(hwnd);
        Sleep(50);
        INPUT inputs[2] = { 0 };
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = (WORD)vk;
        inputs[0].ki.wScan = (WORD)MapVirtualKeyA((UINT)vk, 0);
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = (WORD)vk;
        inputs[1].ki.wScan = (WORD)MapVirtualKeyA((UINT)vk, 0);
        inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
        UINT sent = SendInput(2, inputs, sizeof(INPUT));
        char buf[32];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "OK sent=%u", sent);
        return buf;
    }

    // click <x> <y>
    //   PostMessage WM_MOUSEMOVE + WM_LBUTTONDOWN + WM_LBUTTONUP to TA's HWND.
    //   Coordinates are client-area pixels. This tests whether TA's menu
    //   consumes mouse messages via the wndproc (ConstructionKickout handles
    //   these for in-game clicks, suggesting the path works).
    if (cmd == "click")
    {
        int x, y;
        if (!(iss >> x >> y)) return "ERR bad args";
        CTAHook* tahook = (CTAHook*)LocalShare->TAHook;
        HWND hwnd = tahook ? tahook->TAhWnd : NULL;
        if (!hwnd) return "ERR no TAhWnd";
        LPARAM lp = MAKELPARAM((WORD)x, (WORD)y);
        PostMessageA(hwnd, WM_MOUSEMOVE,   0,            lp);
        PostMessageA(hwnd, WM_LBUTTONDOWN, MK_LBUTTON,   lp);
        PostMessageA(hwnd, WM_LBUTTONUP,   0,            lp);
        return "OK";
    }

    // post_key <vk>
    //   Use Win32 PostMessage to inject WM_KEYDOWN + WM_KEYUP into TA's
    //   message queue. Unlike press_key (which direct-calls the wnd proc),
    //   this goes through the OS input path and may be picked up by code that
    //   checks GetAsyncKeyState / PeekMessage.
    if (cmd == "post_key")
    {
        std::string arg;
        if (!(iss >> arg)) return "ERR bad args";
        unsigned vk = 0;
        if (arg.size() > 2 && arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X'))
            vk = (unsigned)strtoul(arg.c_str() + 2, nullptr, 16);
        else
            vk = (unsigned)strtoul(arg.c_str(), nullptr, 10);
        CTAHook* tahook = (CTAHook*)LocalShare->TAHook;
        HWND hwnd = tahook ? tahook->TAhWnd : NULL;
        if (!hwnd) return "ERR no TAhWnd";
        PostMessageA(hwnd, WM_KEYDOWN, (WPARAM)vk, 0);
        PostMessageA(hwnd, WM_KEYUP,   (WPARAM)vk, 0);
        return "OK";
    }

    // press_key <vk>
    //   Post WM_KEYDOWN + WM_KEYUP to TA's window proc via LocalShare->TAWndProc.
    //   vk is decimal or 0x-prefixed hex.
    if (cmd == "press_key")
    {
        std::string arg;
        if (!(iss >> arg)) return "ERR bad args";
        unsigned vk = 0;
        if (arg.size() > 2 && arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X'))
            vk = (unsigned)strtoul(arg.c_str() + 2, nullptr, 16);
        else
            vk = (unsigned)strtoul(arg.c_str(), nullptr, 10);

        CTAHook* tahook = (CTAHook*)LocalShare->TAHook;
        HWND hwnd = tahook ? tahook->TAhWnd : NULL;
        if (!hwnd) return "ERR no TAhWnd";
        if (!LocalShare->TAWndProc) return "ERR no TAWndProc";
        LocalShare->TAWndProc(hwnd, WM_KEYDOWN, (WPARAM)vk, 0);
        LocalShare->TAWndProc(hwnd, WM_KEYUP,   (WPARAM)vk, 0);
        return "OK";
    }

    // send_char <ascii>
    //   Post WM_CHAR. Useful for menu navigation where TA consumes WM_CHAR.
    if (cmd == "send_char")
    {
        int c;
        if (!(iss >> c)) return "ERR bad args";
        CTAHook* tahook = (CTAHook*)LocalShare->TAHook;
        HWND hwnd = tahook ? tahook->TAhWnd : NULL;
        if (!hwnd) return "ERR no TAhWnd";
        if (!LocalShare->TAWndProc) return "ERR no TAWndProc";
        LocalShare->TAWndProc(hwnd, WM_CHAR, (WPARAM)c, 0);
        return "OK";
    }

    // get_progress
    if (cmd == "get_progress")
    {
        char buf[32];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "OK %d", (int)DataShare->TAProgress);
        return buf;
    }

    // dump_bytes <addr> <count>
    //   Hex dump of process memory at the given address (hex). Debug aid
    //   for verifying hook installation / unit struct contents.
    if (cmd == "dump_bytes")
    {
        std::string addrStr;
        unsigned count;
        if (!(iss >> addrStr >> count)) return "ERR bad args";
        if (count > 64) count = 64;
        unsigned addr = 0;
        if (addrStr.size() > 2 && addrStr[0] == '0' && (addrStr[1] == 'x' || addrStr[1] == 'X'))
            addr = (unsigned)strtoul(addrStr.c_str() + 2, nullptr, 16);
        else
            addr = (unsigned)strtoul(addrStr.c_str(), nullptr, 16);
        char buf[256] = "OK ";
        char* p = buf + 3;
        BYTE* src = (BYTE*)addr;
        for (unsigned i = 0; i < count && (p + 3) < buf + sizeof(buf); ++i)
        {
            _snprintf_s(p, 4, _TRUNCATE, "%02X ", src[i]);
            p += 3;
        }
        if (p > buf + 3) *(p - 1) = '\0';
        return buf;
    }

    // dump_unit_name <typeID>
    //   Returns the UnitDef[typeID].Name / UnitName / ObjectName fields.
    //   Useful for verifying whether TA is reading the loose FBI.
    if (cmd == "dump_unit_name")
    {
        unsigned typeID;
        if (!(iss >> typeID)) return "ERR bad args";
        TAdynmemStruct* ta = *(TAdynmemStruct**)0x00511de8;
        if (!ta) return "ERR taPtr null";
        if (typeID == 0 || typeID >= ta->UNITINFOCount) return "ERR typeID out of range";
        UnitDefStruct& u = ta->UnitDef[typeID];
        char buf[256];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE,
            "OK Name=\"%s\" UnitName=\"%s\" ObjectName=\"%s\" FootX=%d FootY=%d",
            u.Name, u.UnitName, u.ObjectName, (int)u.FootX, (int)u.FootY);
        return buf;
    }

    // find_unit_idx <objectname>
    //   Linear scan of TAdynmem->UnitDef for a unit whose Objectname matches
    //   (case-insensitive). Returns "OK <idx>" or "OK 0" if not found.
    if (cmd == "find_unit_idx")
    {
        std::string target;
        if (!(iss >> target)) return "ERR bad args";
        TAdynmemStruct* ta = *(TAdynmemStruct**)0x00511de8;
        if (!ta) return "ERR taPtr null";

        unsigned typeID = 0;
        unsigned count = ta->UNITINFOCount;
        if (count > 4096) count = 4096;  // sanity
        for (unsigned i = 1; i < count; ++i)
        {
            if (_stricmp(ta->UnitDef[i].UnitName, target.c_str()) == 0)
            {
                typeID = (unsigned)ta->UnitDef[i].UnitTypeID;
                break;
            }
        }
        char buf[32];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "OK %u", typeID);
        return buf;
    }

    // get_unit_ext_string <idx> <keyName>
    //   Look up the key index by name, then fetch the stored string for that unit.
    if (cmd == "get_unit_ext_string")
    {
        unsigned idx;
        std::string keyName;
        if (!(iss >> idx >> keyName)) return "ERR bad args";
        UnitDefExtensions* ude = UnitDefExtensions::GetInstance();
        if (!ude) return "ERR no ude";
        unsigned keyIdx = ude->getKeyIndex(keyName);
        if (keyIdx == (unsigned)-1) return "ERR unknown key";
        const std::string& val = ude->getString(idx, keyIdx);
        char buf[256];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE,
            "OK val=\"%s\" keyIdx=0x%08X ude=%p",
            val.c_str(), keyIdx, ude);
        return buf;
    }

    // get_rotate_state
    //   Dump CUnitRotate runtime state + compute allowed rotations for current build.
    if (cmd == "get_rotate_state")
    {
        CUnitRotate* ur = CUnitRotate::GetInstance();
        if (!ur) return "ERR no CUnitRotate";
        TAdynmemStruct* ta = *(TAdynmemStruct**)0x00511de8;
        unsigned buildIdx = ta ? (unsigned)ta->BuildUnitID : 0;
        bool n = ur->IsRotationAllowed(buildIdx, 0);
        bool e = ur->IsRotationAllowed(buildIdx, 1);
        bool s = ur->IsRotationAllowed(buildIdx, 2);
        bool w = ur->IsRotationAllowed(buildIdx, 3);
        char buf[128];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE,
            "OK rotation=%d buildIdx=%u allowed=%c%c%c%c",
            ur->GetRotation(), buildIdx,
            n ? 'N' : '-', e ? 'E' : '-', s ? 'S' : '-', w ? 'W' : '-');
        return buf;
    }

    // set_mouse_map_pos <x> <y> [z]
    //   Directly overwrite TAdynmem->MouseMapPos so the build cursor & ghost
    //   preview think the mouse is at (x,y) in world-space. Handy in tests
    //   that drive into the game but can't move the OS mouse.
    //
    //   NOTE: Position_Dword layout is {ushort x_; ushort X; ushort z_; ushort Z;
    //   ushort y_; ushort Y}. Ghidra sees this as {int x; int y; int z} where
    //   X (high word) is the working coordinate. We write BOTH halves — zeroing
    //   the subpixel part — so stale data in the low word doesn't poison the
    //   32-bit fixed-point value that TA's geometry code reads.
    if (cmd == "set_mouse_map_pos")
    {
        int x, y, z = 0;
        if (!(iss >> x >> y)) return "ERR bad args";
        iss >> z;   // optional
        TAdynmemStruct* ta = *(TAdynmemStruct**)0x00511de8;
        if (!ta) return "ERR taPtr null";
        // Layout:
        //   dword @0 = tamem X / Ghidra x  → east-west
        //   dword @4 = tamem Z / Ghidra y  → elevation (height)
        //   dword @8 = tamem Y / Ghidra z  → north-south (ground depth)
        // Match tamem naming since that's what the existing render code reads:
        // x → East-west, y → ground-depth, z → elevation.
        BYTE* mp = reinterpret_cast<BYTE*>(&ta->MouseMapPos);
        *reinterpret_cast<int*>(mp + 0) = (x & 0xFFFF) << 16;   // tamem X
        *reinterpret_cast<int*>(mp + 8) = (y & 0xFFFF) << 16;   // tamem Y
        *reinterpret_cast<int*>(mp + 4) = (z & 0xFFFF) << 16;   // tamem Z (height)
        char buf[64];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "OK (%d,%d,%d)", x, y, z);
        return buf;
    }

    // inject_slash_key
    //   Directly invoke CUnitRotate::Message with VK_OEM_2 (the '/' key).
    //   Bypasses the Windows message path — handy when the game window isn't focused.
    if (cmd == "inject_slash_key")
    {
        CUnitRotate* ur = CUnitRotate::GetInstance();
        if (!ur) return "ERR no CUnitRotate";
        CTAHook* tahook = (CTAHook*)LocalShare->TAHook;
        HWND hwnd = tahook ? tahook->TAhWnd : NULL;
        bool consumed = ur->Message(hwnd, WM_KEYDOWN, VK_OEM_2, 0);
        char buf[32];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "OK consumed=%d", (int)consumed);
        return buf;
    }

    // ta_key_down <vk>
    //   Directly invoke TA's own KeyDownEvent(vkey, repeatFlag=1) at 0x004c1d50.
    //   This is the exact function TA_WindowProc uses to translate WM_KEYDOWN into
    //   an entry in TA's circular input buffer. Bypasses Windows message queue
    //   entirely — no SetForegroundWindow, no PostMessage, no wndproc chain.
    //   The menu / GUI read-back path is unchanged: next frame, the GUI dispatcher
    //   pulls the char via ReadChar_InputBuffer and fires the matching quickkey.
    if (cmd == "ta_key_down")
    {
        std::string arg;
        if (!(iss >> arg)) return "ERR bad args";
        unsigned vk = 0;
        if (arg.size() > 2 && arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X'))
            vk = (unsigned)strtoul(arg.c_str() + 2, nullptr, 16);
        else
            vk = (unsigned)strtoul(arg.c_str(), nullptr, 10);
        typedef unsigned(__cdecl* _KeyDownEvent)(int vkey, int repeat);
        static _KeyDownEvent KeyDownEvent_fn = (_KeyDownEvent)0x004c1d50;
        unsigned rv = KeyDownEvent_fn((int)vk, 1);
        char buf[32];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "OK rv=%u", rv);
        return buf;
    }

    // ta_key_press <char>
    //   Directly invoke TA's KeyPressedEvent(char) at 0x004c1b20. This enqueues
    //   a raw char code (already translated) into TA's circular input buffer.
    //   Use ta_key_down for Win32 VK codes; use ta_key_press when you already
    //   have the lowercase ASCII or special key code (e.g. 0xf2 for pgup).
    if (cmd == "ta_key_press")
    {
        std::string arg;
        if (!(iss >> arg)) return "ERR bad args";
        unsigned ch = 0;
        if (arg.size() > 2 && arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X'))
            ch = (unsigned)strtoul(arg.c_str() + 2, nullptr, 16);
        else
            ch = (unsigned)strtoul(arg.c_str(), nullptr, 10);
        typedef int(__cdecl* _KeyPressedEvent)(unsigned);
        static _KeyPressedEvent KeyPressedEvent_fn = (_KeyPressedEvent)0x004c1b20;
        int rv = KeyPressedEvent_fn(ch);
        char buf[32];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "OK rv=%d", rv);
        return buf;
    }

    // set_build_unit <typeID>
    //   Directly writes ta->BuildUnitID. Used to test rotation logic without
    //   needing to navigate the build menu UI.
    if (cmd == "set_build_unit")
    {
        unsigned typeID;
        if (!(iss >> typeID)) return "ERR bad args";
        TAdynmemStruct* ta = *(TAdynmemStruct**)0x00511de8;
        if (!ta) return "ERR taPtr null";
        ta->BuildUnitID = (int)typeID;
        return "OK";
    }

    // check_rotatable <typeID>
    //   Queries CUnitRotate::IsRotationAllowed for each of the 4 rotations.
    if (cmd == "check_rotatable")
    {
        unsigned typeID;
        if (!(iss >> typeID)) return "ERR bad args";
        CUnitRotate* ur = CUnitRotate::GetInstance();
        if (!ur) return "ERR no CUnitRotate";
        char buf[128];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE,
            "OK N=%d E=%d S=%d W=%d",
            ur->IsRotationAllowed(typeID, 0),
            ur->IsRotationAllowed(typeID, 1),
            ur->IsRotationAllowed(typeID, 2),
            ur->IsRotationAllowed(typeID, 3));
        return buf;
    }

    // list_ude_keys
    //   Dumps every registered UnitDefExtensions key: name, packed index, type.
    //   Helps diagnose key-index collisions or missing registrations.
    if (cmd == "list_ude_keys")
    {
        // We need access to m_keyIndices. Since that's private, we'll expose
        // via a public accessor on the UnitDefExtensions class. For now, use a
        // cheaper path: try the known keys and report their indices.
        UnitDefExtensions* ude = UnitDefExtensions::GetInstance();
        unsigned rotIdx = ude->getKeyIndex("Rotations");
        unsigned vtIdx  = ude->getKeyIndex("VeterancyThresholds");
        unsigned abIdx  = ude->getKeyIndex("VeterancyAccuracyBuffRate");
        char buf[160];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE,
            "OK Rotations=0x%08X VeterancyThresholds=0x%08X VeterancyAccuracyBuffRate=0x%08X",
            rotIdx, vtIdx, abIdx);
        return buf;
    }

    // get_submenu
    //   Returns g_TAMainStruct->SubMenu_Index, Button_Index, MenuFrame_Index,
    //   and TAProgress. Useful for telling which menu screen the game is on
    //   (title=pre-menu before first input, 2=main menu, 7=skirmish, 0x10/11=lobby).
    //
    //   Memory layout (verified from Ghidra disasm of SetSubMenuState_MainMenu at
    //   0x425a90): g_TAMainStruct pointer lives at [0x00511de8] (SAME address as
    //   TAdynmemStruct* used throughout this project — they alias the same object).
    //   SubMenu_Index at TAMainStruct+0x2bbe, Button_Index at +0x2bbf,
    //   MenuFrame_Index at +0x2bc0.
    if (cmd == "get_submenu")
    {
        unsigned char* taMain = *(unsigned char**)0x00511DE8;
        if (!taMain) return "ERR no taMain";
        int sub = taMain[0x2bbe];
        int btn = taMain[0x2bbf];
        int mf  = taMain[0x2bc0];
        char buf[96];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE,
            "OK sub=%d btn=%d mf=%d progress=%d",
            sub, btn, mf, (int)DataShare->TAProgress);
        return buf;
    }

    // mash_s
    //   Fire VK_S via TA's KeyDownEvent N times spaced out over ~2s, then report
    //   the resulting TAProgress. Used for quick diagnostics during development.
    if (cmd == "mash_s")
    {
        typedef unsigned(__cdecl* _KeyDownEvent)(int, int);
        static _KeyDownEvent KeyDownEvent_fn = (_KeyDownEvent)0x004c1d50;
        for (int i = 0; i < 5; ++i)
        {
            KeyDownEvent_fn(0x53 /*VK_S*/, 1);
            Sleep(400);
        }
        char buf[32];
        _snprintf_s(buf, sizeof(buf), _TRUNCATE,
            "OK progress=%d", (int)DataShare->TAProgress);
        return buf;
    }

    // drive_to_game [timeoutSec=45]
    //   Autonomous driver: advance TA from the title screen into a live game.
    //   Spawns a background worker thread that injects S-keys via TA's own
    //   KeyDownEvent, polling TAProgress and giving the game time between
    //   keypresses. Returns immediately with a 'started' marker — poll via
    //   'get_progress' until it returns 'OK 3' (TAInGame).
    //
    //   Running asynchronously avoids blocking the pipe thread for 10-45s
    //   during the menu drive, which was tripping up Python sync-read clients.
    if (cmd == "drive_to_game")
    {
        int timeoutSec = 45;
        iss >> timeoutSec;
        if (timeoutSec < 5) timeoutSec = 5;
        if (timeoutSec > 300) timeoutSec = 300;

        struct DriveArgs { int timeoutSec; };
        DriveArgs* args = new DriveArgs{ timeoutSec };

        // IMPORTANT: do NOT call IDDrawSurface::OutptFmtTxt from this
        // background thread — the log writer is not thread-safe and racing
        // writes from the game's GUI thread corrupt the file handle, which
        // has been observed to kill the pipe thread mid-session.
        HANDLE h = CreateThread(NULL, 0,
            [](LPVOID param) -> DWORD {
                DriveArgs* a = (DriveArgs*)param;
                int timeoutSec = a->timeoutSec;
                delete a;

                typedef unsigned(__cdecl* _KeyDownEvent)(int, int);
                static _KeyDownEvent KeyDownEvent_fn = (_KeyDownEvent)0x004c1d50;

                DWORD deadline = GetTickCount() + (DWORD)(timeoutSec * 1000);
                int lastProgress = DataShare->TAProgress;
                int stalledTicks = 0;

                while (GetTickCount() < deadline)
                {
                    int prog = DataShare->TAProgress;
                    if (prog == TAInGame)
                        return 0;

                    KeyDownEvent_fn(0x53 /*VK_S*/, 1);
                    if (stalledTicks >= 3)
                        KeyDownEvent_fn(0x0D /*VK_RETURN*/, 1);

                    Sleep(600);

                    int prog2 = DataShare->TAProgress;
                    if (prog2 != lastProgress)
                    {
                        lastProgress = prog2;
                        stalledTicks = 0;
                    }
                    else
                    {
                        ++stalledTicks;
                    }
                }
                return 0;
            },
            args, 0, NULL);

        if (!h)
        {
            delete args;
            return "ERR CreateThread failed";
        }
        CloseHandle(h);
        return "OK started";
    }

    return "ERR unknown command";
}

// -----------------------------------------------------------------------
// PipeThreadProc: wait for a client, read line commands, dispatch.
//   Recreates the pipe after each client disconnects.
// -----------------------------------------------------------------------
DWORD WINAPI DebugPipeServer::PipeThreadProc(LPVOID)
{
    while (g_running)
    {
        HANDLE pipe = CreateNamedPipeA(
            "\\\\.\\pipe\\tadr-debug",
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1,      // max instances
            4096,   // out buffer
            4096,   // in buffer
            0,      // default timeout
            NULL);

        if (pipe == INVALID_HANDLE_VALUE)
        {
            Sleep(1000);
            continue;
        }

        // Block until a client connects.
        if (!ConnectNamedPipe(pipe, NULL) && GetLastError() != ERROR_PIPE_CONNECTED)
        {
            CloseHandle(pipe);
            Sleep(100);
            continue;
        }

        IDDrawSurface::OutptFmtTxt("[DebugPipe] client connected");

        char buf[4096];
        std::string acc;

        while (g_running)
        {
            DWORD read = 0;
            BOOL  ok   = ReadFile(pipe, buf, sizeof(buf) - 1, &read, NULL);
            if (!ok || read == 0)
                break;

            buf[read] = '\0';
            acc += buf;

            size_t pos;
            while ((pos = acc.find('\n')) != std::string::npos)
            {
                std::string ln = acc.substr(0, pos);
                acc.erase(0, pos + 1);
                if (!ln.empty() && ln.back() == '\r')
                    ln.pop_back();
                if (ln.empty())
                    continue;

                std::string resp = PostAndWait(ln);
                resp += "\n";
                DWORD written;
                WriteFile(pipe, resp.c_str(), (DWORD)resp.size(), &written, NULL);
            }
        }

        IDDrawSurface::OutptFmtTxt("[DebugPipe] client disconnected");
        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }
    return 0;
}

// -----------------------------------------------------------------------
// Start / Stop
// -----------------------------------------------------------------------
void DebugPipeServer::Start()
{
    InitializeCriticalSection(&g_queueCs);
    g_running = true;
    g_thread  = CreateThread(NULL, 0, PipeThreadProc, NULL, 0, NULL);
    IDDrawSurface::OutptFmtTxt("[DebugPipe] server started on \\\\.\\pipe\\tadr-debug");
}

void DebugPipeServer::Stop()
{
    g_running = false;
    if (g_thread)
    {
        // Unblock ConnectNamedPipe / ReadFile by cancelling pending I/O.
        CancelSynchronousIo(g_thread);
        WaitForSingleObject(g_thread, 2000);
        CloseHandle(g_thread);
        g_thread = NULL;
    }
    DeleteCriticalSection(&g_queueCs);
}

#endif // TADR_DEBUG_PIPE
