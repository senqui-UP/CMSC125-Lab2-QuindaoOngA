#!/usr/bin/env bash
# test_suite.sh — Automated test script

SCHEDSIM=./schedsim
PASS=0
FAIL=0

run_test() {
    local desc="$1"
    shift
    if "$SCHEDSIM" "$@" > /dev/null 2>&1; then
        echo "[PASS] $desc"
        ((PASS++))
    else
        echo "[FAIL] $desc"
        ((FAIL++))
    fi
}

echo "=== schedsim smoke tests ==="

run_test "FCFS workload1"  --algorithm FCFS --input tests/workload1.txt
run_test "SJF workload1"   --algorithm SJF  --input tests/workload1.txt
run_test "STCF workload1"  --algorithm STCF --input tests/workload1.txt
run_test "RR workload1"    --algorithm RR   --input tests/workload1.txt --quantum 2
run_test "MLFQ workload1"  --algorithm MLFQ --input tests/workload1.txt

echo ""
echo "Results: $PASS passed, $FAIL failed"