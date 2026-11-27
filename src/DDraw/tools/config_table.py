#!/usr/bin/env python3
"""Generate the CONFIGURATIONS table in tdraw.txt from config.h / config_*.h.

    python config_table.py            print the table
    python config_table.py --check    fail if tdraw.txt does not match, or if a switch that differs
                                      between mods has no row
    python config_table.py --write    update tdraw.txt
    python config_table.py --markdown print the GitHub release-notes version

Rows are declared in config_table.json next to this script. The headers are evaluated with a small
preprocessor (no compiler needed), so defaults set in config.h count. Nothing in the headers is
executed: expressions are limited to integers and operators.
"""
import json
import re
import sys
from pathlib import Path

MODS = ["prota", "escalation", "mayhem", "ota", "tazero", "bta", "twilight"]
HEADS = ["prota", "esc", "mayhem", "ota", "tazero", "bta", "twilight"]
HEADER_LINE = re.compile(r"^\s+" + r"\s+".join(HEADS) + r"\s*$")


class Preprocessor:
    def __init__(self, root, predefined):
        self.root = Path(root)
        self.defs = dict(predefined)
        self.depth = 0

    def run(self, name):
        self.depth += 1
        if self.depth > 8:
            raise ValueError("includes nested too deeply")
        text = (self.root / name).read_text(encoding="utf-8", errors="replace").replace("\r", "")
        text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
        text = re.sub(r"\\\n", " ", text)
        stack = []                              # [active, taken, parent_active]
        for line in text.split("\n"):
            m = re.match(r"\s*#\s*(\w+)\s*(.*)$", re.sub(r"//.*$", "", line))
            if not m:
                continue
            d, rest = m.group(1), m.group(2).strip()
            active = all(s[0] for s in stack)
            if d in ("if", "ifdef", "ifndef"):
                if not active:
                    stack.append([False, True])
                    continue
                v = {"if": lambda: self.eval(rest),
                     "ifdef": lambda: rest.split()[0] in self.defs,
                     "ifndef": lambda: rest.split()[0] not in self.defs}[d]()
                stack.append([bool(v), bool(v)])
            elif d == "elif":
                top = stack[-1]
                parent = all(s[0] for s in stack[:-1])
                v = parent and not top[1] and bool(self.eval(rest))
                top[0] = v
                top[1] = top[1] or v
            elif d == "else":
                top = stack[-1]
                top[0], top[1] = not top[1], True
            elif d == "endif":
                stack.pop()
            elif not active:
                continue
            elif d == "define":
                k = re.match(r"(\w+)(\(?)\s*(.*)$", rest)
                if k and k.group(2) != "(":
                    self.defs[k.group(1)] = k.group(3).strip()
            elif d == "undef":
                self.defs.pop(rest.split()[0], None)
            elif d == "error":
                raise ValueError(f"{name}: #error {rest}")
            elif d == "include":
                k = re.match(r'"(config_\w+\.h)"', rest)
                if k:
                    self.run(k.group(1))
        if stack:
            raise ValueError(f"{name}: unterminated #if")
        self.depth -= 1

    def value(self, name, seen=()):
        if name in seen or name not in self.defs:
            return "0"
        v = self.defs[name]
        return self.value(v, seen + (name,)) if re.fullmatch(r"[A-Za-z_]\w*", v) else v

    def eval(self, expr):
        expr = re.sub(r"defined\s*\(?\s*(\w+)\s*\)?", lambda m: str(int(m.group(1) in self.defs)), expr)
        toks = re.findall(r"0[xX][0-9a-fA-F]+|\d+|[A-Za-z_]\w*|&&|\|\||[!=<>]=|[-+!()<>]|\S", expr)
        out = []
        for t in toks:
            if re.fullmatch(r"[A-Za-z_]\w*", t):
                v = self.value(t) or "0"
                if not re.fullmatch(r"-?(0[xX][0-9a-fA-F]+|\d+)", v):
                    raise ValueError(f"cannot evaluate {t} = {v!r}")
                out.append(f"({v})")
            elif t in ("&&", "||", "!"):
                out.append({"&&": " and ", "||": " or ", "!": " not "}[t])
            elif re.fullmatch(r"0[xX][0-9a-fA-F]+|\d+|[!=<>]=|[-+()<>]", t):
                out.append(t)
            else:
                raise ValueError(f"unsupported token {t!r} in #if {expr}")
        # `!A != 1` style precedence does not occur; "not" binds looser in Python, so parenthesise it
        src = re.sub(r" not \s*(\(\d+\)|\d+)", r"(0 if \1 else 1)", "".join(out))
        return int(eval(src, {"__builtins__": {}}, {}))


