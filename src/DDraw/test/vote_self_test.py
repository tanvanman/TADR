"""VoteReject self-target regression test (game 175237 host-self-eject bug).

Validates, against a LIVE TA via the tadr-debug pipe, that:
  A. A *timeout* reject vote (mask=6) targeting the LOCAL player is IGNORED
     (never opened, never self-executes).  <-- the fix
  B. A timeout reject vote targeting a still-gone player still EXECUTES on
     expiry (feature preserved).
  C. A timeout reject vote targeting a player whose traffic resumes is
     CANCELLED, not executed (recovery preserved).

MP-safe: uses NO phantom players and never writes player slots. It discovers
the real local (type=Player_LocalHuman=1) and remote (type=Player_RemoteHuman=3)
dplayIds, keeps suppress_broadcast on the whole time (nothing reaches the wire),
and restores suppress_broadcast + clears votes on exit. Designed to run against
a real 2-player multiplayer game without disturbing it.

  python test/vote_self_test.py --manual    # attach to a game you already started
"""
import json
import re
import subprocess
import sys
import time

PIPE = '\\\\.\\pipe\\tadr-debug'
GHOST_DPID = 999999          # matches no player -> targetSlot stays -1


class Pipe:
    def __init__(self):
        self._open()

    def _open(self):
        self.f = open(PIPE, 'r+b', buffering=0)

    def cmd(self, line):
        for attempt in range(2):
            try:
                self.f.write((line + '\n').encode())
                r = b''
                while True:
                    b = self.f.read(1)
                    if not b or b == b'\n':
                        break
                    if b != b'\r':
                        r += b
                return r.decode(errors='replace')
            except OSError:
                if attempt == 0:
                    time.sleep(0.3); self._open()
                else:
                    raise
        return ''

    def votes(self):
        return json.loads(self.cmd('dump_votes'))


def ta_running():
    out = subprocess.run(['tasklist', '/FI', 'IMAGENAME eq TotalA.exe'],
                         capture_output=True, text=True).stdout
    return 'TotalA.exe' in out


def discover_players(p):
    """Return (local_dpid, remote_dpid) by scanning slots for type 1 / type 3."""
    local = remote = None
    for slot in range(10):
        r = p.cmd(f'dump_player_state {slot}')
        m = re.search(r'active=(\d+) dpid=(\d+) type=(\d+)', r)
        if not m:
            continue
        active, dpid, typ = int(m.group(1)), int(m.group(2)), int(m.group(3))
        if not active or dpid == 0:
            continue
        if typ == 1 and local is None:        # Player_LocalHuman
            local = dpid
        elif typ == 3 and remote is None:     # Player_RemoteHuman
            remote = dpid
    return local, remote


def scenario_self(p, self_dpid, proposer):
    """A: a peer proposes a timeout-reject of the LOCAL player -> must be ignored."""
    p.cmd('reset_votes')
    p.cmd(f'inject_propose {proposer} {self_dpid} 6')
    time.sleep(0.3)
    opened = str(self_dpid) in p.votes()['votes']
    p.cmd(f'expire_vote {self_dpid}')
    time.sleep(0.4)
    v2 = p.votes()
    executed = v2['completedRejects'] > 0
    ok = (not opened) and (not executed) and (str(self_dpid) not in v2['votes'])
    return ok, f"opened={opened} executed={executed} completed={v2['completedRejects']}"


def scenario_gone_executes(p, proposer):
    """B: timeout-reject of a player that stays gone (no live slot) -> must execute."""
    p.cmd('reset_votes')
    p.cmd(f'inject_propose {proposer} {GHOST_DPID} 6')
    time.sleep(0.3)
    opened = str(GHOST_DPID) in p.votes()['votes']
    p.cmd(f'expire_vote {GHOST_DPID}')
    time.sleep(0.5)
    v2 = p.votes()
    executed = v2['completedRejects'] > 0 and str(GHOST_DPID) not in v2['votes']
    return opened and executed, f"opened={opened} executed={executed} completed={v2['completedRejects']}"


def scenario_resumed_cancels(p, remote_dpid, proposer):
    """C: timeout-reject of the LIVE remote peer -> their packets resume -> cancel.

    SAFE: never writes a player slot (that crashes a live game). A live peer
    cancels within ~1 render tick, so 'opened' is caught best-effort by fast
    polling; the PASS criterion is the outcome (not executed, and removed)."""
    p.cmd('reset_votes')
    p.cmd(f'inject_propose {proposer} {remote_dpid} 6')
    opened = False
    for _ in range(50):                                   # ~1s, catch the open window
        if str(remote_dpid) in p.votes()['votes']:
            opened = True
            break
    time.sleep(0.5)                                       # let the cancel settle
    v2 = p.votes()
    cancelled = str(remote_dpid) not in v2['votes'] and v2['completedRejects'] == 0
    return cancelled, f"opened={opened} cancelled={cancelled} completed={v2['completedRejects']}"


def main():
    if '--manual' not in sys.argv:
        print("Refusing to auto-launch against a live game. Run with --manual after "
              "you are in a game.")
        sys.exit(2)
    if not ta_running():
        print("FAIL: TotalA.exe not running")
        sys.exit(2)

    p = Pipe()
    if p.cmd('get_progress') != 'OK 3':
        print(f"FAIL: not in game (get_progress={p.cmd('get_progress')})")
        sys.exit(2)

    local, remote = discover_players(p)
    print(f"  discovered local_dpid={local} remote_dpid={remote}")
    if not local or not remote:
        print("FAIL: could not identify both a local (type1) and remote (type3) player")
        sys.exit(2)

    p.cmd('suppress_broadcast 1')              # belt: nothing reaches the wire
    try:
        results = [
            ("A self-target IGNORED ", scenario_self(p, local, remote)),
            ("B gone player EXECUTES", scenario_gone_executes(p, remote)),
            ("C resumed peer CANCELS", scenario_resumed_cancels(p, remote, local)),
        ]
    finally:
        p.cmd('reset_votes')
        p.cmd('suppress_broadcast 0')          # restore normal VoteReject for the game

    allok = all(ok for _, (ok, _) in results)
    for name, (ok, detail) in results:
        print(f"  [{'PASS' if ok else 'FAIL'}] {name} -- {detail}")
    print(f"\n{'ALL PASS' if allok else 'SOME FAILED'} "
          f"({sum(ok for _,(ok,_) in results)}/{len(results)})")
    sys.exit(0 if allok else 1)


if __name__ == '__main__':
    main()
