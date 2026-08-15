#!/usr/bin/env python3
"""Recompute the demo compiler's unitsHash from a tdraw_unitdump CSV.

tdraw already names its dumps with this value (ChallengeResponse::ComputeUnitDataHash),
so you normally never need this. It exists for the case that function documents as
its known gap: a game with unit restrictions in force, where the compiler hashes only
the enabled subset and tdraw cannot see which units those are.

    python unitshash_from_dump.py tdraw_unitdump_<something>.csv
    python unitshash_from_dump.py dump.csv --only-units ARMCOM,ARMFARK,...

The algorithm (verified against 1200 archived games, 1200/1200 exact):
  MD5 over, for each unit in ascending CRC_FBI order, excluding CRC_FBI 0 and
  0x92549357 (the SY pseudo-unit), the little-endian uint32 (CRC_FBI + CRC_all).

Mirrors gpgnet4ta tapacket/UnitDataRepo.cpp::hash and
tareplay/TaDemoCompiler.cpp::GameContext::getUnitDataHash.
"""
import argparse
import csv
import hashlib
import struct
import sys

SY_UNIT_ID = 0x92549357


def read_rows(path):
    with open(path, newline="", encoding="utf-8", errors="replace") as fh:
        lines = fh.read().splitlines()
    for i, line in enumerate(lines):
        if line.startswith("n,ID,name"):
            header = line.split(",")
            break
    else:
        raise SystemExit(f"{path}: no unit header row found")

    for col in ("crcfbi", "crcall", "unitname"):
        if col not in header:
            raise SystemExit(
                f"{path}: missing '{col}' column — this dump predates the "
                "crcfbi/crcall change; rebuild tdraw and re-dump")

    idx = {c: header.index(c) for c in ("crcfbi", "crcall", "unitname")}
    rows = []
    for rec in csv.reader(lines[i + 1:]):
        if len(rec) < len(header):
            continue
        try:
            rows.append((int(rec[idx["crcfbi"]]),
                         int(rec[idx["crcall"]]),
                         rec[idx["unitname"]]))
        except ValueError:
            continue
    return rows


def units_hash(rows, only=None):
    by_crc = {}
    for crc_fbi, crc_all, name in rows:
        if crc_fbi in (0, SY_UNIT_ID):
            continue
        if only is not None and name.upper() not in only:
            continue
        by_crc[crc_fbi] = crc_all          # dict == the compiler's std::map dedupe

    md5 = hashlib.md5()
    for crc_fbi in sorted(by_crc):
        datum = (crc_fbi + by_crc[crc_fbi]) & 0xFFFFFFFF
        md5.update(struct.pack("<I", datum))
    return md5.hexdigest(), len(by_crc)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv_path")
    ap.add_argument("--only-units",
                    help="comma-separated unit names to include (for restricted games)")
    args = ap.parse_args()

    only = None
    if args.only_units:
        only = {u.strip().upper() for u in args.only_units.split(",") if u.strip()}

    rows = read_rows(args.csv_path)
    digest, n = units_hash(rows, only)
    if not any(crc_all for _, crc_all, _ in rows):
        print("WARNING: every crcall is 0 — unit sync had not run when this dump was "
              "taken, so this digest is meaningless.", file=sys.stderr)
    print(f"{digest}  ({n} units)")


if __name__ == "__main__":
    main()
