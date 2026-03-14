r"""
VoteReject / VoteDialog integration test suite.

Prerequisites:
  - TA is running with the tadr-ddraw DLL injected (Release build with DEBUG_INFO).
  - TA is in a multiplayer skirmish (TAProgress == TAInGame), OR let the
    script call set_progress 3 to fake it (works for logic tests but
    VoteDialog won't render without a real in-game frame loop).
  - suppress_broadcast is sent at the start so no real DirectPlay packets
    are emitted.

Usage:
  python vote_test.py [--pipe \\.\pipe\tadr-debug] [--test <name>]

All tests are run by default.  Pass --test <name> to run a single test.
"""

import ctypes
import ctypes.wintypes as wt
import json
import sys
import time
import argparse

# ---------------------------------------------------------------------------
# Pipe client (ctypes, no extra deps)
# ---------------------------------------------------------------------------

k32 = ctypes.windll.kernel32

GENERIC_READ  = 0x80000000
GENERIC_WRITE = 0x40000000
OPEN_EXISTING = 3
INVALID_HANDLE_VALUE = ctypes.c_void_p(-1).value


def pipe_open(name: str, retries: int = 20):
    """Open the named pipe, retrying until TA has created it."""
    for attempt in range(retries):
        h = k32.CreateFileW(
            name,
            GENERIC_READ | GENERIC_WRITE,
            0, None,
            OPEN_EXISTING,
            0, None)
        if h != INVALID_HANDLE_VALUE:
            return h
        err = k32.GetLastError()
        if err == 2:   # ERROR_FILE_NOT_FOUND — pipe not yet created
            if attempt == 0:
                print(f"  waiting for pipe {name} ...")
            time.sleep(0.5)
        elif err == 231:  # ERROR_PIPE_BUSY
            k32.WaitNamedPipeW(name, 2000)
        else:
            raise IOError(f"CreateFile failed: error {err}")
    raise IOError(f"Could not open pipe {name} after {retries} attempts")


def pipe_send(h, cmd: str) -> str:
    """Send one command line and return the response line."""
    data = (cmd.strip() + '\n').encode('ascii')
    written = wt.DWORD(0)
    ok = k32.WriteFile(h, data, len(data), ctypes.byref(written), None)
    if not ok:
        raise IOError(f"WriteFile failed: {k32.GetLastError()}")

    # Read response byte-by-byte until \n
    buf   = ctypes.create_string_buffer(1)
    nread = wt.DWORD(0)
    resp  = b''
    while True:
        ok = k32.ReadFile(h, buf, 1, ctypes.byref(nread), None)
        if not ok or nread.value == 0:
            break
        resp += buf.raw[:1]
        if resp.endswith(b'\n'):
            break
    return resp.decode('ascii').rstrip('\r\n')


def pipe_close(h):
    k32.CloseHandle(h)


# ---------------------------------------------------------------------------
# Test harness
# ---------------------------------------------------------------------------

_pipe = None

def cmd(line: str) -> str:
    resp = pipe_send(_pipe, line)
    return resp


def ok(line: str) -> str:
    """Send command, assert OK."""
    resp = cmd(line)
    assert resp == 'OK', f"Expected OK from '{line}', got: {resp}"
    return resp


def dump() -> dict:
    """Return parsed dump_votes JSON."""
    resp = cmd('dump_votes')
    return json.loads(resp)


def votes(d: dict) -> dict:
    return d.get('votes', {})


PASS = '\033[92mPASS\033[0m'
FAIL = '\033[91mFAIL\033[0m'


def run_test(name: str, fn) -> bool:
    try:
        ok('reset_votes')
        fn()
        print(f"  {PASS}  {name}")
        return True
    except AssertionError as e:
        print(f"  {FAIL}  {name}: {e}")
        return False
    except Exception as e:
        print(f"  {FAIL}  {name}: {type(e).__name__}: {e}")
        return False


# ---------------------------------------------------------------------------
# Setup helpers
# ---------------------------------------------------------------------------

# Dpids used throughout tests:
# slot 0 = Alice  dpid 100 (local player)
# slot 1 = Bob    dpid 200
# slot 2 = Carol  dpid 300
# slot 3 = Dave   dpid 400  (used in multi-vote tests)

