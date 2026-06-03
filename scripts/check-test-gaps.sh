#!/bin/sh
# Compare branch diff vs base for missing unit/snapshot test updates.
# Module lists: COVERAGE_MODULES, UNIT_GAMEPLAY_SRC, PLAT_SRC, HARNESS_SRC from Makefile.
# Unit suites: tests/unit/unit_*.c that #include "<mod>.h" (see tests/unit/module-map overrides).

set -e

BASE="${1:-${TEST_GAP_BASE:-origin/main}}"
MAP="tests/unit/module-map"
FAIL=0
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)

if [ "$TEST_GAP_WAIVE" = "1" ]; then
    echo "test-gap: skipped (TEST_GAP_WAIVE=1, local only; not used in CI)"
    exit 0
fi

if ! git rev-parse --verify "$BASE" >/dev/null 2>&1; then
    echo "test-gap: error: base ref not found: $BASE" >&2
    exit 1
fi

read_makefile_lists() {
    mf="$ROOT/Makefile"
    COVERAGE_MODULES=$(sed -n 's/^COVERAGE_MODULES = //p' "$mf" | head -1)
    gameplay=$(sed -n '/^UNIT_GAMEPLAY_SRC =/,/^UNIT_CORE_SRC =/p' "$mf" \
        | grep -oE 'src/[a-zA-Z0-9_]+\.c|tests/harness/[a-zA-Z0-9_]+\.c' \
        | sort -u)
    plat=$(grep '^PLAT_SRC = ' "$mf" | sed 's/^PLAT_SRC = //' | tr ' ' '\n' | sort -u)
    harness=$(sed -n 's/^HARNESS_SRC = //p' "$mf" \
        | grep -oE '[a-zA-Z0-9_]+\.c' \
        | sed 's/^/tests\/harness\//' \
        | sort -u)
    HARNESS_SRC_PATHS=$(echo "$harness" | tr '\n' ' ')
    SNAPSHOT_TESTS=$(sed -n '/^SNAPSHOT_TESTS =/,/^$/p' "$mf" \
        | sed '1d;$d' \
        | tr '\t\\' '  ' \
        | tr ' ' '\n' \
        | grep -v '^$' \
        | tr '\n' ' ')
    PLAYER_PATHS=$(printf '%s\n%s\n%s\n' "$gameplay" "$plat" "$harness" | sort -u | grep -v '^$' | tr '\n' ' ')
}

read_makefile_lists
cd "$ROOT"

MERGE_BASE=$(git merge-base "$BASE" HEAD 2>/dev/null || echo "$BASE")
DIFF_RANGE="${MERGE_BASE}...HEAD"
COMMITTED=$(git diff --name-only "$DIFF_RANGE" 2>/dev/null || true)
UNCOMMITTED=$(git diff --name-only HEAD 2>/dev/null || true)
NAME_ONLY=$(printf '%s\n%s' "$COMMITTED" "$UNCOMMITTED" | sort -u | grep -v '^$' || true)

if [ -z "$NAME_ONLY" ]; then
    echo "test-gap: pass (no diff vs $BASE)"
    exit 0
fi

makefile_touches_snapshot_tests() {
    if git diff "$DIFF_RANGE" -- Makefile 2>/dev/null | grep -q 'SNAPSHOT_TESTS'; then
        return 0
    fi
    if git diff HEAD -- Makefile 2>/dev/null | grep -q 'SNAPSHOT_TESTS'; then
        return 0
    fi
    return 1
}

