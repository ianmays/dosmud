#!/bin/sh
# Compare branch diff vs base for missing unit/snapshot test updates.
# See docs/testing.md and .cursor/skills/testing-gap-auditor/SKILL.md.

set -e

BASE="${1:-${TEST_GAP_BASE:-origin/main}}"
WAIVER="tests/.test-gap-waiver"
FAIL=0

if [ -f "$WAIVER" ]; then
    reason=$(head -1 "$WAIVER")
    echo "test-gap: waived (${reason})"
    exit 0
fi

if [ "$TEST_GAP_WAIVE" = "1" ]; then
    echo "test-gap: waived (TEST_GAP_WAIVE=1, local debug only)"
    exit 0
fi

if ! git rev-parse --verify "$BASE" >/dev/null 2>&1; then
    echo "test-gap: error: base ref not found: $BASE" >&2
    exit 1
fi

DIFF_RANGE="${BASE}...HEAD"
# Working tree + index vs base (local uncommitted); union with committed range (CI / pushed branch).
NAME_ONLY=$(git diff --name-only "$BASE" 2>/dev/null || true)
COMMITTED=$(git diff --name-only "$DIFF_RANGE" 2>/dev/null || true)
NAME_ONLY=$(printf '%s\n%s' "$NAME_ONLY" "$COMMITTED" | sort -u | grep -v '^$' || true)

if [ -z "$NAME_ONLY" ]; then
    echo "test-gap: pass (no diff vs $BASE)"
    exit 0
fi

