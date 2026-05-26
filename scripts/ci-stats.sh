#!/bin/sh
# Run CI tests, write ci-test-report.md for PR comments, and emit stats artifacts.

REPORT=ci-test-report.md
STATS_MD=ci-stats.md
STATS_JSON=ci-stats.json
LOG=ci-stats.log
BUILD_ROWS=ci-stats.build
STEP_ROWS=ci-stats.steps
BENCH_ROWS=ci-stats.bench
failed=0
build_release_duration=
build_test_duration=
build_unit_duration=
build_soak_duration=
last_duration=

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
        echo "| Step | Result | Duration |"
        echo "|------|--------|----------|"
    } > "$REPORT"
}

write_stats_header() {
    ref=$(ref_name)
    sha=$(sha_short)
    {
        echo "## CI stats"
        echo ""
        echo "**Ref:** \`$ref\` @ \`$sha\`"
        echo ""
        echo "| Step | Result | Duration |"
        echo "|------|--------|----------|"
    } > "$STATS_MD"
}

now_millis() {
    date +%s%3N
}

format_duration() {
    ms="$1"
    sec=$((ms / 1000))
    rem=$((ms % 1000))
    printf '%s.%03ds' "$sec" "$rem"
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
    out="$1"
    echo "| $2 | $3 | $4 |" >> "$out"
}

record_step_row() {
    printf '%s|%s|%s\n' "$1" "$2" "$3" >> "$STEP_ROWS"
}

record_build_row() {
    printf '%s|%s\n' "$1" "$2" >> "$BUILD_ROWS"
}

run_step() {
    name="$1"
    shift
    if run_timed "$name" "$@"; then
        duration=$(format_duration "$last_duration")
        append_result_row "$REPORT" "$name" "pass" "$duration"
        record_step_row "$name" "pass" "$last_duration"
        return 0
    fi
    duration=$(format_duration "$last_duration")
    append_result_row "$REPORT" "$name" "**fail**" "$duration"
    record_step_row "$name" "fail" "$last_duration"
    failed=1
    return 1
}

run_build_step() {
    duration_var="$1"
    name="$2"
    shift 2
    if run_timed "$name" "$@"; then
        duration=$(format_duration "$last_duration")
        append_result_row "$REPORT" "$name" "pass" "$duration"
        record_step_row "$name" "pass" "$last_duration"
        eval "$duration_var=\$last_duration"
    else
        duration=$(format_duration "$last_duration")
        append_result_row "$REPORT" "$name" "**fail**" "$duration"
        record_step_row "$name" "fail" "$last_duration"
        eval "$duration_var=\$last_duration"
        failed=1
    fi
    record_build_row "$name" "$(eval "printf '%s' \"\${$duration_var}\"")"
}

append_snapshots_row() {
    result="$1"
    duration="$2"
    line=$(grep 'snapshot tests passed:' "$LOG" | tail -1)
    if [ "$result" = "skip" ]; then
        append_result_row "$REPORT" "snapshots" "not run (make test failed)" "$duration"
        record_step_row "snapshots" "skip" "-1"
        return 0
    fi
    if [ -n "$line" ]; then
        append_result_row "$REPORT" "snapshots" "pass ($line)" "$duration"
        return 0
    fi
    append_result_row "$REPORT" "snapshots" "**fail**" "$duration"
    failed=1
    return 1
}