def setup_4player():
    ok('setup_player 0 100 Alice')
    ok('setup_player 1 200 Bob')
    ok('setup_player 2 300 Carol')
    ok('setup_player 3 400 Dave')
    ok('set_local 0')
    ok('set_progress 3')
    ok('suppress_broadcast 1')


def setup_3player():
    ok('setup_player 0 100 Alice')
    ok('setup_player 1 200 Bob')
    ok('setup_player 2 300 Carol')
    ok('clear_player 3')
    ok('set_local 0')
    ok('set_progress 3')
    ok('suppress_broadcast 1')


def setup_2player():
    ok('setup_player 0 100 Alice')
    ok('setup_player 1 200 Bob')
    ok('clear_player 2')
    ok('clear_player 3')
    ok('set_local 0')
    ok('set_progress 3')
    ok('suppress_broadcast 1')


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

def test_manual_vote_passes():
    """3-player: A proposes vs C, B votes YES -> passes (2/2 non-target voted)."""
    setup_3player()
    # Bob proposes to reject Carol
    ok('inject_propose 200 300 1')
    d = dump()
    assert '300' in votes(d), "vote for Carol not found"
    v = votes(d)['300']
    assert v['yes'] == 1, f"expected 1 yes (Bob implicit), got {v['yes']}"
    assert v['mask'] == 1

    # Alice votes YES (local player button)
    ok('local_yes 300')
    d = dump()
    # 2 yes votes from 2 non-target players -> should pass -> vote erased
    assert '300' not in votes(d), "vote should be resolved after 2 YES"


def test_manual_vote_no_blocks():
    """3-player: A proposes vs C, B votes NO making YES threshold unreachable."""
    setup_3player()
    ok('inject_propose 200 300 1')  # Bob proposes Carol
    ok('inject_no 100 300')         # Alice votes NO (need 2/2 YES, now impossible)
    d = dump()
    # Vote should be erased (manual vote cancelled by NO majority)
    assert '300' not in votes(d), "vote should be cancelled by NO majority"
    # A transient notice should have been added
    assert d['notices'] >= 1, "expected a failure transient notice"


def test_proposer_no_cancels_implicit_yes():
    """Proposer voting NO should remove their implicit YES from the vote.
    Uses 4 players so that a single NO vote doesn't immediately hit the
    NO-majority threshold (need >1 NO to cancel in a 4-player game)."""
    setup_4player()
    ok('inject_propose 200 400 1')  # Bob proposes Dave (Bob has implicit YES)
    d = dump()
    assert votes(d)['400']['yes'] == 1, f"expected 1 yes; got {d}"

    ok('inject_no 200 400')  # Bob votes NO — should cancel his YES
    d = dump()
    assert '400' in votes(d), f"vote should still exist after 1 NO in 4-player game; got {d}"
    v = votes(d)['400']
    assert v['yes'] == 0, f"Bob's YES should be cancelled; yes={v['yes']}"
    assert v['no']  == 1, f"Bob's NO should be recorded; no={v['no']}"


def test_local_no_cancels_yes():
    """Local player clicking NO should cancel their prior YES.
    Uses 4 players so that a single NO vote doesn't immediately hit the
    NO-majority threshold (need >1 NO to cancel in a 4-player game)."""
    setup_4player()
    # Alice proposes to reject Dave (Alice gets implicit YES)
    ok('inject_propose 100 400 1')
    d = dump()
    assert votes(d)['400']['yes'] == 1, f"expected 1 yes; got {d}"

    ok('local_no 400')  # Alice clicks NO
    d = dump()
    assert '400' in votes(d), f"vote should still exist after 1 NO in 4-player game; got {d}"
    v = votes(d)['400']
    assert v['yes'] == 0, f"Alice's YES should be cancelled; yes={v['yes']}"
    assert v['no']  == 1


def test_manual_vote_timeout_60s():
    """Manual vote that expires should not auto-reject, just add notice."""
    setup_3player()
    ok('inject_propose 200 300 1')  # Bob proposes Carol
    d = dump()
    assert '300' in votes(d)

    ok('expire_vote 300')  # set expiry to now-1

    # Tick runs on the next render frame; wait 200ms to be safe.
    # (The test environment needs a live TA for Tick to run automatically.
    #  If Tick hasn't run yet, pump the pipe one more time to force DrainQueue
    #  from the next Tick.)
    time.sleep(0.2)
    # Send a no-op command to give render thread a chance to process
    ok('suppress_broadcast 1')
    time.sleep(0.2)

    d = dump()
    # Vote should be expired and removed (no auto-reject for manual votes)
    assert '300' not in votes(d), "manual vote should expire without auto-reject"
    assert d['completedRejects'] == 0, "manual vote timeout should not create a completed reject"
    assert d['notices'] >= 1, "expected timeout transient notice"