needs_check=0
for path in $NAME_ONLY; do
    case "$path" in
        src/*|include/*|tests/unit/*|tests/regression/*|tests/harness/*)
            needs_check=1
            break
            ;;
        Makefile)
            if makefile_touches_snapshot_tests; then
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

module_in_coverage() {
    mod="$1"
    for m in $COVERAGE_MODULES; do
        if [ "$m" = "$mod" ]; then
            return 0
        fi
    done
    return 1
}

src_path_for_module() {
    mod="$1"
    for p in $HARNESS_SRC_PATHS; do
        base=$(basename "$p" .c)
        if [ "$base" = "$mod" ]; then
            echo "$p"
            return 0
        fi
    done
    echo "src/${mod}.c"
}

module_from_header() {
    basename "$1" .h
}

file_changed_non_whitespace() {
    path="$1"
    if git diff -w "$DIFF_RANGE" -- "$path" 2>/dev/null | grep -q '^[+-][^+-]'; then
        return 0
    fi
    if git diff -w HEAD -- "$path" 2>/dev/null | grep -q '^[+-][^+-]'; then
        return 0
    fi
    return 1
}

path_added_in_diff() {
    path="$1"
    if git diff "$DIFF_RANGE" --diff-filter=A -- "$path" 2>/dev/null | grep -q .; then
        return 0
    fi
    if git diff HEAD --diff-filter=A -- "$path" 2>/dev/null | grep -q .; then
        return 0
    fi
    return 1
}

snapshot_listed() {
    name="$1"
    for t in $SNAPSHOT_TESTS; do
        if [ "$t" = "$name" ]; then
            return 0
        fi
    done
    return 1
}

# Resolve unit_*.c for a module: #include "<mod>.h" in tests/unit, plus optional module-map.
unit_files_for_module() {
    mod="$1"
    found=""

    for u in tests/unit/unit_*.c; do
        [ -f "$u" ] || continue
        if grep -q "#include \"$mod.h\"" "$u" 2>/dev/null; then
            found="$found $u"
        fi
    done

    if [ -f "$MAP" ]; then
        line=$(grep "^${mod}:" "$MAP" 2>/dev/null | head -1)
        if [ -n "$line" ]; then
            set -- $(echo "$line" | sed 's/^[^:]*://')
            for u in "$@"; do
                case "$u" in
                    tests/unit/*) found="$found $u" ;;
                    unit_*.c) found="$found tests/unit/$u" ;;
                esac
            done
        fi
    fi

    found=$(echo "$found" | tr ' ' '\n' | sort -u | grep -v '^$' || true)
    echo "$found"
}

unit_touched_for_module() {
    mod="$1"
    units=$(unit_files_for_module "$mod")
    if [ -z "$units" ]; then
        return 1
    fi
    for u in $units; do
        if echo "$NAME_ONLY" | grep -qx "$u"; then
            if file_changed_non_whitespace "$u"; then
                return 0
            fi
        fi
    done
    return 1
}

snapshot_coverage_touched() {
    if makefile_touches_snapshot_tests; then
        return 0
    fi
    for path in $NAME_ONLY; do
        case "$path" in
            tests/regression/*.input|tests/regression/*.expect)
                if file_changed_non_whitespace "$path"; then
                    return 0
                fi
                ;;
        esac
    done
    return 1
}

# --- Heuristic 1: in-scope header without unit update ---
for path in $NAME_ONLY; do
    case "$path" in
        include/*.h|src/*.h|tests/harness/*.h)
            ;;
        *)
            continue
            ;;
    esac
    if ! file_changed_non_whitespace "$path"; then
        continue
    fi
    mod=$(module_from_header "$path")
    if ! module_in_coverage "$mod"; then
        continue
    fi
    units=$(unit_files_for_module "$mod")
    if [ -z "$units" ]; then
        continue
    fi
    if unit_touched_for_module "$mod"; then
        continue
    fi
    echo "test-gap: unit gap: $path changed without matching unit test update ($units)" >&2
    FAIL=1
done

# --- Heuristic 2: coverage-module .c without unit update ---
for mod in $COVERAGE_MODULES; do
    src=$(src_path_for_module "$mod")
    if ! echo "$NAME_ONLY" | grep -qx "$src"; then
        continue
    fi
    if ! file_changed_non_whitespace "$src"; then
        continue
    fi
    units=$(unit_files_for_module "$mod")
    if [ -z "$units" ]; then
        continue
    fi
    if unit_touched_for_module "$mod"; then
        continue
    fi
    echo "test-gap: unit gap: $src changed without tests/unit update (expect $units)" >&2
    FAIL=1
done

# --- Heuristic 3: UNIT_GAMEPLAY_SRC + HARNESS_SRC paths without snapshot touch ---
player_changed=0
for p in $PLAYER_PATHS; do
    if echo "$NAME_ONLY" | grep -qx "$p"; then
        if file_changed_non_whitespace "$p"; then
            player_changed=1
            break
        fi
    fi
done

if [ "$player_changed" = "1" ]; then
    if ! snapshot_coverage_touched; then
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
    if ! path_added_in_diff "$path"; then
        continue
    fi
    name=$(basename "$path" .input)
    if ! snapshot_listed "$name"; then
        echo "test-gap: snapshot gap: new $path not listed in Makefile SNAPSHOT_TESTS" >&2
        FAIL=1
    fi
done

# --- Warn only (exit 0): game.c without a touched game.h suite file ---
if echo "$NAME_ONLY" | grep -qx 'src/game.c'; then
    if file_changed_non_whitespace 'src/game.c'; then
        if ! unit_touched_for_module game; then
            echo "test-gap: note: src/game.c changed without tests/unit suite including game.h updated (ok if static router only)" >&2
        fi
    fi
fi

if [ "$FAIL" -ne 0 ]; then
    echo "test-gap: fail (add or update unit/snapshot tests; see docs/testing.md)" >&2
    if [ "$TEST_GAP_INFORMATIVE" = "1" ]; then
        echo "test-gap: informative mode (TEST_GAP_INFORMATIVE=1); exiting 0 for CI" >&2
        exit 0
    fi
    exit 1
fi

echo "test-gap: pass"
exit 0
