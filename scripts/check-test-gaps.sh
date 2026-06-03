#!/bin/sh
# Compare branch diff vs base for missing unit/snapshot test updates.
# Module lists: COVERAGE_MODULES, UNIT_GAMEPLAY_SRC, PLAT_SRC, HARNESS_SRC from Makefile.
# Unit suites: owning tests/unit/unit_*.c per tests/unit/module-map.

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
    COVERAGE_MODULES=$(sed -n '/^COVERAGE_MODULES =/,/^$/p' "$mf" \
        | sed '1s/^COVERAGE_MODULES = //' \
        | tr '\t\\' '  ' \
        | tr ' ' '\n' \
        | grep -v '^$' \
        | tr '\n' ' ')
    gameplay=$(sed -n '/^UNIT_GAMEPLAY_SRC =/,/^UNIT_CORE_SRC =/p' "$mf" \
        | grep -oE 'src/[a-zA-Z0-9_]+\.c|tests/harness/[a-zA-Z0-9_]+\.c' \
        | sort -u)
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
    UNIT_TEST_SRC_PATHS=$(sed -n '/^UNIT_TEST_SRC =/,/^UNIT_CORE_OBJS =/p' "$mf" \
        | grep -oE 'unit_[a-zA-Z0-9_]+\.c' \
        | sed 's|^|tests/unit/|' \
        | sort -u \
        | tr '\n' ' ')
    entrypoints="src/main.c src/platdos.c src/platpos.c src/platwin.c"
    PLAYER_PATHS=$(printf '%s\n%s\n%s\n' "$gameplay" "$harness" "$entrypoints" | sort -u | grep -v '^$' | tr '\n' ' ')
}

read_makefile_lists
cd "$ROOT"

MERGE_BASE=$(git merge-base "$BASE" HEAD 2>/dev/null) || {
    echo "test-gap: error: no merge base between $BASE and HEAD (fetch full history or use a related base ref)" >&2
    exit 1
}
BASE_COVERAGE_MODULES=$(git show "${MERGE_BASE}:Makefile" 2>/dev/null \
    | sed -n '/^COVERAGE_MODULES =/,/^$/p' \
    | sed '1s/^COVERAGE_MODULES = //' \
    | tr '\t\\' '  ' \
    | tr ' ' '\n' \
    | grep -v '^$' \
    | sort \
    | tr '\n' ' ')
COVERAGE_MODULES_ALL=$(printf '%s\n%s\n' "$BASE_COVERAGE_MODULES" "$COVERAGE_MODULES" \
    | tr ' ' '\n' | sort -u | grep -v '^$' | tr '\n' ' ')
DIFF_RANGE="${MERGE_BASE}...HEAD"
COMMITTED=$(git diff --name-only "$DIFF_RANGE" 2>/dev/null || true)
UNCOMMITTED=$(git diff --name-only HEAD 2>/dev/null || true)
NAME_ONLY=$(printf '%s\n%s' "$COMMITTED" "$UNCOMMITTED" | sort -u | grep -v '^$' || true)

if [ -z "$NAME_ONLY" ]; then
    echo "test-gap: pass (no diff vs $BASE)"
    exit 0
fi

snapshot_tests_sorted_from_stream() {
    sed -n '/^SNAPSHOT_TESTS =/,/^$/p' \
        | sed '1d;$d' \
        | tr '\t\\' '  ' \
        | tr ' ' '\n' \
        | grep -v '^$' \
        | sort \
        | tr '\n' ' '
}

snapshot_tests_sorted_at_ref() {
    ref="$1"
    if [ "$ref" = "worktree" ]; then
        snapshot_tests_sorted_from_stream < "$ROOT/Makefile"
    else
        git show "${ref}:Makefile" 2>/dev/null | snapshot_tests_sorted_from_stream
    fi
}

unit_test_src_sorted_from_stream() {
    sed -n '/^UNIT_TEST_SRC =/,/^UNIT_CORE_OBJS =/p' \
        | grep -oE 'unit_[a-zA-Z0-9_]+\.c' \
        | sed 's|^|tests/unit/|' \
        | sort \
        | tr '\n' ' '
}

unit_test_src_sorted_at_ref() {
    ref="$1"
    if [ "$ref" = "worktree" ]; then
        unit_test_src_sorted_from_stream < "$ROOT/Makefile"
    else
        git show "${ref}:Makefile" 2>/dev/null | unit_test_src_sorted_from_stream
    fi
}