append_unit_row() {
    duration="$1"
    total=$(grep '^Total:' "$LOG" | tail -1)
    passline=$(grep '^Pass:' "$LOG" | tail -1)
    if [ -n "$total" ] && [ -n "$passline" ]; then
        append_result_row "$REPORT" "unit tests" "pass ($total; $passline)" "$duration"
        return 0
    fi
    append_result_row "$REPORT" "unit tests" "**fail**" "$duration"
    failed=1
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
        failed=1
        return 1
    fi
    grep 'SOAK_BENCH ' "$LOG" | while read -r line; do
        name=$(printf '%s\n' "$line" | sed -n 's/^.*SOAK_BENCH \([^ ]*\).*/\1/p')
        us=$(printf '%s\n' "$line" | sed -n 's/.*us_per_tick=\([0-9]*\).*/\1/p')
        limit=$(printf '%s\n' "$line" | sed -n 's/.*limit=\([0-9]*\).*/\1/p')
        ticks=$(printf '%s\n' "$line" | sed -n 's/.*ticks=\([0-9]*\).*/\1/p')
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

append_soak_benchmark_section() {
    out="$1"
    if [ ! -s "$BENCH_ROWS" ]; then
        failed=1
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

append_build_timing_section() {
    out="$1"
    {
        echo ""
        echo "### Build timings (wall-clock)"
        echo ""
        echo "| Build target | Duration |"
        echo "|--------------|----------|"
    } >> "$out"
    while IFS='|' read -r name elapsed; do
        if [ -z "$name" ]; then
            continue
        fi
        echo "| $name | $(format_duration "$elapsed") |" >> "$out"
    done < "$BUILD_ROWS"
    {
        echo ""
        echo "Build rows above cover compile plus link only. Test execution stays in the main results table."
    } >> "$out"
}

append_artifact_section() {
    out="$1"
    {
        echo ""
        echo "### Stats artifacts"
        echo ""
        echo "- \`ci-stats.json\` - machine-readable timings and soak benchmark data uploaded as the CI workflow artifact"
    } >> "$out"
}

write_stats_rows() {
    while IFS='|' read -r name result elapsed; do
        if [ -z "$name" ]; then
            continue
        fi
        if [ "$elapsed" = "-1" ]; then
            duration="-"
        else
            duration=$(format_duration "$elapsed")
        fi
        echo "| $name | $result | $duration |" >> "$STATS_MD"
    done < "$STEP_ROWS"
}

write_json() {
    ref=$(ref_name)
    sha=$(sha_short)
    generated=$(timestamp_utc)
    total_step_ms=0
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
        while IFS='|' read -r name status elapsed; do
            if [ -z "$name" ]; then
                continue
            fi
            if [ "$elapsed" -ge 0 ] 2>/dev/null; then
                total_step_ms=$((total_step_ms + elapsed))
            fi
            if [ "$first" -eq 0 ]; then
                echo ","
            fi
            first=0
            printf '    {"name":"%s","result":"%s","duration_ms":%s}' \
                "$(json_escape "$name")" "$(json_escape "$status")" "$elapsed"
        done < "$STEP_ROWS"
        echo
        echo '  ],'
        printf '  "total_step_duration_ms": %s,\n' "$total_step_ms"
        echo '  "build_timings": ['
        first=1
        while IFS='|' read -r name elapsed; do
            if [ -z "$name" ]; then
                continue
            fi
            if [ "$first" -eq 0 ]; then
                echo ","
            fi
            first=0
            printf '    {"name":"%s","duration_ms":%s}' \
                "$(json_escape "$name")" "$elapsed"
        done < "$BUILD_ROWS"
        echo
        echo '  ],'
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
    rm -f "$BUILD_ROWS" "$STEP_ROWS" "$BENCH_ROWS"
}

trap cleanup EXIT HUP INT TERM

write_header
write_stats_header
: > "$LOG"
: > "$BUILD_ROWS"
: > "$STEP_ROWS"
: > "$BENCH_ROWS"

run_step "check-layers" make check-layers

run_build_step build_release_duration "make build" make build
run_build_step build_test_duration "make test" make test
run_build_step build_unit_duration "make build-unit" make build-unit
run_build_step build_soak_duration "make build-soak" make build-soak

if [ -f ./dosmud ] && grep -q '| make test | pass |' "$REPORT"; then
    if run_timed "snapshots" make snapshot-run; then
        duration=$(format_duration "$last_duration")
        append_snapshots_row "run" "$duration"
        record_step_row "snapshots" "pass" "$last_duration"
    else
        duration=$(format_duration "$last_duration")
        append_result_row "$REPORT" "snapshots" "**fail**" "$duration"
        record_step_row "snapshots" "fail" "$last_duration"
        failed=1
    fi
else
    append_snapshots_row "skip" "-"
fi

if run_timed "unit tests" ./tests/unit/build/dosmud_unit; then
    duration=$(format_duration "$last_duration")
    append_unit_row "$duration"
    record_step_row "unit tests" "pass" "$last_duration"
else
    duration=$(format_duration "$last_duration")
    append_result_row "$REPORT" "unit tests" "**fail**" "$duration"
    record_step_row "unit tests" "fail" "$last_duration"
    failed=1
fi

if run_timed "unit coverage" make test-unit-coverage; then
    duration=$(format_duration "$last_duration")
    append_result_row "$REPORT" "unit coverage" "pass" "$duration"
    record_step_row "unit coverage" "pass" "$last_duration"
else
    duration=$(format_duration "$last_duration")
    append_result_row "$REPORT" "unit coverage" "**fail**" "$duration"
    record_step_row "unit coverage" "fail" "$last_duration"
    failed=1
fi

run_step "soak tests" ./tests/soak/build/dosmud_soak
capture_soak_benchmarks || true
append_coverage_section "$REPORT" || true
append_soak_benchmark_section "$REPORT" || true
append_build_timing_section "$REPORT"
append_artifact_section "$REPORT"

write_stats_rows
append_build_timing_section "$STATS_MD"
append_coverage_section "$STATS_MD" || true
append_soak_benchmark_section "$STATS_MD" || true
append_artifact_section "$STATS_MD"
write_json

if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
    cat "$STATS_MD" >> "$GITHUB_STEP_SUMMARY"
fi

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
