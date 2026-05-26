#!/usr/bin/env python3
"""Append one CI stats run to the persistent metrics history file."""

import argparse
import json
from pathlib import Path


def load_json(path):
    with open(path, "r", encoding="utf-8") as handle:
        return json.load(handle)


def build_record(stats):
    steps = {}
    for step in stats.get("steps", []):
        duration_ms = step.get("duration_ms")
        if duration_ms is None or duration_ms < 0:
            continue
        steps[step["name"]] = duration_ms

    benchmarks = {}
    for bench in stats.get("benchmarks", []):
        us_per_tick = bench.get("us_per_tick")
        if us_per_tick is None:
            continue
        benchmarks[bench["name"]] = us_per_tick

    return {
        "timestamp": stats["generated_at"],
        "sha": stats["sha"],
        "result": stats["result"],
        "steps": steps,
        "benchmarks": benchmarks,
    }


def load_history(path):
    if not path.exists():
        return {"runs": []}
    return load_json(path)


def write_history(path, history):
    with open(path, "w", encoding="utf-8") as handle:
        json.dump(history, handle, indent=2)
        handle.write("\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True, dest="input_path")
    parser.add_argument("--output", required=True, dest="output_path")
    args = parser.parse_args()

    input_path = Path(args.input_path)
    output_path = Path(args.output_path)

    stats = load_json(input_path)
    history = load_history(output_path)
    history["runs"].append(build_record(stats))
    write_history(output_path, history)


if __name__ == "__main__":
    main()