def test_timeout_vote_zero_initial_yes():
    """Timeout vote (mask=6) should start with 0 YES votes."""
    setup_3player()
    ok('inject_propose 200 300 6')  # Bob proposes timeout reject of Carol
    d = dump()
    assert '300' in votes(d)
    v = votes(d)['300']
    assert v['yes']  == 0, f"timeout vote should start with 0 yes; got {v['yes']}"
    assert v['no']   == 0
    assert v['mask'] == 6


def test_timeout_vote_needs_explicit_yes():
    """Timeout vote only passes once someone explicitly YES votes."""
    setup_2player()
    # Alice proposes timeout reject of Bob
    ok('inject_propose 100 200 6')
    d = dump()
    # With 2 players: local player is Alice, vote is on Bob -> Alice sees the vote
    # votesNeeded = 1 (only 1 non-target). 0 votes so far -> should NOT auto-resolve.
    assert '200' in votes(d), "timeout vote should exist"
    assert votes(d)['200']['yes'] == 0

    # Now Alice clicks YES
    ok('local_yes 200')
    d = dump()
    # 1 yes, 1 needed -> vote passes
    assert '200' not in votes(d), "timeout vote should pass after 1 YES"
    assert d['completedRejects'] == 1, "should have a completed reject entry"


def test_multiple_timeout_votes():
    """Multiple simultaneous timeout votes should all appear in the dump."""
    setup_4player()
    ok('inject_propose 100 200 6')  # Alice proposes timeout reject of Bob
    ok('inject_propose 100 300 6')  # Alice proposes timeout reject of Carol
    d = dump()
    assert '200' in votes(d), "Bob vote missing"
    assert '300' in votes(d), "Carol vote missing"
    assert len(votes(d)) == 2


def test_timeout_vote_no_majority_closes_dialog_row():
    """After NO majority on timeout vote, voting is closed but auto-reject still pending.
       The vote should be votingClosed=true and excluded from GetActiveVotes (dialog row gone)."""
    setup_3player()
    ok('inject_propose 100 300 6')  # Alice proposes timeout reject of Carol

    # In 3-player game: 2 non-target. votesNeeded=2 for timeout.
    # To block: noVoteCount > nonTarget - votesNeeded = 2 - 2 = 0, so any NO suffices.
    ok('inject_no 200 300')  # Bob votes NO
    d = dump()

    # Vote still exists in m_votes (auto-reject pending) but votingClosed=true
    assert '300' in votes(d), "vote should still exist for auto-reject"
    assert votes(d)['300']['closed'] == True, "votingClosed should be true"


def test_stored_player_name_used_after_reject():
    """Completed reject entry should use stored targetName, not live GetPlayerName."""
    setup_2player()
    ok('inject_propose 100 200 6')  # timeout reject of Bob
    ok('local_yes 200')             # Alice YES -> vote passes (2-player, need 1)
    d = dump()
    # Bob is now 'rejected' so the completed reject entry should exist
    # (We can't easily check the name from Python, but we can verify the entry exists)
    assert '200' not in votes(d), "vote should be resolved"
    assert d['completedRejects'] == 1, "completed reject entry should exist"


def test_simultaneous_dropout_all_proposed():
    """Three players drop simultaneously: the dropout scan proposes a timeout reject for each.
    Uses force_dropout_check to run the scan regardless of TAProgress (main-menu state).
    Sets LastMsgTimeStamp = gameNow - 901 to exceed the 900-unit gap threshold."""
    setup_4player()
    resp = cmd('get_game_time')
    assert resp.startswith('OK '), f"get_game_time failed: {resp}"
    game_now = int(resp.split()[1])
    stale_ts = game_now - 901   # gap = 901 > 900 threshold
    ok(f'set_last_msg_ts 1 {stale_ts}')   # Bob
    ok(f'set_last_msg_ts 2 {stale_ts}')   # Carol
    ok(f'set_last_msg_ts 3 {stale_ts}')   # Dave
    # force_dropout_check runs the scan unconditionally (same logic as Tick, ignores TAProgress)
    resp = cmd('force_dropout_check')
    assert resp.startswith('OK '), f"force_dropout_check failed: {resp}"
    found = int(resp.split()[1])
    assert found == 3, f"expected 3 timed-out players detected, got {found}"
    d = dump()
    assert '200' in votes(d), f"Bob should have a timeout vote; got {d}"
    assert '300' in votes(d), f"Carol should have a timeout vote; got {d}"
    assert '400' in votes(d), f"Dave should have a timeout vote; got {d}"
    for dpid in ('200', '300', '400'):
        assert votes(d)[dpid]['mask'] == 6, f"dpid={dpid} should be timeout (mask=6)"