def evaluate(root):
    """{mod: {macro: value}} with config.h defaults applied."""
    res = {}
    for mod in MODS:
        pp = Preprocessor(root, {f"TDRAW_CONFIG_{mod.upper()}": ""})
        pp.run("config.h")
        res[mod] = {k: pp.value(k) if re.fullmatch(r"[A-Za-z_]\w*", v) else v
                    for k, v in pp.defs.items()
                    if not k.startswith("TDRAW_CONFIG_")}
    return res


def rows(values, spec):
    """[(label, [cell per mod], kind)] and a list of problems."""
    out, problems, covered = [], [], set(spec.get("ignore", {}))
    for r in spec["rows"]:
        covered.update(r["macros"])
        cells = []
        for mod in MODS:
            vs = {values[mod].get(m) for m in r["macros"]}
            if None in vs:
                problems.append(f"{mod}: {r['macros']} is not defined ({r['label']})")
            if len(vs) > 1:
                problems.append(f"{mod}: {r['macros']} disagree, so they need separate rows")
            v = sorted(x or "?" for x in vs)[0]
            if r["kind"] == "bool":
                v = {"0": "n", "1": "y"}.get(v, v)
            cells.append(v)
        if len(set(cells)) > 1:                 # the table lists differences only
            out.append((r["label"], cells, r.get("note")))
    macros = {m for mod in MODS for m in values[mod]}
    for m in sorted(macros - covered):
        if len({values[mod].get(m) for mod in MODS}) > 1:
            problems.append(f"{m} differs between mods but has no row in config_table.json")
    return out, problems


def text_table(rs):
    head = " " * 60 + "prota  esc  mayhem  ota  tazero  bta  twilight"
    cols = (62, 68, 74, 81, 87, 94, 101)
    lines = [head]
    for label, cells, _ in rs:
        line = "  " + label
        for c, v in zip(cols, cells):
            line = line.ljust(max(c, len(line) + 1)) + v
        lines.append(line)
    return lines


def markdown_table(rs):
    mark = {"y": "\U0001F7E2✓", "n": "\U0001F534✗"}
    out = ["| Feature | ProTA | ESC | Mayhem | OTA | TA Zero | BTA | Twilight |",
           "|---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|"]
    for label, cells, _ in rs:
        out.append("| " + re.sub(r"\s+", " ", label) + " | " + " | ".join(mark.get(c, c) for c in cells) + " |")
    return "\n".join(out)


def replace_table(text, lines):
    ls = text.split("\n")
    start = next(i for i, x in enumerate(ls) if HEADER_LINE.match(x))
    end = next(i for i in range(start, len(ls)) if not ls[i].strip())
    return "\n".join(ls[:start] + lines + ls[end:])


def main(argv):
    here = Path(__file__).resolve().parent
    root = here.parent
    spec = json.loads((here / "config_table.json").read_text(encoding="utf-8"))
    rs, problems = rows(evaluate(root), spec)
    if "--markdown" in argv:
        sys.stdout.reconfigure(encoding="utf-8")
        print(markdown_table(rs))
    elif not argv:
        print("\n".join(text_table(rs)))
    for p in problems:
        print("PROBLEM: " + p, file=sys.stderr)
    txt = root / "tdraw.txt"
    old = txt.read_bytes().decode("utf-8")
    new = replace_table(old, text_table(rs))
    if "--write" in argv and not problems:
        txt.write_bytes(new.encode("utf-8"))
    if "--check" in argv and new != old:
        problems.append("stale")
        print("PROBLEM: the table in tdraw.txt is out of date; run config_table.py --write", file=sys.stderr)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
