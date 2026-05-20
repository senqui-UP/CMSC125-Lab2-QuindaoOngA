# MLFQ Design — schedsim

## Overview

The Multi-Level Feedback Queue (MLFQ) implemented in `schedsim` uses three priority queues and a per-tick simulation loop. It is designed to reward short, interactive processes with high priority while gradually demoting long CPU-bound processes — without ever needing to know a process's burst time in advance.

---

## Queue Configuration

| Queue | Priority | Quantum | Allotment | Behavior          |
|-------|----------|---------|-----------|-------------------|
| Q0    | Highest  | 2 ticks | 4 ticks   | Interactive, short|
| Q1    | Medium   | 4 ticks | 8 ticks   | Medium jobs       |
| Q2    | Lowest   | 8 ticks | Infinite  | CPU-bound, long   |

**Boost period:** every 20 ticks, all processes return to Q0.
These values are hardcoded defaults. Step 6 will add `--mlfq-config` file parsing to make them configurable at runtime.

---

## Core Rules

### 1. New processes enter Q0
Every arriving process is placed at the back of Q0 regardless of its history. This gives short and interactive jobs immediate high priority.

### 2. Allotment exhaustion → demotion
Each queue level has an allotment — the total CPU time a process may accumulate at that level before being demoted. This is tracked via `time_in_queue`, which increments every tick the process runs and resets on demotion or boost.

- `time_in_queue >= allotment` → move to next lower queue
- Q2 has infinite allotment — processes are never demoted further
- Completion on the same tick as allotment expiry: **completion takes priority**, no demotion occurs

### 3. Quantum expiry → requeue at same level
If a process exhausts its time slice (tracked via `quantum_used`) without exhausting its full allotment, it is requeued at the back of the same queue. `quantum_used` resets; `time_in_queue` does not.

### 4. Priority boost every `boost_period` ticks
All unfinished processes are moved to Q0 and their `time_in_queue` and `quantum_used` counters reset. This prevents starvation of long-running processes stuck in lower queues.
The currently running process is also moved to Q0 on a boost but finishes its current tick before being re-evaluated.

### 5. Higher-priority preemption
MLFQ is preemptive across queue levels. If a higher-priority queue becomes non-empty mid-slice (via a new arrival or a boost), the currently running process is preempted on the next tick. Its `quantum_used` resets — no demotion occurs for a preemption. The process returns to the back of its current queue level.

### 6. Lowest queue is preemptible, not FCFS in the strict sense
Q2 has infinite allotment and a large quantum (8 ticks), which gives it FCFS-like behavior for CPU-bound processes. However, it is still preemptible by Q0 or Q1 processes. It is not truly non-preemptible.

---

## Key Design Constraint

> `burst_time` is **never read** for scheduling decisions.

The scheduler uses only:
- `remaining_time` — to detect completion
- `time_in_queue` — to track allotment usage
- `quantum_used` — to track slice usage
- Queue membership — to determine priority

This mimics real OS behavior where the scheduler does not know how long a process will run.

---

## Quantum and Allotment Justification

**Q0: quantum=2, allotment=4**
Short interactive processes (burst ≤ 4) finish entirely within Q0. The small quantum ensures fairness among simultaneous Q0 processes.

**Q1: quantum=4, allotment=8**
Medium jobs that were not short enough for Q0 get a longer slice. The larger allotment gives them time to make progress before demotion.

**Q2: quantum=8, allotment=∞**
Long CPU-bound jobs settle here and run in large chunks, minimizing context-switch overhead. The boost period prevents them starving forever.

**Boost period: 20 ticks**
Long enough that it does not fire constantly on short workloads, but short enough to prevent indefinite starvation on realistic workloads.

---

## Starvation Prevention

Without the boost mechanism, a steady stream of short jobs arriving in Q0 could starve long-running Q2 processes indefinitely. The boost period of 20 ticks guarantees that any process waiting in a lower queue will eventually be promoted back to Q0, regardless of the arrival pattern.

---

## Edge Cases Handled

| Edge Case | Handling |
|-----------|----------|
| Boost fires during execution | Current process finishes its tick, boost applied at start of next scheduling decision |
| Allotment expires on completion tick | Completion takes priority — no demotion |
| `quantum = -1` (infinite) | Treated as infinite allotment — process runs until allotment or completion |
| Single process | Demotes normally through queues; no context switches |
| Simultaneous arrivals | Enqueued into Q0 in PID order |
| Idle CPU | Clock jumps to next arrival; boost timer advances accordingly |

---

## Context Switch Counting

A context switch is counted only when the CPU transitions between two valid processes. Transitions from idle to a process do not count.

```
if (prev_pid != -1 && prev_pid != current_pid)
    context_switches++;
```

---

## Sample Output (workload1.txt)

```
[t=0]  RUN P1 (Q0, remaining=8)
[t=2]  RUN P2 (Q0, remaining=4)
[t=4]  RUN P1 (Q0, remaining=6)
[t=6]  DEMOTE P1 Q0 → Q1
[t=6]  RUN P3 (Q0, remaining=9)
...
[t=20] PRIORITY BOOST — all processes → Q0
...
PID | AT | BT | FT | TT | WT | RT
  1    0    8   20   20   12    0
  2    1    4   12   11    7    1
  3    2    9   26   24   15    4
  4    3    5   23   20   15    5
```

---

## Future Work (Step 6)

- `--mlfq-config <file>` to override queue count, quantums, allotments, and
  boost period at runtime
- Per-queue statistics (time spent in each queue per process)