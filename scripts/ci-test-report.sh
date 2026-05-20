#!/bin/sh
# Run CI tests, write ci-test-report.md for PR comments. Exit 1 if any step fails.

REPORT=ci-test-report.md
LOG=ci-test.log
failed=0

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
        echo "| Step | Result |"
        echo "|------|--------|"
    } > "$REPORT"
}

run_step() {
    name="$1"
    shift
    echo "=== $name ===" >> "$LOG"
    if "$@" >> "$LOG" 2>&1; then
        echo "| $name | pass |" >> "$REPORT"
        return 0
    fi
    echo "| $name | **fail** |" >> "$REPORT"
    failed=1
    return 1
}

append_snapshots_row() {
    line=$(grep 'snapshot tests passed:' "$LOG" | tail -1)
    if [ -n "$line" ]; then
        echo "| snapshots | pass ($line) |" >> "$REPORT"
    else
        echo "| snapshots | **fail** |" >> "$REPORT"
        failed=1
    fi
}

append_unit_row() {
    total=$(grep '^Total:' "$LOG" | tail -1)
    passline=$(grep '^Pass:' "$LOG" | tail -1)
    if [ -n "$total" ] && [ -n "$passline" ]; then
        echo "| unit tests | pass ($total; $passline) |" >> "$REPORT"
    else
        echo "| unit tests | **fail** |" >> "$REPORT"
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
    if ! grep -q '^unit coverage' "$LOG"; then
        echo "| unit coverage | **fail** |" >> "$REPORT"
        failed=1
        return 1
    fi
    echo "| unit coverage | pass |" >> "$REPORT"
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

write_header
: > "$LOG"

run_step "check-layers" make check-layers || true
run_step "build (TEST_MODE)" make test || true

echo "=== snapshots ===" >> "$LOG"
if make test-run >> "$LOG" 2>&1; then
    append_snapshots_row
else
    echo "| snapshots | **fail** |" >> "$REPORT"
    failed=1
fi

echo "=== unit tests ===" >> "$LOG"
if make test-unit >> "$LOG" 2>&1; then
    append_unit_row
else
    echo "| unit tests | **fail** |" >> "$REPORT"
    failed=1
fi

echo "=== unit coverage ===" >> "$LOG"
if make test-unit-coverage >> "$LOG" 2>&1; then
    append_coverage_section || true
else
    echo "| unit coverage | **fail** |" >> "$REPORT"
    failed=1
fi

run_step "soak tests" make test-soak || true
append_soak_benchmark_section || true

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