makefile_touches_snapshot_tests() {
    if git diff "$DIFF_RANGE" -- Makefile 2>/dev/null | grep -q '^[+-].*SNAPSHOT_TESTS'; then
        return 0
    fi
    if git diff HEAD -- Makefile 2>/dev/null | grep -q '^[+-].*SNAPSHOT_TESTS'; then
        return 0
    fi
    base_list=$(snapshot_tests_sorted_at_ref "$MERGE_BASE")
    head_list=$(snapshot_tests_sorted_at_ref "HEAD")
    work_list=$(snapshot_tests_sorted_at_ref "worktree")
    if [ "$base_list" != "$head_list" ]; then
        return 0
    fi
    if [ "$head_list" != "$work_list" ]; then
        return 0
    fi
    return 1
}

coverage_modules_sorted_from_stream() {
    sed -n '/^COVERAGE_MODULES =/,/^$/p' \
        | sed '1s/^COVERAGE_MODULES = //' \
        | tr '\t\\' '  ' \
        | tr ' ' '\n' \
        | grep -v '^$' \
        | sort \
        | tr '\n' ' '
}

coverage_modules_sorted_at_ref() {
    ref="$1"
    if [ "$ref" = "worktree" ]; then
        coverage_modules_sorted_from_stream < "$ROOT/Makefile"
    else
        git show "${ref}:Makefile" 2>/dev/null | coverage_modules_sorted_from_stream
    fi
}

makefile_touches_coverage_modules() {
    if git diff "$DIFF_RANGE" -- Makefile 2>/dev/null | grep -q '^[+-].*COVERAGE_MODULES'; then
        return 0
    fi
    if git diff HEAD -- Makefile 2>/dev/null | grep -q '^[+-].*COVERAGE_MODULES'; then
        return 0
    fi
    base_list=$(coverage_modules_sorted_at_ref "$MERGE_BASE")
    head_list=$(coverage_modules_sorted_at_ref "HEAD")
    work_list=$(coverage_modules_sorted_at_ref "worktree")
    if [ "$base_list" != "$head_list" ]; then
        return 0
    fi
    if [ "$head_list" != "$work_list" ]; then
        return 0
    fi
    return 1
}

makefile_touches_unit_test_src() {
    if git diff "$DIFF_RANGE" -- Makefile 2>/dev/null | grep -q '^[+-].*UNIT_TEST_SRC'; then
        return 0
    fi
    if git diff HEAD -- Makefile 2>/dev/null | grep -q '^[+-].*UNIT_TEST_SRC'; then
        return 0
    fi
    base_list=$(unit_test_src_sorted_at_ref "$MERGE_BASE")
    head_list=$(unit_test_src_sorted_at_ref "HEAD")
    work_list=$(unit_test_src_sorted_at_ref "worktree")
    if [ "$base_list" != "$head_list" ]; then
        return 0
    fi
    if [ "$head_list" != "$work_list" ]; then
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
            if makefile_touches_snapshot_tests || makefile_touches_coverage_modules \
                || makefile_touches_unit_test_src; then
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

module_in_coverage_scope() {
    mod="$1"
    for m in $COVERAGE_MODULES_ALL; do
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

module_for_basename() {
    base="$1"
    case "$base" in
        th_world) echo world ;;
        *) echo "$base" ;;
    esac
}

module_for_source_path() {
    path="$1"
    case "$path" in
        tests/harness/testharn.c) echo testharn ;;
        tests/harness/th_world.c) echo world ;;
        *)
            module_for_basename "$(basename "$path" .c)"
            ;;
    esac
}