# Early exit: tooling/docs-only PR (stack introduction, DEV_PLAN, etc.)
needs_check=0
for path in $NAME_ONLY; do
    case "$path" in
        src/*|include/*|tests/unit/*|tests/regression/*|tests/harness/*)
            needs_check=1
            break
            ;;
        Makefile)
            if git diff "$BASE" -- Makefile 2>/dev/null | grep -q 'SNAPSHOT_TESTS'; then
                needs_check=1
                break
            fi
            if git diff "$DIFF_RANGE" -- Makefile 2>/dev/null | grep -q 'SNAPSHOT_TESTS'; then
                needs_check=1
                break
            fi
            ;;
    esac
done

if [ "$needs_check" = "0" ]; then
    echo "test-gap: pass (no gameplay or test diff vs $BASE)"
    exit 0
fi

unit_file_for_module() {
    mod="$1"
    case "$mod" in
        command) echo "tests/unit/unit_cmd.c" ;;
        invent) echo "tests/unit/unit_inv.c" ;;
        combat) echo "tests/unit/unit_cbt.c" ;;
        dialogue) echo "tests/unit/unit_dial.c" ;;
        world) echo "tests/unit/unit_wrld.c" ;;
        game) echo "tests/unit/unit_game.c" ;;
        gout) echo "tests/unit/unit_gout.c" ;;
        genc) echo "tests/unit/unit_genc.c" ;;
        gprog) echo "tests/unit/unit_gprog.c" ;;
        gatmos) echo "tests/unit/unit_gatmos.c" ;;
        wanderer) echo "tests/unit/unit_wandr.c" ;;
        fmt) echo "tests/unit/unit_fmt.c" ;;
        items) echo "tests/unit/unit_item.c" ;;
        testharn) echo "tests/unit/unit_tharn.c tests/unit/unit_harn.c" ;;
        *) echo "" ;;
    esac
}

module_from_header() {
    base=$(basename "$1" .h)
    echo "$base"
}

file_changed_non_whitespace() {
    path="$1"
    if git diff -w "$BASE" -- "$path" 2>/dev/null | grep -q '^[+-][^+-]'; then
        return 0
    fi
    if git diff -w "$DIFF_RANGE" -- "$path" 2>/dev/null | grep -q '^[+-][^+-]'; then
        return 0
    fi
    return 1
}

unit_touched_for_module() {
    mod="$1"
    units=$(unit_file_for_module "$mod")
    for u in $units; do
        if echo "$NAME_ONLY" | grep -qx "$u"; then
            return 0
        fi
    done
    return 1
}

regression_touched() {
    echo "$NAME_ONLY" | grep -q '^tests/regression/' && return 0
    if echo "$NAME_ONLY" | grep -qx 'Makefile'; then
        if git diff "$BASE" -- Makefile 2>/dev/null | grep -q 'SNAPSHOT_TESTS'; then
            return 0
        fi
        if git diff "$DIFF_RANGE" -- Makefile 2>/dev/null | grep -q 'SNAPSHOT_TESTS'; then
            return 0
        fi
    fi
    return 1
}

# --- Heuristic 1: in-scope header without unit update ---
for path in $NAME_ONLY; do
    case "$path" in
        include/*.h|src/*.h)
            ;;
        *)
            continue
            ;;
    esac
    if ! file_changed_non_whitespace "$path"; then
        continue
    fi
    mod=$(module_from_header "$path")
    units=$(unit_file_for_module "$mod")
    if [ -z "$units" ]; then
        continue
    fi
    if unit_touched_for_module "$mod"; then
        continue
    fi
    if regression_touched; then
        continue
    fi
    echo "test-gap: unit gap: $path changed without matching unit test update ($units)" >&2
    FAIL=1
done

# --- Heuristic 2: coverage-module .c without unit or regression ---
COVERAGE="command invent combat game genc wanderer dialogue gatmos world gprog items fmt gout testharn"

for mod in $COVERAGE; do
    src="src/${mod}.c"
    case "$mod" in
        testharn) src="tests/harness/testharn.c" ;;
    esac
    if ! echo "$NAME_ONLY" | grep -qx "$src"; then
        continue
    fi
    if ! file_changed_non_whitespace "$src"; then
        continue
    fi
    if unit_touched_for_module "$mod"; then
        continue
    fi
    if regression_touched; then
        continue
    fi
    echo "test-gap: unit gap: $src changed without tests/unit or tests/regression update" >&2
    FAIL=1
done

# --- Heuristic 3: player-visible paths without snapshot touch ---
PLAYER="src/gout.c src/grendr.c src/game.c src/command.c src/invent.c src/combat.c src/dialogue.c src/genc.c src/wanderer.c src/gatmos.c src/gprog.c src/fmt.c tests/harness/testharn.c"

player_changed=0
for p in $PLAYER; do
    if echo "$NAME_ONLY" | grep -qx "$p"; then
        if file_changed_non_whitespace "$p"; then
            player_changed=1
            break
        fi
    fi
done

if [ "$player_changed" = "1" ]; then
    if ! regression_touched; then
        echo "test-gap: snapshot gap: player-visible source changed without tests/regression or SNAPSHOT_TESTS update" >&2
        FAIL=1
    fi
fi

# --- Heuristic 4: new .input not listed in SNAPSHOT_TESTS ---
for path in $NAME_ONLY; do
    case "$path" in
        tests/regression/*.input)
            ;;
        *)
            continue
            ;;
    esac
    if ! git diff "$BASE" --diff-filter=A -- "$path" 2>/dev/null | grep -q .; then
        if ! git diff "$DIFF_RANGE" --diff-filter=A -- "$path" 2>/dev/null | grep -q .; then
            continue
        fi
    fi
    name=$(basename "$path" .input)
    if ! grep -q "$name" Makefile 2>/dev/null; then
        echo "test-gap: snapshot gap: new $path not listed in Makefile SNAPSHOT_TESTS" >&2
        FAIL=1
    fi
done

# --- Warn only (exit 0): game.c without unit_game.c ---
if echo "$NAME_ONLY" | grep -qx 'src/game.c'; then
    if file_changed_non_whitespace 'src/game.c'; then
        if ! echo "$NAME_ONLY" | grep -qx 'tests/unit/unit_game.c'; then
            echo "test-gap: note: src/game.c changed without tests/unit/unit_game.c (ok if static router only)" >&2
        fi
    fi
fi

if [ "$FAIL" -ne 0 ]; then
    echo "test-gap: fail (add tests or commit tests/.test-gap-waiver with reason)" >&2
    exit 1
fi

echo "test-gap: pass"
exit 0
