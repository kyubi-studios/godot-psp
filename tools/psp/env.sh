#!/usr/bin/env bash
# Kaynaklayın: source tools/psp/env.sh
export PSPDEV="$HOME/pspdev"
export PATH="$PSPDEV/bin:$PATH"
export PPSSPP_HEADLESS="$HOME/ppsspp-build/PPSSPPHeadless"
export GODOT_PSP_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