def test_timeout_vote_ally_must_consent():
    """Timeout vote should not pass until an ally of the target has voted YES."""
    # 3-player game: Alice(0/100) and Dave(3/400) are allies; Bob(1/200) is FFA.
    # Vote to timeout-reject Dave. votesNeeded=2 (3 non-target... wait)
    # Actually: 4 players total, target=Dave, nonTarget=3, votesNeeded=2.
    # teammateConsent requires Alice (ally of Dave) to have voted.
    setup_4player()
    ok('set_ally 0 3 1')   # Alice and Dave are allies
    # Alice proposes timeout reject of Dave (Alice has NO implicit YES for timeout votes)
    ok('inject_propose 100 400 6')
    # Bob votes YES (1 yes) — not an ally, consent not satisfied
    ok('inject_yes 200 400')
    d = dump()
    assert '400' in votes(d), f"vote should not pass without ally consent; got {d}"
    v = votes(d)['400']
    assert v['yes'] == 1, f"should have 1 yes; got {v}"

    # Carol votes YES (2 yes) — still not an ally of Dave, consent still not satisfied
    ok('inject_yes 300 400')
    d = dump()
    assert '400' in votes(d), f"vote should not pass without ally (Alice) voting; got {d}"

    # Alice (ally of Dave) votes YES — consent now satisfied, vote passes (2 needed, have 3)
    ok('inject_yes 100 400')
    d = dump()
    assert '400' not in votes(d), f"vote should pass once ally votes; got {d}"
    assert d['completedRejects'] == 1


def test_timeout_vote_ffa_needs_two_yes():
    """Timeout vote in FFA (no allies): needs 2 YES in a 4-player game; 1 is not enough."""
    setup_4player()
    ok('inject_propose 100 400 6')   # Alice proposes timeout reject of Dave
    ok('inject_yes 200 400')         # Bob votes YES -> 1 yes, need 2
    d = dump()
    assert '400' in votes(d), f"1 yes should not pass (need 2); got {d}"

    ok('inject_yes 300 400')         # Carol votes YES -> 2 yes -> passes (FFA, no ally constraint)
    d = dump()
    assert '400' not in votes(d), f"2 yes should pass in FFA; got {d}"
    assert d['completedRejects'] == 1


def test_change_no_to_yes_remote():
    """Remote player who voted NO can change to YES (old NO should be cancelled)."""
    setup_4player()
    ok('inject_propose 200 400 1')   # Bob proposes Dave (Bob: 1 YES implicit)
    ok('inject_no 100 400')          # Alice votes NO
    d = dump()
    v = votes(d)['400']
    assert v['yes'] == 1 and v['no'] == 1, f"setup: expected 1y/1n, got {v}"

    ok('inject_yes 100 400')         # Alice changes to YES
    d = dump()
    # Vote should now pass: Alice YES + Bob YES = 2/3 needed in 4-player game (need 2)
    assert '400' not in votes(d), f"vote should pass after Alice changes NO->YES; got {d}"


def test_change_no_to_yes_local():
    """Local player who voted NO can change to YES via local_yes."""
    setup_4player()
    ok('inject_propose 200 400 1')   # Bob proposes Dave (Bob: 1 YES)
    ok('local_no 400')               # Alice (local) votes NO
    d = dump()
    v = votes(d)['400']
    assert v['yes'] == 1 and v['no'] == 1, f"setup: expected 1y/1n, got {v}"

    ok('local_yes 400')              # Alice changes to YES -> should pass
    d = dump()
    assert '400' not in votes(d), f"vote should pass after Alice changes NO->YES; got {d}"


