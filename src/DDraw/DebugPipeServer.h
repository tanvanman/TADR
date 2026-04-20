#pragma once

#ifdef TADR_DEBUG_PIPE

#include <windows.h>
#include <string>

// DebugPipeServer: named-pipe debug interface for automated testing.
//
// A background thread listens on \\.\pipe\tadr-debug.  Each connected
// client can send line-delimited commands; responses are sent back as
// single lines.  Commands are executed on the render thread (via
// DrainQueue, called from VoteReject::Tick) so they never race with
// game state.
//
// Commands (see DebugPipeServer.cpp for full list):
//   setup_player <slot> <dpid> <name>
//   clear_player <slot>
//   set_local <slot>
//   set_progress <n>           (3 = TAInGame)
//   suppress_broadcast <0|1>   (suppress VoteReject->BroadcastMsg)
//   inject_propose <fromDpid> <targetDpid> <rejectMask>
//   inject_yes     <fromDpid> <targetDpid>
//   inject_no      <fromDpid> <targetDpid>
//   local_yes      <targetDpid>
//   local_no       <targetDpid>
//   expire_vote    <targetDpid>
//   reset_votes
//   dump_votes                 (returns single-line JSON)
//
// Responses: "OK [data]\n" or "ERR message\n".

class DebugPipeServer
{
public:
    static void Start();
    static void Stop();

    // Called from VoteReject::Tick() on the render thread to drain the
    // command queue and execute pending commands.
    static void DrainQueue();

private:
    static DWORD WINAPI PipeThreadProc(LPVOID param);
    static std::string ExecuteCommand(const std::string& line);
};

#endif // TADR_DEBUG_PIPE
