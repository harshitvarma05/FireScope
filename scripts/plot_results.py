#!/usr/bin/env python3
"""Plot FireScope measured CSV results (requires matplotlib)."""
import argparse
import csv
from collections import defaultdict
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", type=Path)
    parser.add_argument("--output", type=Path, default=Path("benchmarks/results/plots"))
    args = parser.parse_args()
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    with args.csv.open(newline="") as file:
        rows = list(csv.DictReader(file))
    grouped = defaultdict(list)
    for row in rows:
        grouped[row["backend"]].append(row)
    cpu = {int(row["grid"]): row for row in grouped.get("cpu", [])}
    args.output.mkdir(parents=True, exist_ok=True)
    plt.rcParams.update({"font.family": "sans-serif", "font.size": 11, "axes.spines.top": False, "axes.spines.right": False})
    for metric, label, filename in [
        ("total_ms", "Median execution time (ms)", "execution_time.png"),
        ("speedup", "Speedup over same-host serial CPU", "speedup.png"),
        ("mcell_updates_per_s", "Million cell updates / second", "throughput.png"),
    ]:
        figure, axes = plt.subplots(figsize=(8, 5), constrained_layout=True)
        for backend, data in grouped.items():
            data.sort(key=lambda row: int(row["grid"]))
            x, y = [], []
            for row in data:
                grid = int(row["grid"])
                if metric == "speedup":
                    if grid not in cpu:
                        continue
                    baseline = cpu[grid]
                    if row["steps"] != baseline["steps"] or row["seed"] != baseline["seed"]:
                        raise ValueError("Speedup requires matching grid, steps and seed")
                    value = float(baseline["total_ms"]) / float(row["total_ms"])
                else:
                    value = float(row[metric])
                x.append(grid)
                y.append(value)
            axes.plot(x, y, marker="o", linewidth=2, label=backend.upper() if backend == "cpu" else backend.capitalize())
        axes.set_xscale("log", base=2)
        if metric == "total_ms":
            axes.set_yscale("log")
        sizes = sorted({int(row["grid"]) for row in rows})
        axes.set_xticks(sizes, [f"{size}²" for size in sizes])
        axes.set_xlabel("Simulation grid")
        axes.set_ylabel(label)
        axes.set_title("FireScope — headless benchmark, warm-up + median of five runs")
        axes.grid(alpha=0.18)
        axes.legend()
        figure.savefig(args.output / filename, dpi=180)
        figure.savefig(args.output / filename.replace(".png", ".pdf"))
        plt.close(figure)


if __name__ == "__main__":
    main()