def test_change_yes_to_no_remote():
    """Remote player who voted YES can change to NO (old YES should be cancelled).
    Uses 5 players: votesNeeded = ceil(2/3 * 4) = 3, so 2 YES won't pass the vote."""
    ok('setup_player 0 100 Alice')
    ok('setup_player 1 200 Bob')
    ok('setup_player 2 300 Carol')
    ok('setup_player 3 400 Dave')
    ok('setup_player 4 500 Eve')
    ok('set_local 0')
    ok('set_progress 3')
    ok('suppress_broadcast 1')

    ok('inject_propose 200 500 1')   # Bob proposes Eve (Bob: 1 YES implicit)
    ok('inject_yes 100 500')         # Alice votes YES -> 2 YES, need 3, not yet passing
    d = dump()
    v = votes(d)['500']
    assert v['yes'] == 2 and v['no'] == 0, f"setup: expected 2y/0n, got {v}"

    ok('inject_no 100 500')          # Alice changes to NO -> 1 YES (Bob), 1 NO (Alice)
    d = dump()
    # nonTarget=4, votesNeeded=3, noVoteCount=1 > nonTarget-votesNeeded=1? 1>1 is False -> vote stays open
    assert '500' in votes(d), f"vote should still be open; got {d}"
    v = votes(d)['500']
    assert v['yes'] == 1, f"Alice's YES should be cancelled; yes={v['yes']}"
    assert v['no']  == 1, f"Alice's NO should be recorded; no={v['no']}"


def test_reset_clears_everything():
    """reset_votes should clear votes, notices, and completed rejects."""
    setup_3player()
    ok('inject_propose 200 300 1')
    ok('inject_propose 200 100 6')  # also a timeout vote
    d = dump()
    assert len(votes(d)) > 0

    ok('reset_votes')
    d = dump()
    assert len(votes(d)) == 0,         "votes should be cleared"
    assert d['notices'] == 0,          "notices should be cleared"
    assert d['completedRejects'] == 0, "completed rejects should be cleared"


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

ALL_TESTS = [
    ('manual_vote_passes',                  test_manual_vote_passes),
    ('manual_vote_no_blocks',               test_manual_vote_no_blocks),
    ('proposer_no_cancels_implicit_yes',    test_proposer_no_cancels_implicit_yes),
    ('local_no_cancels_yes',                test_local_no_cancels_yes),
    ('manual_vote_timeout_60s',             test_manual_vote_timeout_60s),
    ('timeout_vote_zero_initial_yes',       test_timeout_vote_zero_initial_yes),
    ('timeout_vote_needs_explicit_yes',     test_timeout_vote_needs_explicit_yes),
    ('multiple_timeout_votes',              test_multiple_timeout_votes),
    ('timeout_vote_no_majority_closes_row', test_timeout_vote_no_majority_closes_dialog_row),
    ('stored_player_name_after_reject',     test_stored_player_name_used_after_reject),
    ('reset_clears_everything',             test_reset_clears_everything),
    ('change_no_to_yes_remote',             test_change_no_to_yes_remote),
    ('change_no_to_yes_local',              test_change_no_to_yes_local),
    ('change_yes_to_no_remote',             test_change_yes_to_no_remote),
    ('timeout_vote_ffa_needs_two_yes',      test_timeout_vote_ffa_needs_two_yes),
    ('simultaneous_dropout_all_proposed',   test_simultaneous_dropout_all_proposed),
    ('timeout_vote_ally_must_consent',      test_timeout_vote_ally_must_consent),
]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--pipe', default=r'\\.\pipe\tadr-debug')
    parser.add_argument('--test', default=None, help='Run a single named test')
    args = parser.parse_args()

    global _pipe
    print(f"Connecting to {args.pipe} ...")
    _pipe = pipe_open(args.pipe)
    print("Connected.\n")

    tests = ALL_TESTS
    if args.test:
        tests = [(n, f) for n, f in ALL_TESTS if n == args.test]
        if not tests:
            print(f"Unknown test: {args.test}")
            print("Available:", ', '.join(n for n, _ in ALL_TESTS))
            sys.exit(1)

    passed = failed = 0
    for name, fn in tests:
        if run_test(name, fn):
            passed += 1
        else:
            failed += 1

    print(f"\n{passed}/{passed+failed} tests passed")
    pipe_close(_pipe)
    sys.exit(0 if failed == 0 else 1)


if __name__ == '__main__':
    main()
