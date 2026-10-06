#!/usr/bin/env python3
"""Collect median timings: each CLI benchmark does warm-up + five measured runs."""
import argparse
import csv
import datetime
import io
import json
import platform
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", required=True, type=Path)
    parser.add_argument("--backends", nargs="+", help="Default: available backends from this build/device")
    parser.add_argument("--grids", nargs="+", type=int, default=[128, 256, 512, 1024])
    parser.add_argument("--steps", type=int, default=500)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--output", type=Path, default=Path("benchmarks/results/local.csv"))
    args = parser.parse_args()
    executable = str(args.executable.resolve())
    available = subprocess.run([executable, "--list-backends"], text=True, capture_output=True, check=True).stdout.split()
    backends = args.backends or available
    if any(backend not in available for backend in backends):
        parser.error(f"Requested backend unavailable; this build/device supports: {', '.join(available)}")
    if args.steps <= 0 or any(size < 1 or size > 2048 for size in args.grids):
        parser.error("Steps must be positive and grid sizes must be within 1..2048")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    records = []
    for backend in backends:
        for grid in args.grids:
            print(f"Benchmarking {backend}, {grid} x {grid}, {args.steps} steps: warm-up + five runs", flush=True)
            command = [executable, "--backend", backend, "--grid", str(grid), "--steps", str(args.steps),
                       "--seed", str(args.seed), "--headless", "--benchmark"]
            result = subprocess.run(command, text=True, capture_output=True, check=True)
            rows = list(csv.DictReader(io.StringIO(result.stdout)))
            if len(rows) != 1:
                raise RuntimeError(f"Expected one CSV benchmark row: {result.stdout}")
            records.extend(rows)
            with args.output.open("w", newline="") as file:
                writer = csv.DictWriter(file, fieldnames=list(records[0]))
                writer.writeheader()
                writer.writerows(records)
    metadata = {
        "created_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "platform": platform.platform(), "architecture": platform.machine(),
        "backends": backends, "grids": args.grids, "steps": args.steps, "seed": args.seed,
        "warmup_runs": 1, "measured_runs": 5, "statistic": "median",
        "scope": "Headless wall time of synchronous step(batch); excludes initialization, rendering and final GPU readback",
        "note": "Same-host CPU/GPU comparison. Do not combine CPU baselines from another host for speedup.",
    }
    args.output.with_suffix(".json").write_text(json.dumps(metadata, indent=2) + "\n")
    print(f"Saved {args.output}")


if __name__ == "__main__":
    main()
