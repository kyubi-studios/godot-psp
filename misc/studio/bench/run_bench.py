#!/usr/bin/env python3
"""Run editor benchmark scenarios against a project made by make_project.py.

Scenarios:
  cold_import  delete .godot/, then `--headless --import` (full scan + import of every asset)
  warm_open    `--headless --import` with an up-to-date .godot/ (startup scan cost only)
  scene_load   `-s res://bench_load.gd` → time to load the big scene without the resource cache

Usage: run_bench.py <godot_bin> <project_dir> [--runs 3] [--scenarios a,b] [--json out.json] [--label NAME]
"""

import argparse
import datetime
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

SCENARIOS = ("cold_import", "warm_open", "scene_load")
LOAD_RE = re.compile(r"BENCH_LOAD_MS=([0-9.]+)")


def median(values):
    ordered = sorted(values)
    mid = len(ordered) // 2
    if len(ordered) % 2:
        return ordered[mid]
    return (ordered[mid - 1] + ordered[mid]) / 2


def parse_load_ms(text):
    match = LOAD_RE.search(text)
    return float(match.group(1)) if match else None


def summarize(samples):
    ok = [s for s in samples if s is not None]
    return {
        "median": median(ok) if ok else None,
        "min": min(ok) if ok else None,
        "max": max(ok) if ok else None,
        "runs": len(ok),
        "failed": len(samples) - len(ok),
        "samples": samples,
    }


def _run(cmd, timeout, env=None):
    """Return (elapsed_ms, output) or (None, output) when the process fails or times out."""
    start = time.perf_counter()
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout, env=env)
    except subprocess.TimeoutExpired as e:
        return None, (e.stdout or "") if isinstance(e.stdout, str) else ""
    elapsed = (time.perf_counter() - start) * 1000.0
    output = proc.stdout + proc.stderr
    if proc.returncode != 0:
        return None, output
    return elapsed, output


def time_command(cmd, timeout):
    return _run(cmd, timeout)[0]


def run_scenario(name, godot, project, env, timeout):
    if name == "cold_import":
        shutil.rmtree(os.path.join(project, ".godot"), ignore_errors=True)
        return _run([godot, "--headless", "--path", project, "--import"], timeout, env)[0]
    if name == "warm_open":
        return _run([godot, "--headless", "--path", project, "--import"], timeout, env)[0]
    if name == "scene_load":
        elapsed, output = _run([godot, "--headless", "--path", project, "-s", "res://bench_load.gd"], timeout, env)
        return parse_load_ms(output) if elapsed is not None else None
    raise ValueError(f"unknown scenario: {name}")


def _git_head():
    try:
        return subprocess.run(["git", "rev-parse", "--short", "HEAD"], capture_output=True, text=True, check=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("godot")
    parser.add_argument("project")
    parser.add_argument("--runs", type=int, default=3)
    parser.add_argument("--scenarios", default=",".join(SCENARIOS))
    parser.add_argument("--json")
    parser.add_argument("--label", default="run")
    parser.add_argument("--timeout", type=int, default=1800)
    args = parser.parse_args(argv)

    scenarios = [s for s in args.scenarios.split(",") if s]
    for s in scenarios:
        if s not in SCENARIOS:
            parser.error(f"unknown scenario '{s}' (choose from {', '.join(SCENARIOS)})")

    project = os.path.abspath(args.project)
    # Isolate editor settings and caches from the user's own.
    sandbox = tempfile.mkdtemp(prefix="studio-bench-")
    env = dict(os.environ, XDG_CONFIG_HOME=f"{sandbox}/config", XDG_DATA_HOME=f"{sandbox}/data", XDG_CACHE_HOME=f"{sandbox}/cache")

    try:
        if "warm_open" in scenarios or "scene_load" in scenarios:
            # Make sure the project is imported before warm scenarios.
            _run([args.godot, "--headless", "--path", project, "--import"], args.timeout, env)

        results = {}
        for name in scenarios:
            samples = []
            for i in range(args.runs):
                value = run_scenario(name, args.godot, project, env, args.timeout)
                print(f"{name} #{i + 1}: {'FAILED' if value is None else f'{value:.1f} ms'}", file=sys.stderr)
                samples.append(value)
            results[name] = summarize(samples)
            if name == "cold_import":
                # Leave the project imported for the following scenarios.
                _run([args.godot, "--headless", "--path", project, "--import"], args.timeout, env)
    finally:
        shutil.rmtree(sandbox, ignore_errors=True)

    report = {
        "label": args.label,
        "commit": _git_head(),
        "date": datetime.datetime.now().isoformat(timespec="seconds"),
        "cpus": os.cpu_count(),
        "godot": os.path.abspath(args.godot),
        "project": project,
        "results": results,
    }
    if args.json:
        with open(args.json, "w", encoding="utf-8") as f:
            json.dump(report, f, indent=1)

    print(f"| {args.label} ({report['commit']}, {report['cpus']} CPUs) | median ms | min | max | failed |")
    print("|---|---:|---:|---:|---:|")
    for name, r in results.items():
        fmt = lambda v: "—" if v is None else f"{v:.1f}"  # noqa: E731
        print(f"| {name} | {fmt(r['median'])} | {fmt(r['min'])} | {fmt(r['max'])} | {r['failed']} |")
    return 1 if any(r["median"] is None for r in results.values()) else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
