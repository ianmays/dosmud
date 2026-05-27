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
snapshot_pass_count=0
snapshot_fail_count=0
snapshot_skip_count=0
snapshot_summary_line=
unit_pass_count=0
unit_fail_count=0
unit_skip_count=0
soak_pass_count=0
soak_fail_count=0
soak_skip_count=0
coverage_overall_branch=
coverage_overall_line=

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

json_number_or_null() {
    value="$1"
    if [ -n "$value" ]; then
        printf '%s' "$value"
    else
        printf 'null'
    fi
}

display_number_or_na() {
    value="$1"
    suffix="$2"
    if [ -n "$value" ]; then
        printf '%s%s' "$value" "$suffix"
    else
        printf 'n/a'
    fi
}

parse_snapshot_counts() {
    snapshot_summary_line=$(grep 'snapshot tests passed:' "$LOG" | tail -1)
    if [ -z "$snapshot_summary_line" ]; then
        return 1
    fi
    passed=$(printf '%s\n' "$snapshot_summary_line" | sed -n 's/.*snapshot tests passed: \([0-9][0-9]*\)\/\([0-9][0-9]*\).*/\1/p')
    total=$(printf '%s\n' "$snapshot_summary_line" | sed -n 's/.*snapshot tests passed: \([0-9][0-9]*\)\/\([0-9][0-9]*\).*/\2/p')
    if [ -z "$passed" ] || [ -z "$total" ]; then
        return 1
    fi
    snapshot_pass_count=$passed
    snapshot_fail_count=$((total - passed))
    snapshot_skip_count=0
    return 0
}

parse_section_line() {
    section_name="$1"
    line_re="$2"
    awk -v heading="=== $section_name ===" -v line_re="$line_re" '
        $0 == heading { in_section = 1; next }
        /^=== .* ===$/ && in_section { exit }
        in_section && $0 ~ line_re { line = $0 }
        END { if (line) print line }
    ' "$LOG"
}

parse_section_counts() {
    section_name="$1"
    passline=$(parse_section_line "$section_name" '^Pass: ')
    if [ -z "$passline" ]; then
        return 1
    fi
    passcount=$(printf '%s\n' "$passline" | sed -n 's/^Pass: \([0-9][0-9]*\), fail: \([0-9][0-9]*\), skip: \([0-9][0-9]*\).*/\1/p')
    failcount=$(printf '%s\n' "$passline" | sed -n 's/^Pass: \([0-9][0-9]*\), fail: \([0-9][0-9]*\), skip: \([0-9][0-9]*\).*/\2/p')
    skipcount=$(printf '%s\n' "$passline" | sed -n 's/^Pass: \([0-9][0-9]*\), fail: \([0-9][0-9]*\), skip: \([0-9][0-9]*\).*/\3/p')
    if [ -z "$passcount" ] || [ -z "$failcount" ] || [ -z "$skipcount" ]; then
        return 1
    fi
    printf '%s|%s|%s\n' "$passcount" "$failcount" "$skipcount"
    return 0
}

parse_unit_counts() {
    counts=$(parse_section_counts "unit tests") || return 1
    unit_pass_count=$(printf '%s\n' "$counts" | cut -d'|' -f1)
    unit_fail_count=$(printf '%s\n' "$counts" | cut -d'|' -f2)
    unit_skip_count=$(printf '%s\n' "$counts" | cut -d'|' -f3)
    return 0
}

parse_soak_counts() {
    counts=$(parse_section_counts "soak tests") || return 1
    soak_pass_count=$(printf '%s\n' "$counts" | cut -d'|' -f1)
    soak_fail_count=$(printf '%s\n' "$counts" | cut -d'|' -f2)
    soak_skip_count=$(printf '%s\n' "$counts" | cut -d'|' -f3)
    return 0
}

parse_coverage_overall() {
    line=$(parse_section_line "unit coverage" 'overall')
    if [ -z "$line" ]; then
        return 1
    fi
    branch_pct=$(printf '%s\n' "$line" | sed -n 's/^[[:space:]]*overall[[:space:]]*\([0-9][0-9]*\.[0-9][0-9]*\)[[:space:]]*\/[[:space:]]*\([0-9][0-9]*\.[0-9][0-9]*\).*/\1/p')
    line_pct=$(printf '%s\n' "$line" | sed -n 's/^[[:space:]]*overall[[:space:]]*\([0-9][0-9]*\.[0-9][0-9]*\)[[:space:]]*\/[[:space:]]*\([0-9][0-9]*\.[0-9][0-9]*\).*/\2/p')
    if [ -z "$branch_pct" ] || [ -z "$line_pct" ]; then
        return 1
    fi
    coverage_overall_branch=$branch_pct
    coverage_overall_line=$line_pct
    return 0
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
    if [ "$result" = "skip" ]; then
        snapshot_pass_count=0
        snapshot_fail_count=0
        snapshot_skip_count=1
        append_result_row "$REPORT" "snapshots" "not run (make test failed)" "$duration"
        record_step_row "snapshots" "skip" "-1"
        return 0
    fi
    if parse_snapshot_counts; then
        append_result_row "$REPORT" "snapshots" "pass ($snapshot_summary_line)" "$duration"
        return 0
    fi
    snapshot_pass_count=0
    snapshot_fail_count=1
    snapshot_skip_count=0
    append_result_row "$REPORT" "snapshots" "**fail**" "$duration"
    failed=1
    return 1
}

