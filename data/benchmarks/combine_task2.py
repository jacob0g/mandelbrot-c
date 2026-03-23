#!/usr/bin/env python3
"""
Combine per-chunksize timing data files into a single summary file.

Each input file contains per-rank timing breakdowns for a specific
(N, RANK, CHUNKSIZE) configuration. This script aggregates them into
one output file per (N, RANK) pair with statistics across chunk sizes.
"""

import argparse
import glob
import os
import re
import sys
from collections import defaultdict


FILE_PATTERN = re.compile(r"N_(\d+)-RANK_(\d+)-CHUNKSIZE_(\d+)\.dat$")


def parse_timing_file(filepath):
    """Parse a single timing data file, returning lists of (work, wait, comm) per rank."""
    works, waits, comms = [], [], []
    with open(filepath) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            works.append(float(parts[1]))
            waits.append(float(parts[2]))
            comms.append(float(parts[3]))
    return works, waits, comms


def compute_statistics(works, waits, comms):
    """Compute per-chunksize summary: total runtime, mean and variance of each timing component."""
    n = len(works)
    total_runtime = max(w + wa + c for w, wa, c in zip(works, waits, comms))

    avg_work = sum(works) / n
    avg_wait = sum(waits) / n
    avg_comm = sum(comms) / n

    # Population variance (every rank is measured, not a sample)
    var_work = sum((x - avg_work) ** 2 for x in works) / n
    var_wait = sum((x - avg_wait) ** 2 for x in waits) / n
    var_comm = sum((x - avg_comm) ** 2 for x in comms) / n

    return total_runtime, avg_work, avg_wait, avg_comm, var_work, var_wait, var_comm


def discover_files(data_dir):
    """Find and group timing files by (N, RANK), returning {(n, rank): [(chunksize, path), ...]}."""
    pattern = os.path.join(data_dir, "N_*-RANK_*-CHUNKSIZE_*.dat")
    groups = defaultdict(list)

    for filepath in glob.glob(pattern):
        match = FILE_PATTERN.search(os.path.basename(filepath))
        if not match:
            continue
        n, rank, chunksize = int(match.group(1)), int(match.group(2)), int(match.group(3))
        groups[(n, rank)].append((chunksize, filepath))

    return groups


def main():
    parser = argparse.ArgumentParser(
        description="Combine per-chunksize timing files into summary tables."
    )
    parser.add_argument(
        "data_dir",
        help="Directory containing N_*-RANK_*-CHUNKSIZE_*.dat timing files",
    )
    args = parser.parse_args()

    data_dir = args.data_dir
    if not os.path.isdir(data_dir):
        print(f"Error: '{data_dir}' is not a directory", file=sys.stderr)
        sys.exit(1)

    groups = discover_files(data_dir)
    if not groups:
        print(f"No matching timing files found in '{data_dir}'", file=sys.stderr)
        sys.exit(1)

    for (n, rank), files in sorted(groups.items()):
        results = []
        for chunksize, filepath in files:
            works, waits, comms = parse_timing_file(filepath)
            stats = compute_statistics(works, waits, comms)
            results.append((chunksize, *stats))

        results.sort(key=lambda r: r[0])

        output_name = f"combined_N_{n}-RANK_{rank}.dat"
        output_path = os.path.join(data_dir, output_name)
        with open(output_path, "w") as out:
            out.write(
                "# chunksize\ttotal_runtime\tavg_work\tavg_wait\tavg_comm"
                "\tvar_work\tvar_wait\tvar_comm\n"
            )
            for row in results:
                out.write(f"{row[0]}\t" + "\t".join(f"{v:.6f}" for v in row[1:]) + "\n")

        print(f"Wrote {len(results)} rows to {output_path}")


if __name__ == "__main__":
    main()
