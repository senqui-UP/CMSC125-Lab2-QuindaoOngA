#!/usr/bin/env bash
# test_suite.sh — Automated test script
# Usage: bash tests/test_suite.sh   (or: make test)
#
# Each test either:
#   - checks exit code (crash/error-path tests)
#   - checks Avg TT and Avg WT extracted from output (metric tests)
#
# Tolerances: ±0.05 on floating-point averages

SCHEDSIM=./schedsim
PASS=0
FAIL=0
TOLERANCE=0.05

# ── helpers ───────────────────────────────────────────────────────────────────

pass() { echo "[PASS] $1"; ((PASS++)); }
fail() { echo "[FAIL] $1"; ((FAIL++)); }

# run_metric_test <desc> <expected_tt> <expected_wt> <args...>
# Runs schedsim, extracts "TT: X.XX | WT: Y.YY" from output, compares with tolerance.
run_metric_test() {
    local desc="$1"
    local exp_tt="$2"
    local exp_wt="$3"
    shift 3

    local output
    output=$("$SCHEDSIM" "$@" 2>/dev/null)
    local rc=$?

    if [[ $rc -ne 0 ]]; then
        fail "$desc (exit code $rc)"
        return
    fi

    local got_tt got_wt
    got_tt=$(echo "$output" | grep -oP 'TT:\s*\K[0-9]+\.[0-9]+' | head -1)
    got_wt=$(echo "$output" | grep -oP 'WT:\s*\K[0-9]+\.[0-9]+' | head -1)

    if [[ -z "$got_tt" || -z "$got_wt" ]]; then
        fail "$desc (could not parse TT/WT from output)"
        return
    fi

    # Compare with awk for portable floating-point arithmetic
    local ok
    ok=$(awk -v tt="$got_tt" -v ett="$exp_tt" \
              -v wt="$got_wt" -v ewt="$exp_wt" \
              -v tol="$TOLERANCE" \
         'BEGIN {
             tt_ok = (tt - ett < 0 ? ett - tt : tt - ett) <= tol
             wt_ok = (wt - ewt < 0 ? ewt - wt : wt - ewt) <= tol
             print (tt_ok && wt_ok) ? "1" : "0"
         }')

    if [[ "$ok" == "1" ]]; then
        pass "$desc (TT=$got_tt WT=$got_wt)"
    else
        fail "$desc — expected TT=$exp_tt WT=$exp_wt, got TT=$got_tt WT=$got_wt"
    fi
}

# run_exit_test <desc> <expected_exit_code> <args...>
# Verifies the binary exits with the expected code.
run_exit_test() {
    local desc="$1"
    local expected_exit="$2"
    shift 2
    "$SCHEDSIM" "$@" > /dev/null 2>&1
    local rc=$?
    if [[ $rc -eq $expected_exit ]]; then
        pass "$desc"
    else
        fail "$desc (expected exit $expected_exit, got $rc)"
    fi
}

# run_crash_test <desc> <args...>
# Passes as long as the binary exits 0 (runs without crashing).
run_crash_test() {
    local desc="$1"
    shift
    "$SCHEDSIM" "$@" > /dev/null 2>&1
    local rc=$?
    if [[ $rc -eq 0 ]]; then
        pass "$desc"
    else
        fail "$desc (crashed with exit $rc)"
    fi
}

# ── workload1.txt — known correct averages ────────────────────────────────────
echo ""
echo "=== workload1.txt — metric correctness ==="

run_metric_test "FCFS  workload1  TT=515.00 WT=359.00" \
    515.00 359.00 --algorithm FCFS --input tests/workload1.txt

run_metric_test "SJF   workload1  TT=461.00 WT=305.00" \
    461.00 305.00 --algorithm SJF  --input tests/workload1.txt

run_metric_test "STCF  workload1  TT=393.00 WT=237.00" \
    393.00 237.00 --algorithm STCF --input tests/workload1.txt

run_metric_test "RR    workload1  q=2  TT=630.20 WT=474.20" \
    630.20 474.20 --algorithm RR   --input tests/workload1.txt --quantum 2

run_metric_test "MLFQ  workload1  TT=664.60 WT=508.60" \
    664.60 508.60 --algorithm MLFQ --input tests/workload1.txt

# ── workload2.txt — idle gap handling ─────────────────────────────────────────
echo ""
echo "=== workload2.txt — idle gap handling ==="

run_metric_test "FCFS  workload2  TT=4.50 WT=0.75" \
    4.50 0.75 --algorithm FCFS --input tests/workload2.txt

run_metric_test "SJF   workload2  TT=4.50 WT=0.75" \
    4.50 0.75 --algorithm SJF  --input tests/workload2.txt

run_metric_test "STCF  workload2  TT=4.25 WT=0.50" \
    4.25 0.50 --algorithm STCF --input tests/workload2.txt

run_metric_test "RR    workload2  q=2  TT=4.50 WT=0.75" \
    4.50 0.75 --algorithm RR   --input tests/workload2.txt --quantum 2

run_metric_test "MLFQ  workload2  TT=4.50 WT=0.75" \
    4.50 0.75 --algorithm MLFQ --input tests/workload2.txt

# ── single.txt — single-process edge case ─────────────────────────────────────
echo ""
echo "=== single.txt — single-process edge case ==="

run_metric_test "FCFS  single  TT=10.00 WT=0.00" \
    10.00 0.00 --algorithm FCFS --input tests/single.txt

run_metric_test "SJF   single  TT=10.00 WT=0.00" \
    10.00 0.00 --algorithm SJF  --input tests/single.txt

run_metric_test "STCF  single  TT=10.00 WT=0.00" \
    10.00 0.00 --algorithm STCF --input tests/single.txt

