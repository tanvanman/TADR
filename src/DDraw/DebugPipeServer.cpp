#include "DebugPipeServer.h"

#ifdef TADR_DEBUG_PIPE

#include "VoteReject.h"
#include "iddrawsurface.h"
#include "tamem.h"

#include <windows.h>
#include <string>
#include <sstream>
#include <vector>

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
// PostAndWait: enqueue a command from the pipe thread and block until
//   the render thread executes it (timeout 5 s).
// -----------------------------------------------------------------------
static std::string PostAndWait(const std::string& line)
{
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
