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
// Automation / inspection commands for autonomous test iteration:
//   press_key <vk>             post WM_KEYDOWN+WM_KEYUP to TA's wndproc
//   send_char <ascii>          post WM_CHAR for a single ASCII character
//   get_progress               echo DataShare->TAProgress (1..4)
//   find_unit_idx <objectname> scan UnitDef array for Objectname match
//   get_unit_ext_string <idx> <keyName>  query UnitDefExtensions by key name
//   get_rotate_state           dump CUnitRotate rotation + allowed mask
//   inject_slash_key           invoke CUnitRotate::Message(VK_OEM_2)
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

    // Execute a single command line. Used by PostAndWait to run read-only
    // commands directly on the pipe thread when the render-thread drain
    // isn't reliably firing (e.g. in the title screen).
    static std::string ExecuteCommand(const std::string& line);

private:
    static DWORD WINAPI PipeThreadProc(LPVOID param);
};

#endif // TADR_DEBUG_PIPE
