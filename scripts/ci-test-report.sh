#!/bin/sh
# Run CI tests, write ci-test-report.md for PR comments. Exit 1 if any step fails.

REPORT=ci-test-report.md
LOG=ci-test.log
failed=0
build_test_duration=
build_unit_duration=
build_soak_duration=
last_duration=

sha_short() {
    if [ -n "${GITHUB_SHA}" ]; then
        echo "$GITHUB_SHA" | cut -c1-7
    else
        echo "local"
    fi
}

write_header() {
    ref="${GITHUB_HEAD_REF:-${GITHUB_REF_NAME:-local}}"
    {
        echo "## CI test results"
        echo ""
        echo "**Ref:** \`$ref\` @ \`$(sha_short)\`"
        echo ""
        echo "| Step | Result | Duration |"
        echo "|------|--------|----------|"
    } > "$REPORT"
}

now_millis() {
    date +%s%3N
}

format_duration() {
    ms="$1"
    sec=$((ms / 1000))
    rem=$((ms % 1000))
    printf '%s.%03ds\n' "$sec" "$rem"
}

run_timed() {
    name="$1"
    shift
    start=$(now_millis)
    echo "=== $name ===" >> "$LOG"
    if "$@" >> "$LOG" 2>&1; then
        status=0
    else
        status=$?
    fi
    end=$(now_millis)
    last_duration=$((end - start))
    return "$status"
}

append_result_row() {
    echo "| $1 | $2 | $3 |" >> "$REPORT"
}

run_step() {
    name="$1"
    shift
    if run_timed "$name" "$@"; then
        append_result_row "$name" "pass" "$(format_duration "$last_duration")"
        return 0
    fi
    append_result_row "$name" "**fail**" "$(format_duration "$last_duration")"
    failed=1
    return 1
}

append_snapshots_row() {
    duration="$1"
    line=$(grep 'snapshot tests passed:' "$LOG" | tail -1)
    if [ -n "$line" ]; then
        append_result_row "snapshots" "pass ($line)" "$duration"
    else
        append_result_row "snapshots" "**fail**" "$duration"
        failed=1
    fi
}

append_unit_row() {
    duration="$1"
    total=$(grep '^Total:' "$LOG" | tail -1)
    passline=$(grep '^Pass:' "$LOG" | tail -1)
    if [ -n "$total" ] && [ -n "$passline" ]; then
        append_result_row "unit tests" "pass ($total; $passline)" "$duration"
    else
        append_result_row "unit tests" "**fail**" "$duration"
        failed=1
    fi
}

append_soak_benchmark_section() {
    if ! grep -q 'SOAK_BENCH ' "$LOG"; then
        failed=1
        return 1
    fi
    {
        echo ""
        echo "### Soak benchmarks (us per tick or round)"
        echo ""
        echo "| Scenario | measured | limit |"
        echo "|----------|----------|-------|"
    } >> "$REPORT"
    grep 'SOAK_BENCH ' "$LOG" | while read -r line; do
        bench=$(echo "$line" | sed 's/^[^S]*//')
        name=$(echo "$bench" | sed -n 's/^SOAK_BENCH \([^ ]*\).*/\1/p')
        us=$(echo "$bench" | sed -n 's/.*us_per_tick=\([0-9]*\).*/\1/p')
        limit=$(echo "$bench" | sed -n 's/.*limit=\([0-9]*\).*/\1/p')
        if [ -z "$limit" ]; then
            limit="?"
        fi
        echo "| $name | ${us} us | <= $limit |" >> "$REPORT"
    done
    return 0
}

append_coverage_section() {
    duration="$1"
    if ! grep -q '^unit coverage' "$LOG"; then
        append_result_row "unit coverage" "**fail**" "$duration"
        failed=1
        return 1
    fi
    append_result_row "unit coverage" "pass" "$duration"
    {
        echo ""
        echo "### Unit coverage (branch % / line %)"
        echo ""
        echo '```'
        awk '/^unit coverage|^  / { print }' "$LOG" | sed '/^$/d'
        echo '```'
    } >> "$REPORT"
    return 0
}

append_build_timing_section() {
    {
        echo ""
        echo "### Build timings (wall-clock)"
        echo ""
        echo "| Build target | Duration |"
        echo "|--------------|----------|"
        echo "| make test | $(format_duration "$build_test_duration") |"
        echo "| make build-unit | $(format_duration "$build_unit_duration") |"
        echo "| make build-soak | $(format_duration "$build_soak_duration") |"
        echo ""
        echo "Build rows above cover compile plus link only. Test execution stays in the main results table."
    } >> "$REPORT"
}

write_header
: > "$LOG"

run_step "check-layers" make check-layers || true
if run_timed "build (TEST_MODE)" make test; then
    build_test_duration=$last_duration
    append_result_row "build (TEST_MODE)" "pass" "$(format_duration "$last_duration")"
else
    build_test_duration=$last_duration
    append_result_row "build (TEST_MODE)" "**fail**" "$(format_duration "$last_duration")"
    failed=1
fi

if run_timed "build unit binary" make build-unit; then
    build_unit_duration=$last_duration
    append_result_row "build unit binary" "pass" "$(format_duration "$last_duration")"
else
    build_unit_duration=$last_duration
    append_result_row "build unit binary" "**fail**" "$(format_duration "$last_duration")"
    failed=1
fi

if run_timed "build soak binary" make build-soak; then
    build_soak_duration=$last_duration
    append_result_row "build soak binary" "pass" "$(format_duration "$last_duration")"
else
    build_soak_duration=$last_duration
    append_result_row "build soak binary" "**fail**" "$(format_duration "$last_duration")"
    failed=1
fi

if run_timed "snapshots" make test-run; then
    append_snapshots_row "$(format_duration "$last_duration")"
else
    append_result_row "snapshots" "**fail**" "$(format_duration "$last_duration")"
    failed=1
fi

if run_timed "unit tests" ./tests/unit/build/dosmud_unit; then
    append_unit_row "$(format_duration "$last_duration")"
else
    append_result_row "unit tests" "**fail**" "$(format_duration "$last_duration")"
    failed=1
fi

if run_timed "unit coverage" make test-unit-coverage; then
    append_coverage_section "$(format_duration "$last_duration")" || true
else
    append_result_row "unit coverage" "**fail**" "$(format_duration "$last_duration")"
    failed=1
fi

run_step "soak tests" ./tests/soak/build/dosmud_soak || true
append_soak_benchmark_section || true
append_build_timing_section

if [ "$failed" -ne 0 ]; then
    {
        echo ""
        echo "### Log excerpt"
        echo ""
        echo '```'
        tail -50 "$LOG"
        echo '```'
    } >> "$REPORT"
    exit 1
fi

exit 0
