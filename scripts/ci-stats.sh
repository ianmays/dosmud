#!/bin/sh
# Run CI tests, collect timings and soak benchmarks, and write report artifacts.

REPORT=ci-test-report.md
STATS_MD=ci-stats.md
STATS_JSON=ci-stats.json
LOG=ci-stats.log
STEP_ROWS=ci-stats.steps
BENCH_ROWS=ci-stats.bench
failed=0

sha_short() {
    if [ -n "${GITHUB_SHA:-}" ]; then
        echo "$GITHUB_SHA" | cut -c1-7
        return 0
    fi
    git rev-parse --short=7 HEAD 2>/dev/null || echo local
}

ref_name() {
    if [ -n "${GITHUB_HEAD_REF:-}" ]; then
        echo "$GITHUB_HEAD_REF"
        return 0
    fi
    if [ -n "${GITHUB_REF_NAME:-}" ]; then
        echo "$GITHUB_REF_NAME"
        return 0
    fi
    git branch --show-current 2>/dev/null || echo local
}

timestamp_utc() {
    date -u +%Y-%m-%dT%H:%M:%SZ
}

json_escape() {
    printf '%s' "$1" | sed 's/\\/\\\\/g; s/"/\\"/g'
}

write_header() {
    ref=$(ref_name)
    sha=$(sha_short)
    {
        echo "<!-- ci-test-results -->"
        echo ""
        echo "## CI test results"
        echo ""
        echo "**Ref:** \`$ref\` @ \`$sha\`"
        echo ""
        echo "| Step | Result |"
        echo "|------|--------|"
    } > "$REPORT"
}

write_stats_header() {
    ref=$(ref_name)
    sha=$(sha_short)
    {
        echo "## CI stats"
        echo ""
        echo "**Ref:** \`$ref\` @ \`$sha\`"
    } > "$STATS_MD"
}

run_step() {
    name="$1"
    shift
    start=$(date +%s)
    echo "=== $name ===" >> "$LOG"
    if "$@" >> "$LOG" 2>&1; then
        status=pass
    else
        status=fail
        failed=1
    fi
    end=$(date +%s)
    elapsed=$((end - start))
    printf '%s|%s|%s\n' "$name" "$status" "$elapsed" >> "$STEP_ROWS"
    STEP_STATUS="$status"
    return 0
}

append_snapshot_summary() {
    line=$(grep 'snapshot tests passed:' "$LOG" | tail -1)
    if [ -n "$line" ]; then
        {
            echo ""
            echo "### Snapshot summary"
            echo ""
            echo "$line"
        } >> "$REPORT"
        return 0
    fi
    return 1
}

append_unit_summary() {
    total=$(grep '^Total:' "$LOG" | tail -1)
    passline=$(grep '^Pass:' "$LOG" | tail -1)
    if [ -n "$total" ] && [ -n "$passline" ]; then
        {
            echo ""
            echo "### Unit summary"
            echo ""
            echo "- $total"
            echo "- $passline"
        } >> "$REPORT"
        return 0
    fi
    return 1
}

append_coverage_section() {
    out="$1"
    if ! grep -q '^unit coverage' "$LOG"; then
        return 1
    fi
    {
        echo ""
        echo "### Unit coverage (branch % / line %)"
        echo ""
        echo '```'
        awk '
            /^unit coverage/ { keep = 1 }
            /^=== soak tests ===$/ { exit }
            keep { print }
        ' "$LOG"
        echo '```'
    } >> "$out"
    return 0
}

capture_soak_benchmarks() {
    if ! grep -q 'SOAK_BENCH ' "$LOG"; then
        return 1
    fi
    grep 'SOAK_BENCH ' "$LOG" | while read -r line; do
        name=$(printf '%s\n' "$line" | sed -n 's/^.*SOAK_BENCH \([^ ]*\).*/\1/p')
        ticks=$(printf '%s\n' "$line" | sed -n 's/.*ticks=\([0-9]*\).*/\1/p')
        us=$(printf '%s\n' "$line" | sed -n 's/.*us_per_tick=\([0-9]*\).*/\1/p')
        limit=$(printf '%s\n' "$line" | sed -n 's/.*limit=\([0-9]*\).*/\1/p')
        if [ -z "$limit" ]; then
            limit="?"
        fi
        if [ -z "$ticks" ]; then
            ticks="?"
        fi
        printf '%s|%s|%s|%s\n' "$name" "$ticks" "$us" "$limit" >> "$BENCH_ROWS"
    done
    return 0
}

