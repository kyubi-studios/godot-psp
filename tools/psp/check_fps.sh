#!/usr/bin/env bash
# Kullanım: check_fps.sh <test.log> <min_fps> — ardışık "[PSP] frame N ... t=" satırları arasındaki en kötü FPS >= min_fps.
set -euo pipefail
MIN="$2"
python3 - "$1" "$MIN" <<'PY'
import re, sys
log, mn = sys.argv[1], float(sys.argv[2])
pts = [(int(f), int(t)) for f, t in re.findall(r'\[PSP\] frame (\d+) .*?t=(\d+)', open(log, errors='ignore').read())]
if len(pts) < 2:
    print("[check_fps] FAIL not enough frame samples"); sys.exit(1)
worst = min((f2 - f1) * 1000.0 / max(t2 - t1, 1) for (f1, t1), (f2, t2) in zip(pts, pts[1:]))
print(f"[check_fps] {'PASS' if worst >= mn else 'FAIL'} worst {worst:.1f} fps (min {mn})")
sys.exit(0 if worst >= mn else 1)
PY
