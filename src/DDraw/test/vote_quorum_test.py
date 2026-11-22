"""VoteReject quorum regression test -- "whole team disconnected" deadlock.

Reproduces the bug where, if every member of team A drops at once, team B can
never reject any of them:

  1. votesNeeded was derived from *all* active players, including the ones who
     had just dropped, so the 2/3 threshold needed more YES votes than there
     were players left able to click.
  2. teammate consent required a YES from an ally of the target.  A dropped
     player stays in TA's Players[] table until the reject packet lands, so the
     target's dropped allies were found and consent could never be granted.

Timeout votes (mask 6) papered over this via the unconditional 90 s auto-reject
in Tick(); manual votes (mask 1) have no such escape and deadlocked outright.

The fix (CanCastVote/ComputeTally in VoteReject.cpp) excludes players who cannot
cast a vote -- dropped remote humans, and AI slots -- from both the quorum
denominator and the teammate-consent scan.

Scenarios:
  Q1  manual reject of a dropped player carries on the survivors' votes alone
  Q2  timeout reject of a dropped player carries on quorum, not the 90 s timer
  Q3  a *live* ally still blocks the vote until they consent  (no over-fix)
  Q4  the last live ally dropping AFTER all votes are cast still carries the
      vote, with no further vote packet to trigger the check

Verified against an UNFIXED dll: Q1, Q2 and Q4 FAIL -- the vote stays open
forever, logging e.g. "4 yes, 0 no / 9 (need 6), teammate consent: 0" -- while
Q3 PASSES.  Q3 passing on both builds is the point: it is the control that
catches an over-broad fix that would let a still-connected ally be ignored.

Q4 also fails with CanCastVote/ComputeTally in place but Tick()'s once-per-second
re-evaluation removed, so it pins that half of the fix specifically.

  python test/vote_quorum_test.py --yes-destroy-my-game

HOW THIS RUNS -- and why it does NOT enter a game (all of this cost real time
to rediscover, so please leave it alone):

  * Run it at TA's MAIN MENU.  None of OnReceive / ProposeReject /
    CheckAndExecuteReject / ExecuteReject consults TAProgress, and
    VoteReject::Tick() -- which drains the debug-pipe command queue -- runs at
    the menu as well, so the entire vote path is exercisable there.
  * Do NOT drive into a game first.  setup_player fills in only 4 fields of
    PlayerStruct; in-game, TA's per-frame loops dereference the remaining
    garbage and kill the process within ~15 s, which is not even enough for one
    scenario.  At the menu nothing walks the table and TA is stable.
  * Never write slot 0.  Overwriting the real local player's slot kills TA
    instantly once in a game.
  * The DebugPipe server crashes TA shortly after a client disconnects, so a
    second script can never attach to the same TA instance.  One connection,
    one process, then relaunch.
"""
import json
import re
import subprocess
import sys
import time

PIPE = '\\\\.\\pipe\\tadr-debug'

# Slots 0/1 are the real local player and AI slots; leave them alone.  At the
# menu they are inactive, so they contribute nothing to the eligible-voter count.
TEAM_A = [(2, 101, 'alpha1'), (3, 102, 'alpha2'), (4, 103, 'alpha3'), (5, 104, 'alpha4')]
TEAM_B = [(6, 201, 'bravo1'), (7, 202, 'bravo2'), (8, 203, 'bravo3'), (9, 204, 'bravo4')]
TARGET = 101                 # alpha1, the player being voted on
LIVE_ALLY = 102              # alpha2, used by Q3
STALE_GAP = 2000             # > NetworkDropoutTimeoutSec(30) * 30 = 900

B = [d for _, d, _ in TEAM_B]


class Pipe:
    def __init__(self):
        self.f = open(PIPE, 'r+b', buffering=0)

    def cmd(self, line):
        self.f.write((line + '\n').encode())
        r = b''
        while True:
            b = self.f.read(1)
            if not b or b == b'\n':
                break
            if b != b'\r':
                r += b
        return r.decode(errors='replace')

    def votes(self):
        return json.loads(self.cmd('dump_votes'))

    def game_now(self):
        r = self.cmd('get_game_time')
        m = re.match(r'OK (-?\d+)', r)
        if not m:
            raise RuntimeError(f'get_game_time -> {r}')
        return int(m.group(1))


def ta_running():
    out = subprocess.run(['tasklist', '/FI', 'IMAGENAME eq TotalA.exe'],
                         capture_output=True, text=True).stdout
    return 'TotalA.exe' in out


def build_table(p, live_team_a_slots=()):
    """Lay out a 4v4 with team A dropped, except slots in live_team_a_slots."""
    p.cmd('reset_votes')
    for slot, dpid, name in TEAM_A + TEAM_B:
        p.cmd(f'setup_player {slot} {dpid} {name}')

    # Ally each team internally; clear every cross-team pair and every pair with
    # the two real slots, so AreAllies() sees exactly the intended teams.
    for team in (TEAM_A, TEAM_B):
        for i in range(len(team)):
            for j in range(i + 1, len(team)):
                p.cmd(f'set_ally {team[i][0]} {team[j][0]} 1')
    for a, _, _ in TEAM_A:
        for b, _, _ in TEAM_B:
            p.cmd(f'set_ally {a} {b} 0')
    for s, _, _ in TEAM_A + TEAM_B:
        p.cmd(f'set_ally 0 {s} 0')
        p.cmd(f'set_ally 1 {s} 0')

    now = p.game_now()
    # GameTimeSec is the floor in max(LastMsgTimeStamp, GameTimeSec); keep it
    # well below the stale timestamps or the clamp hides the dropout.
    p.cmd(f'set_game_time {now - STALE_GAP - 1000}')
    for slot, _, _ in TEAM_B:
        p.cmd(f'set_last_msg_ts {slot} {now}')
    for slot, _, _ in TEAM_A:
        fresh = slot in live_team_a_slots
        p.cmd(f'set_last_msg_ts {slot} {now if fresh else now - STALE_GAP}')

    v = p.votes()
    if str(TARGET) in v['votes']:
        raise RuntimeError(f"target {TARGET} already has a vote open: {v}")
    return v