append_soak_benchmarks() {
    out="$1"
    if [ ! -s "$BENCH_ROWS" ]; then
        return 1
    fi
    {
        echo ""
        echo "### Soak benchmarks (us per tick or round)"
        echo ""
        echo "| Scenario | measured | limit |"
        echo "|----------|----------|-------|"
    } >> "$out"
    while IFS='|' read -r name ticks us limit; do
        if [ -z "$name" ]; then
            continue
        fi
        echo "| $name | ${us} us | <= $limit |" >> "$out"
    done < "$BENCH_ROWS"
    return 0
}

write_step_table() {
    out="$1"
    include_seconds="$2"
    total_seconds=0
    if [ "$include_seconds" -eq 1 ]; then
        {
            echo ""
            echo "| Step | Result | Seconds |"
            echo "|------|--------|---------|"
        } >> "$out"
    fi
    while IFS='|' read -r name status seconds; do
        if [ -z "$name" ]; then
            continue
        fi
        total_seconds=$((total_seconds + seconds))
        if [ "$include_seconds" -eq 1 ]; then
            echo "| $name | $status | $seconds |" >> "$out"
        else
            echo "| $name | $status |" >> "$out"
        fi
    done < "$STEP_ROWS"
    if [ "$include_seconds" -eq 1 ]; then
        if [ "$failed" -eq 0 ]; then
            pipeline_result=pass
        else
            pipeline_result=fail
        fi
        echo "| total | $pipeline_result | $total_seconds |" >> "$out"
    fi
}

write_report() {
    write_header
    write_step_table "$REPORT" 0
    append_snapshot_summary || true
    append_unit_summary || true
    append_coverage_section "$REPORT" || true
    append_soak_benchmarks "$REPORT" || true
    if [ "$failed" -ne 0 ]; then
        {
            echo ""
            echo "### Log excerpt"
            echo ""
            echo '```'
            tail -50 "$LOG"
            echo '```'
        } >> "$REPORT"
    fi
}

write_stats() {
    write_stats_header
    write_step_table "$STATS_MD" 1
    append_coverage_section "$STATS_MD" || true
    append_soak_benchmarks "$STATS_MD" || true
    if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
        cat "$STATS_MD" >> "$GITHUB_STEP_SUMMARY"
    fi
}

write_json() {
    ref=$(ref_name)
    sha=$(sha_short)
    generated=$(timestamp_utc)
    total_seconds=0
    {
        echo "{"
        printf '  "ref": "%s",\n' "$(json_escape "$ref")"
        printf '  "sha": "%s",\n' "$(json_escape "$sha")"
        printf '  "generated_at": "%s",\n' "$(json_escape "$generated")"
        if [ "$failed" -eq 0 ]; then
            result="pass"
        else
            result="fail"
        fi
        printf '  "result": "%s",\n' "$(json_escape "$result")"
        echo '  "steps": ['
        first=1
        while IFS='|' read -r name status seconds; do
            if [ -z "$name" ]; then
                continue
            fi
            total_seconds=$((total_seconds + seconds))
            if [ "$first" -eq 0 ]; then
                echo ","
            fi
            first=0
            printf '    {"name":"%s","result":"%s","seconds":%s}' \
                "$(json_escape "$name")" "$(json_escape "$status")" "$seconds"
        done < "$STEP_ROWS"
        echo
        echo '  ],'
        printf '  "total_seconds": %s,\n' "$total_seconds"
        echo '  "benchmarks": ['
        first=1
        if [ -s "$BENCH_ROWS" ]; then
            while IFS='|' read -r name ticks us limit; do
                if [ -z "$name" ]; then
                    continue
                fi
                if [ "$first" -eq 0 ]; then
                    echo ","
                fi
                first=0
                printf '    {"name":"%s","ticks":%s,"us_per_tick":%s,"limit":%s}' \
                    "$(json_escape "$name")" "$ticks" "$us" "$limit"
            done < "$BENCH_ROWS"
            echo
        else
            echo
        fi
        echo '  ]'
        echo "}"
    } > "$STATS_JSON"
}

cleanup() {
    rm -f "$STEP_ROWS" "$BENCH_ROWS"
}

trap cleanup EXIT HUP INT TERM

: > "$LOG"
: > "$STEP_ROWS"
: > "$BENCH_ROWS"

run_step "check-layers" make check-layers || true
run_step "build (TEST_MODE)" make test || true

run_step "snapshots" make test-run || true
run_step "unit tests" make test-unit || true
run_step "unit coverage" make test-unit-coverage || true
run_step "soak tests" make test-soak || true
capture_soak_benchmarks || true

write_report
write_stats
write_json

if [ "$failed" -ne 0 ]; then
    exit 1
fi

exit 0