run_metric_test "RR    single  q=2  TT=10.00 WT=0.00" \
    10.00 0.00 --algorithm RR   --input tests/single.txt --quantum 2

run_metric_test "MLFQ  single  TT=10.00 WT=0.00" \
    10.00 0.00 --algorithm MLFQ --input tests/single.txt

# ── simultaneous.txt — all arrive at t=0 ──────────────────────────────────────
echo ""
echo "=== simultaneous.txt — all arrive at t=0 ==="

run_metric_test "FCFS  simultaneous  TT=15.80 WT=10.80" \
    15.80 10.80 --algorithm FCFS --input tests/simultaneous.txt

run_metric_test "SJF   simultaneous  TT=12.20 WT=7.20" \
    12.20  7.20 --algorithm SJF  --input tests/simultaneous.txt

run_metric_test "STCF  simultaneous  TT=12.20 WT=7.20" \
    12.20  7.20 --algorithm STCF --input tests/simultaneous.txt

run_metric_test "RR    simultaneous  q=2  TT=18.00 WT=13.00" \
    18.00 13.00 --algorithm RR   --input tests/simultaneous.txt --quantum 2

run_metric_test "MLFQ  simultaneous  TT=18.00 WT=13.00" \
    18.00 13.00 --algorithm MLFQ --input tests/simultaneous.txt

# ── staircase.txt — no overlap, no idle gaps ───────────────────────────────────
echo ""
echo "=== staircase.txt — no overlap no idle gaps ==="

run_metric_test "FCFS  staircase  TT=5.60 WT=0.00" \
    5.60 0.00 --algorithm FCFS --input tests/staircase.txt

run_metric_test "SJF   staircase  TT=5.60 WT=0.00" \
    5.60 0.00 --algorithm SJF  --input tests/staircase.txt

run_metric_test "STCF  staircase  TT=5.60 WT=0.00" \
    5.60 0.00 --algorithm STCF --input tests/staircase.txt

run_metric_test "RR    staircase  q=2  TT=5.60 WT=0.00" \
    5.60 0.00 --algorithm RR   --input tests/staircase.txt --quantum 2

run_metric_test "MLFQ  staircase  TT=5.60 WT=0.00" \
    5.60 0.00 --algorithm MLFQ --input tests/staircase.txt

# ── identical_burst.txt — tie-breaking under equal burst time ─────────────────
echo ""
echo "=== identical_burst.txt — identical burst times ==="

run_metric_test "FCFS  identical_burst  TT=22.80 WT=12.80" \
    22.80 12.80 --algorithm FCFS --input tests/identical_burst.txt

run_metric_test "SJF   identical_burst  TT=22.80 WT=12.80" \
    22.80 12.80 --algorithm SJF  --input tests/identical_burst.txt

run_metric_test "STCF  identical_burst  TT=22.80 WT=12.80" \
    22.80 12.80 --algorithm STCF --input tests/identical_burst.txt

run_metric_test "RR    identical_burst  q=2  TT=32.40 WT=22.40" \
    32.40 22.40 --algorithm RR   --input tests/identical_burst.txt --quantum 2

run_metric_test "MLFQ  identical_burst  TT=38.80 WT=28.80" \
    38.80 28.80 --algorithm MLFQ --input tests/identical_burst.txt

# ── --compare mode ────────────────────────────────────────────────────────────
echo ""
echo "=== --compare mode (no crash) ==="

run_crash_test "compare workload1 q=2"  --compare --input tests/workload1.txt
run_crash_test "compare workload1 q=4"  --compare --input tests/workload1.txt --quantum 4
run_crash_test "compare workload2"       --compare --input tests/workload2.txt
run_crash_test "compare single"          --compare --input tests/single.txt
run_crash_test "compare simultaneous"    --compare --input tests/simultaneous.txt
run_crash_test "compare staircase"       --compare --input tests/staircase.txt
run_crash_test "compare identical_burst" --compare --input tests/identical_burst.txt

# ── error paths — must exit non-zero ─────────────────────────────────────────
echo ""
echo "=== error paths ==="

run_exit_test "missing --input"              1 --algorithm FCFS
run_exit_test "missing --algorithm"          1 --input tests/workload1.txt
run_exit_test "unknown algorithm"            1 --algorithm FOO  --input tests/workload1.txt
run_exit_test "RR zero quantum"              1 --algorithm RR   --input tests/workload1.txt --quantum 0
run_exit_test "RR negative quantum"          1 --algorithm RR   --input tests/workload1.txt --quantum -1
run_exit_test "nonexistent input file"       1 --algorithm FCFS --input tests/does_not_exist.txt

# ── stress test — 100 processes, memory stability ─────────────────────────────
echo ""
echo "=== stress test — 100 processes ==="

run_crash_test "FCFS  stress"       --algorithm FCFS --input tests/stress.txt
run_crash_test "SJF   stress"       --algorithm SJF  --input tests/stress.txt
run_crash_test "STCF  stress"       --algorithm STCF --input tests/stress.txt
run_crash_test "RR    stress  q=2"  --algorithm RR   --input tests/stress.txt --quantum 2
run_crash_test "RR    stress  q=5"  --algorithm RR   --input tests/stress.txt --quantum 5
run_crash_test "MLFQ  stress"       --algorithm MLFQ --input tests/stress.txt
run_crash_test "compare stress"     --compare        --input tests/stress.txt

# ── summary ───────────────────────────────────────────────────────────────────
echo ""
TOTAL=$((PASS + FAIL))
echo "Results: $PASS/$TOTAL passed, $FAIL failed"
echo ""
if [[ $FAIL -eq 0 ]]; then
    echo "All tests passed."
    exit 0
else
    exit 1
fi