def teardown(p):
    p.cmd('reset_votes')
    for slot, _, _ in TEAM_A + TEAM_B:
        p.cmd(f'clear_player {slot}')
    p.cmd('suppress_broadcast 0')


def q1_manual_carries(p):
    """Manual reject of a dropped player: only team B is left able to vote.

    eligible = 4 (team B) -> needs ceil(2/3*4) = 3.  Every ally of the target
    has dropped, so teammate consent is vacuous.  All four team-B votes are cast
    so the scenario does not depend on the exact threshold."""
    before = build_table(p)
    p.cmd(f'inject_propose {B[1]} {TARGET} 1')          # proposer auto-votes YES
    for d in (B[2], B[3], B[0]):
        p.cmd(f'inject_yes {d} {TARGET}')
    time.sleep(0.4)
    v = p.votes()
    # A manual vote that carries leaves no completed-reject and no failure
    # notice; a vote that failed by NO-majority would raise 'notices'.
    passed = str(TARGET) not in v['votes'] and v['notices'] == before['notices']
    return passed, f"open={str(TARGET) in v['votes']} notices={v['notices']}"


def q2_timeout_carries(p):
    """Timeout reject of a dropped player: quorum is 2, consent is vacuous."""
    before = build_table(p)
    p.cmd(f'inject_propose {B[1]} {TARGET} 6')          # mask 6: no auto-vote
    for d in (B[2], B[3]):
        p.cmd(f'inject_yes {d} {TARGET}')
    time.sleep(0.4)
    v = p.votes()
    passed = (str(TARGET) not in v['votes']
              and v['completedRejects'] == before['completedRejects'] + 1)
    return passed, (f"open={str(TARGET) in v['votes']} "
                    f"completed={v['completedRejects']} (was {before['completedRejects']})")


def q3_live_ally_blocks(p):
    """A team-A player who is still talking must still be able to block.

    eligible = 5 (live ally + team B) -> needs 4.  Four team-B YES votes meet the
    threshold but must NOT carry, because the target's live ally has not
    consented.  The ally's YES then releases it."""
    before = build_table(p, live_team_a_slots=(3,))     # alpha2 (102) is alive
    p.cmd(f'inject_propose {B[1]} {TARGET} 1')
    for d in (B[2], B[3], B[0]):
        p.cmd(f'inject_yes {d} {TARGET}')
    time.sleep(0.4)
    v = p.votes()
    blocked = str(TARGET) in v['votes'] and v['votes'][str(TARGET)]['yes'] == 4

    p.cmd(f'inject_yes {LIVE_ALLY} {TARGET}')           # the live ally consents
    time.sleep(0.4)
    v2 = p.votes()
    released = str(TARGET) not in v2['votes'] and v2['notices'] == before['notices']
    return blocked and released, f"blocked={blocked} released={released}"


def q4_ally_drops_after_voting(p):
    """The target's last live ally drops AFTER every other vote is already in.

    No further vote packet arrives, so only Tick()'s once-per-second
    re-evaluation can notice that votesNeeded dropped and consent went vacuous.
    Without that pass the ballot sits open until it expires -- and a manual vote
    expires as a FAILURE, which is the deadlock all over again."""
    before = build_table(p, live_team_a_slots=(3,))     # alpha2 (102) is alive
    p.cmd(f'inject_propose {B[1]} {TARGET} 1')
    for d in (B[2], B[3], B[0]):
        p.cmd(f'inject_yes {d} {TARGET}')
    time.sleep(0.4)
    blocked = str(TARGET) in p.votes()['votes']         # held up by the live ally

    # The ally now drops.  Deliberately cast NO further votes.
    now = p.game_now()
    p.cmd(f'set_game_time {now - STALE_GAP - 1000}')
    p.cmd(f'set_last_msg_ts 3 {now - STALE_GAP}')
    time.sleep(2.5)                                     # >1 s: Tick() re-evaluates
    v2 = p.votes()
    carried = str(TARGET) not in v2['votes'] and v2['notices'] == before['notices']
    return blocked and carried, f"blocked={blocked} carried_after_ally_drop={carried}"


def main():
    if '--yes-destroy-my-game' not in sys.argv:
        print(__doc__)
        print("Refusing to run: this test overwrites player slots 2-9.")
        sys.exit(2)
    if not ta_running():
        print("FAIL: TotalA.exe not running")
        sys.exit(2)

    p = Pipe()
    p.cmd('suppress_broadcast 1')            # nothing reaches the wire
    try:
        results = [
            ("Q1 manual carries w/o allies ", q1_manual_carries(p)),
            ("Q2 timeout carries on quorum ", q2_timeout_carries(p)),
            ("Q3 live ally still blocks    ", q3_live_ally_blocks(p)),
            ("Q4 ally drops after voting   ", q4_ally_drops_after_voting(p)),
        ]
    finally:
        teardown(p)

    allok = all(ok for _, (ok, _) in results)
    for name, (ok, detail) in results:
        print(f"  [{'PASS' if ok else 'FAIL'}] {name} -- {detail}")
    print(f"\n{'ALL PASS' if allok else 'SOME FAILED'} "
          f"({sum(ok for _, (ok, _) in results)}/{len(results)})")
    sys.exit(0 if allok else 1)


if __name__ == '__main__':
    main()