module_from_header() {
    module_for_basename "$(basename "$1" .h)"
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

# Resolve owning unit_*.c for a module from tests/unit/module-map (UNIT_TEST_SRC only).
unit_suite_in_build() {
    u="$1"
    for b in $UNIT_TEST_SRC_PATHS; do
        if [ "$b" = "$u" ]; then
            return 0
        fi
    done
    return 1
}

unit_files_for_module() {
    mod="$1"
    found=""

    if [ ! -f "$MAP" ]; then
        echo "$found"
        return
    fi

    line=$(grep "^${mod}:" "$MAP" 2>/dev/null | head -1)
    if [ -n "$line" ]; then
        set -- $(echo "$line" | sed 's/^[^:]*://')
        for u in "$@"; do
            case "$u" in
                tests/unit/*)
                    if unit_suite_in_build "$u"; then
                        found="$found $u"
                    fi
                    ;;
                unit_*.c)
                    full="tests/unit/$u"
                    if unit_suite_in_build "$full"; then
                        found="$found $full"
                    fi
                    ;;
            esac
        done
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

unit_tests_touched() {
    for path in $NAME_ONLY; do
        case "$path" in
            tests/unit/unit_*.c)
                if file_changed_non_whitespace "$path"; then
                    return 0
                fi
                ;;
        esac
    done
    return 1
}

# Every COVERAGE_MODULES entry must have an owning suite in module-map.
verify_module_map_coverage() {
    for mod in $COVERAGE_MODULES; do
        if [ ! -f "$MAP" ]; then
            echo "test-gap: config gap: missing $MAP" >&2
            FAIL=1
            return
        fi
        line=$(grep "^${mod}:" "$MAP" 2>/dev/null | head -1)
        if [ -z "$line" ]; then
            echo "test-gap: config gap: COVERAGE_MODULES lists $mod but $MAP has no owning suite (add $mod: unit_*.c)" >&2
            FAIL=1
            continue
        fi
        set -- $(echo "$line" | sed 's/^[^:]*://')
        for u in "$@"; do
            case "$u" in
                tests/unit/*) full="$u" ;;
                unit_*.c) full="tests/unit/$u" ;;
                *) continue ;;
            esac
            if ! unit_suite_in_build "$full"; then
                echo "test-gap: config gap: $MAP lists $full for $mod but it is not in Makefile UNIT_TEST_SRC" >&2
                FAIL=1
            fi
        done
        units=$(unit_files_for_module "$mod")
        if [ -z "$units" ]; then
            echo "test-gap: config gap: COVERAGE_MODULES lists $mod but no UNIT_TEST_SRC suite is mapped in $MAP" >&2
            FAIL=1
        fi
    done
}

# --- Heuristic 1: in-scope header without unit update ---
verify_module_map_coverage
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
    if ! module_in_coverage_scope "$mod"; then
        continue
    fi
    units=$(unit_files_for_module "$mod")
    if [ -z "$units" ]; then
        echo "test-gap: unit gap: $path changed but module-map has no suite for $mod (add $mod: unit_*.c to $MAP)" >&2
        FAIL=1
        continue
    fi
    if unit_touched_for_module "$mod"; then
        continue
    fi
    echo "test-gap: unit gap: $path changed without matching unit test update ($units)" >&2
    FAIL=1
done

# --- Shared config header: gameplay constants drive unit and snapshot behavior ---
if echo "$NAME_ONLY" | grep -qx 'include/config.h'; then
    if file_changed_non_whitespace 'include/config.h'; then
        if ! unit_tests_touched && ! snapshot_coverage_touched; then
            echo "test-gap: test gap: include/config.h changed without tests/unit or tests/regression update" >&2
            FAIL=1
        fi
    fi
fi

# --- Heuristic 2: coverage-module .c without unit update ---
check_coverage_source_unit_gap() {
    src="$1"
    mod="$2"
    if ! echo "$NAME_ONLY" | grep -qx "$src"; then
        return 0
    fi
    if ! file_changed_non_whitespace "$src"; then
        return 0
    fi
    units=$(unit_files_for_module "$mod")
    if [ -z "$units" ]; then
        echo "test-gap: unit gap: $src changed but module-map has no suite for $mod (add $mod: unit_*.c to $MAP)" >&2
        FAIL=1
        return 0
    fi
    if unit_touched_for_module "$mod"; then
        return 0
    fi
    if [ "$mod" = "game" ] && [ "$src" = "src/game.c" ]; then
        echo "test-gap: note: src/game.c changed without tests/unit suite including game.h updated (ok if static router only)" >&2
        return 0
    fi
    echo "test-gap: unit gap: $src changed without tests/unit update (expect $units)" >&2
    FAIL=1
}

for mod in $COVERAGE_MODULES_ALL; do
    check_coverage_source_unit_gap "$(src_path_for_module "$mod")" "$mod"
done

for hp in $HARNESS_SRC_PATHS; do
    mod=$(module_for_source_path "$hp")
    base=$(basename "$hp" .c)
    if [ "$mod" = "$base" ]; then
        continue
    fi
    check_coverage_source_unit_gap "$hp" "$mod"
done

# --- Heuristic 3: player paths without snapshot touch (unit-only changes ok) ---
player_changed=0
player_needs_snapshot=0
for p in $PLAYER_PATHS; do
    if ! echo "$NAME_ONLY" | grep -qx "$p"; then
        continue
    fi
    if ! file_changed_non_whitespace "$p"; then
        continue
    fi
    if [ "$p" = "src/game.c" ]; then
        continue
    fi
    player_changed=1
    mod=$(module_for_source_path "$p")
    if module_in_coverage "$mod"; then
        if ! unit_touched_for_module "$mod"; then
            player_needs_snapshot=1
        fi
    else
        player_needs_snapshot=1
    fi
done

if [ "$player_changed" = "1" ] && [ "$player_needs_snapshot" = "1" ]; then
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
