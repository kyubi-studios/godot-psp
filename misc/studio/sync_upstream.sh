#!/usr/bin/env bash
# Merge the tracked upstream Godot branch into the current branch (normally `main`),
# then build, run the tests and the editor smoke test.
#
# Usage: misc/studio/sync_upstream.sh [--no-build] [--branch 4.7]
# Exit codes: 0 = up to date or merged and verified, 1 = verification failed,
#             2 = merge conflict (left in progress for manual resolution).
set -u

UPSTREAM_REMOTE="upstream"
UPSTREAM_URL="https://github.com/godotengine/godot.git"
UPSTREAM_BRANCH="4.7"
BUILD=1

while [ $# -gt 0 ]; do
	case "$1" in
		--no-build) BUILD=0 ;;
		--branch) UPSTREAM_BRANCH="$2"; shift ;;
		*) echo "unknown argument: $1"; exit 1 ;;
	esac
	shift
done

cd "$(git rev-parse --show-toplevel)" || exit 1
git config rerere.enabled true
git config rerere.autoupdate true

if ! git diff --quiet || ! git diff --cached --quiet; then
	echo "Working tree has uncommitted changes; commit or stash them first."
	exit 1
fi

git remote get-url "$UPSTREAM_REMOTE" > /dev/null 2>&1 || git remote add "$UPSTREAM_REMOTE" "$UPSTREAM_URL"
git fetch "$UPSTREAM_REMOTE" "$UPSTREAM_BRANCH:refs/remotes/$UPSTREAM_REMOTE/$UPSTREAM_BRANCH" || exit 1
TARGET="$UPSTREAM_REMOTE/$UPSTREAM_BRANCH"

if git merge-base --is-ancestor "$TARGET" HEAD; then
	echo "Already up to date with $TARGET."
	exit 0
fi

echo "Merging $(git rev-list --count HEAD.."$TARGET") new upstream commit(s) from $TARGET..."
if ! git merge --no-edit -m "Merge $TARGET into $(git branch --show-current)" "$TARGET"; then
	if [ -z "$(git diff --name-only --diff-filter=U)" ]; then
		# rerere resolved every conflict; finish the merge.
		git commit --no-edit || exit 2
	else
		echo "Merge conflicts in:"
		git diff --name-only --diff-filter=U
		echo "Resolve them (see STUDIO_HOOKS.md), then 'git commit'. Or 'git merge --abort'."
		exit 2
	fi
fi

python3 misc/studio/check_hooks.py || exit 1

if [ $BUILD -eq 1 ]; then
	scons platform=linuxbsd target=editor tests=yes debug_symbols=no -j"$(nproc)" || exit 1
	BIN=bin/godot.linuxbsd.editor.x86_64
	"$BIN" --headless --test || exit 1
	misc/studio/smoke_test.sh "$BIN" || exit 1
fi

echo "Upstream merge verified."
exit 0
