"""Sampling profiler for when `perf` is unavailable (kernel.perf_event_paranoid > 2).

gdb launches the program as its child (allowed with ptrace_scope=1); an external shell
loop sends SIGINT every SAMPLE_MS milliseconds and the main thread's stack is recorded.
Output: folded stacks ("count frame;frame;...") for fold.py or flamegraph tools.

Usage:
  SAMPLE_OUT=out.txt SAMPLE_MS=5 gdb -q -batch -x gdb_sample.py --args <godot> <args...>

The editor binary is stripped by default (link flag -s when debug_symbols=no). Relink an
unstripped copy without recompiling: take the final link command from
`scons <your flags> verbose=yes -n` (after moving the binary away), drop "-s" and change "-o".
"""
import gdb, os, subprocess, collections

out_path = os.environ.get("SAMPLE_OUT", "samples.txt")
interval = float(os.environ.get("SAMPLE_MS", "20")) / 1000.0
stacks = collections.Counter()

gdb.execute("set pagination off")
gdb.execute("set confirm off")
gdb.execute("handle SIGINT stop print nopass")
gdb.execute("handle SIGPIPE nostop noprint pass")
gdb.execute("handle SIG32 nostop noprint pass")
gdb.execute("handle SIG33 nostop noprint pass")
gdb.execute("set print thread-events off")

gdb.execute("starti", to_string=True)
pid = gdb.selected_inferior().pid
ticker_proc = subprocess.Popen(["sh", "-c", f"while kill -INT {pid} 2>/dev/null; do sleep {interval}; done"])
gdb.execute("continue", to_string=True)
while True:
    inf = gdb.selected_inferior()
    if not inf or inf.pid == 0:
        break
    try:
        main = inf.threads()[-1]  # lowest-numbered thread is main; threads() is newest-first
        for th in inf.threads():
            if th.num == 1:
                main = th
        main.switch()
        f = gdb.newest_frame()
        names = []
        depth = 0
        while f is not None and depth < 40:
            names.append(f.name() or "??")
            f = f.older()
            depth += 1
        stacks[";".join(reversed(names))] += 1
    except gdb.error:
        pass
    try:
        gdb.execute("continue", to_string=True)
    except gdb.error:
        break
ticker_proc.kill()
with open(out_path, "w") as fo:
    for s, c in stacks.most_common():
        fo.write(f"{c} {s}\n")
