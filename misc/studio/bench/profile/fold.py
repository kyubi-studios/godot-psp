#!/usr/bin/env python3
"""Summarize folded stacks from gdb_sample.py: inclusive and self percentages. Usage: fold.py samples.txt [N]"""
import sys, collections
# Self and inclusive counts per function from folded stacks.
incl = collections.Counter(); self_c = collections.Counter(); total = 0
for line in open(sys.argv[1]):
    c, s = line.split(" ", 1); c = int(c); total += c
    fr = s.strip().split(";")
    for name in set(fr): incl[name] += c
    self_c[fr[-1]] += c
n = int(sys.argv[2]) if len(sys.argv) > 2 else 30
print("total samples", total)
print("--- inclusive")
for k, v in incl.most_common(n): print(f"{100*v/total:5.1f}% {k[:110]}")
print("--- self")
for k, v in self_c.most_common(15): print(f"{100*v/total:5.1f}% {k[:110]}")