append_unit_row() {
    duration="$1"
    total=$(grep '^Total:' "$LOG" | tail -1)
    passline=$(grep '^Pass:' "$LOG" | tail -1)
    if [ -n "$total" ] && [ -n "$passline" ] && parse_unit_counts; then
        append_result_row "$REPORT" "unit tests" "pass ($total; $passline)" "$duration"
        return 0
    fi
    unit_pass_count=0
    unit_fail_count=1
    unit_skip_count=0
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
        measured=$(display_number_or_na "$us" " us")
        if [ -n "$limit" ]; then
            limit_display="<= $limit"
        else
            limit_display="n/a"
        fi
        echo "| $name | $measured | $limit_display |" >> "$out"
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
        echo '  "test_counts": {'
        printf '    "snapshots": {"pass":%s,"fail":%s,"skip":%s},\n' \
            "$snapshot_pass_count" "$snapshot_fail_count" "$snapshot_skip_count"
        printf '    "unit": {"pass":%s,"fail":%s,"skip":%s},\n' \
            "$unit_pass_count" "$unit_fail_count" "$unit_skip_count"
        printf '    "soak": {"pass":%s,"fail":%s,"skip":%s}\n' \
            "$soak_pass_count" "$soak_fail_count" "$soak_skip_count"
        echo '  },'
        if [ -n "$coverage_overall_branch" ] && [ -n "$coverage_overall_line" ]; then
            echo '  "unit_coverage_overall": {'
            printf '    "branch_pct": %s,\n' "$(json_number_or_null "$coverage_overall_branch")"
            printf '    "line_pct": %s\n' "$(json_number_or_null "$coverage_overall_line")"
            echo '  },'
        fi
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
                    "$(json_escape "$name")" \
                    "$(json_number_or_null "$ticks")" \
                    "$(json_number_or_null "$us")" \
                    "$(json_number_or_null "$limit")"
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
        if append_snapshots_row "run" "$duration"; then
            record_step_row "snapshots" "pass" "$last_duration"
        else
            record_step_row "snapshots" "fail" "$last_duration"
        fi
    else
        duration=$(format_duration "$last_duration")
        snapshot_pass_count=0
        snapshot_fail_count=1
        snapshot_skip_count=0
        append_result_row "$REPORT" "snapshots" "**fail**" "$duration"
        record_step_row "snapshots" "fail" "$last_duration"
        failed=1
    fi
else
    append_snapshots_row "skip" "-"
fi

if run_timed "unit tests" ./tests/unit/build/dosmud_unit; then
    duration=$(format_duration "$last_duration")
    if append_unit_row "$duration"; then
        record_step_row "unit tests" "pass" "$last_duration"
    else
        record_step_row "unit tests" "fail" "$last_duration"
    fi
else
    duration=$(format_duration "$last_duration")
    if ! parse_unit_counts; then
        unit_pass_count=0
        unit_fail_count=1
        unit_skip_count=0
    fi
    append_result_row "$REPORT" "unit tests" "**fail**" "$duration"
    record_step_row "unit tests" "fail" "$last_duration"
    failed=1
fi

if run_timed "unit coverage" make test-unit-coverage; then
    duration=$(format_duration "$last_duration")
    if parse_coverage_overall; then
        append_result_row "$REPORT" "unit coverage" "pass" "$duration"
        record_step_row "unit coverage" "pass" "$last_duration"
    else
        append_result_row "$REPORT" "unit coverage" "**fail**" "$duration"
        record_step_row "unit coverage" "fail" "$last_duration"
        failed=1
    fi
else
    duration=$(format_duration "$last_duration")
    append_result_row "$REPORT" "unit coverage" "**fail**" "$duration"
    record_step_row "unit coverage" "fail" "$last_duration"
    failed=1
fi

if run_timed "soak tests" ./tests/soak/build/dosmud_soak; then
    duration=$(format_duration "$last_duration")
    if parse_soak_counts; then
        append_result_row "$REPORT" "soak tests" "pass" "$duration"
        record_step_row "soak tests" "pass" "$last_duration"
    else
        soak_pass_count=0
        soak_fail_count=1
        soak_skip_count=0
        append_result_row "$REPORT" "soak tests" "**fail**" "$duration"
        record_step_row "soak tests" "fail" "$last_duration"
        failed=1
    fi
else
    duration=$(format_duration "$last_duration")
    if ! parse_soak_counts; then
        soak_pass_count=0
        soak_fail_count=1
        soak_skip_count=0
    fi
    append_result_row "$REPORT" "soak tests" "**fail**" "$duration"
    record_step_row "soak tests" "fail" "$last_duration"
    failed=1
fi
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
