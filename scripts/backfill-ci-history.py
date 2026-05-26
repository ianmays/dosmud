#!/usr/bin/env python3
"""Backfill CI history entries with derived test counts and coverage data."""

import argparse
import json
import re
from pathlib import Path


SNAPSHOT_RE = re.compile(r"snapshot tests passed: (\d+)/(\d+)")
UNIT_RE = re.compile(r"^Pass: (\d+), fail: (\d+), skip: (\d+)\.$", re.MULTILINE)
BENCH_RE = re.compile(r"SOAK_BENCH ([^ ]+)")
COVERAGE_RE = re.compile(r"^\s*overall\s+([0-9]+\.[0-9]+)\s*/\s*([0-9]+\.[0-9]+)\s*$", re.MULTILINE)


def load_json(path):
    with open(path, "r", encoding="utf-8") as handle:
        return json.load(handle)


def write_json(path, data):
    with open(path, "w", encoding="utf-8") as handle:
        json.dump(data, handle, indent=2)
        handle.write("\n")


def parse_log(text):
    snapshot_match = SNAPSHOT_RE.search(text)
    if snapshot_match:
        snapshot_pass = int(snapshot_match.group(1))
        snapshot_total = int(snapshot_match.group(2))
        snapshot_counts = {
            "pass": snapshot_pass,
            "fail": snapshot_total - snapshot_pass,
            "skip": 0,
        }
    else:
        snapshot_counts = {"pass": 0, "fail": 1, "skip": 0}

    unit_match = UNIT_RE.search(text)
    if unit_match:
        unit_counts = {
            "pass": int(unit_match.group(1)),
            "fail": int(unit_match.group(2)),
            "skip": int(unit_match.group(3)),
        }
    else:
        unit_counts = {"pass": 0, "fail": 1, "skip": 0}

    benches = sorted(set(BENCH_RE.findall(text)))
    soak_counts = {"pass": len(benches), "fail": 0, "skip": 0}

    coverage_match = COVERAGE_RE.search(text)
    if not coverage_match:
        raise ValueError("Could not parse overall unit coverage from log.")
    coverage = {
        "branch_pct": float(coverage_match.group(1)),
        "line_pct": float(coverage_match.group(2)),
    }

    return {
        "test_counts": {
            "snapshots": snapshot_counts,
            "unit": unit_counts,
            "soak": soak_counts,
        },
        "unit_coverage_overall": coverage,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--history", required=True, dest="history_path")
    parser.add_argument("--sha", required=True)
    parser.add_argument("--log", required=True, dest="log_path")
    args = parser.parse_args()

    history_path = Path(args.history_path)
    log_path = Path(args.log_path)
    history = load_json(history_path)
    log_text = log_path.read_text(encoding="utf-8")
    derived = parse_log(log_text)

    for run in history.get("runs", []):
        if run.get("sha") == args.sha:
            run["test_counts"] = derived["test_counts"]
            run["unit_coverage_overall"] = derived["unit_coverage_overall"]
            write_json(history_path, history)
            return

    raise ValueError("Could not find matching SHA in history.")


if __name__ == "__main__":
    main()
