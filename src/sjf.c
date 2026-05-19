// Shortest Job First Implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/process.h"
#include "../include/scheduler.h"


/* ------------------------------------------------------------------
 * Algorithm:
 *   1. At each CPU selection point, scan all processes that have arrived and not yet completed
 *   2. Pick process with smallest burst_time and run until completion
 *   3. For ties: smallest burst_time > earliest arrival time > lowest PID
 *   4.If no process has arrived yet, jump the clock to the next arrival and insert an IDLE Gantt entry
 *
 * Complexity: O(n^2)
 * ------------------------------------------------------------------ */

// Helpers ──────────────────────────────────────────────────────────
// append one entry to the Gantt log
static void gantt_append(SchedulerState *state, int pid, int start, int end) {
    if (state->gantt_count >= MAX_GANTT_ENTRIES) {
        fprintf(stderr, "Warning: Gantt log full, entry dropped\n");
        return;
    }
    state->gantt[state->gantt_count].pid   = pid;
    state->gantt[state->gantt_count].start = start;
    state->gantt[state->gantt_count].end   = end;
    state->gantt_count++;
}

// comparator for initial sort by arrival_time then PID
static int cmp_arrival(const void *a, const void *b) {
    const Process *pa = (const Process *)a;
    const Process *pb = (const Process *)b;
    if (pa->arrival_time != pb->arrival_time)
        return pa->arrival_time - pb->arrival_time;
    return pa->pid - pb->pid;
}

// pick the best candidate from arrived, unfinished processes.
// Returns index into local[] of the chosen process, or -1 if none are ready
static int pick_shortest(const Process local[], const int done[], int n,
                         int current_time) {
    int best = -1;

    for (int i = 0; i < n; i++) {
        if (done[i])
            continue;
        if (local[i].arrival_time > current_time)
            continue;

        if (best == -1) {
            best = i;
            continue;
        }

        const Process *b = &local[best];
        const Process *c = &local[i];

        // Tie-break 1: shorter burst_time
        if (c->burst_time < b->burst_time) {
            best = i;
            continue;
        }
        if (c->burst_time > b->burst_time)
            continue;

        // Tie-break 2: earlier arrival_time
        if (c->arrival_time < b->arrival_time) {
            best = i;
            continue;
        }
        if (c->arrival_time > b->arrival_time)
            continue;

        // Tie-break 3: lower PID
        if (c->pid < b->pid)
            best = i;
    }

    return best;
}

// find the next arrival time among unfinished processes
static int next_arrival(const Process local[], const int done[], int n,
                        int current_time) {
    int earliest = -1;
    for (int i = 0; i < n; i++) {
        if (done[i])
            continue;
        if (local[i].arrival_time <= current_time)
            continue;
        if (earliest == -1 || local[i].arrival_time < earliest)
            earliest = local[i].arrival_time;
    }
    return earliest;
}

// print results table
static void print_results(const Process *processes, int n) {
    printf("\n--- SJF Results ---\n");
    printf("%-6s %-6s %-6s %-6s %-6s %-6s\n",
           "PID", "AT", "BT", "FT", "TT", "WT");
    printf("%-6s %-6s %-6s %-6s %-6s %-6s\n",
           "---", "--", "--", "--", "--", "--");

    double total_tt = 0, total_wt = 0;
    for (int i = 0; i < n; i++) {
        const Process *p = &processes[i];
        printf("%-6d %-6d %-6d %-6d %-6d %-6d\n",
               p->pid, p->arrival_time, p->burst_time,
               p->finish_time, p->turnaround_time, p->waiting_time);
        total_tt += p->turnaround_time;
        total_wt += p->waiting_time;
    }

    printf("\nAverage Turnaround Time : %.2f\n", total_tt / n);
    printf("Average Waiting Time    : %.2f\n",   total_wt / n);
}

// schedule_sjf ─────────────────────────────────────────────────────
int schedule_sjf(SchedulerState *state)
{
    if (!state || state->n <= 0) {
        fprintf(stderr, "SJF Error: empty or null workload\n");
        return -1;
    }

    int n = state->n;

    // Work on a local copy — sort by arrival so idle-jump logic is clean
    Process local[MAX_PROCESSES];
    memcpy(local, state->processes, sizeof(Process) * (size_t)n);
    qsort(local, (size_t)n, sizeof(Process), cmp_arrival);

    int done[MAX_PROCESSES] = {0};      // tracks which processes have completed
    int completed    = 0;
    int current_time = 0;

    while (completed < n) {

        int idx = pick_shortest(local, done, n, current_time);

        // Idle gap: no process has arrived yet
        if (idx == -1) {
            int jump = next_arrival(local, done, n, current_time);
            if (jump == -1)             // if no more processes — should not happen
                break;
            gantt_append(state, -1, current_time, jump);
            current_time = jump;
            continue;
        }

        Process *p = &local[idx];

        // At first execution: record start_time
        p->start_time = current_time;

        // Run to completion (non-preemptive)
        gantt_append(state, p->pid, current_time, current_time + p->burst_time);
        current_time += p->burst_time;

        // Record completion and compute metrics inline
        p->finish_time     = current_time;
        p->turnaround_time = p->finish_time - p->arrival_time;
        p->waiting_time    = p->turnaround_time - p->burst_time;

        done[idx] = 1;
        completed++;
    }

    // Write computed metrics back to the caller's process array
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (state->processes[j].pid == local[i].pid) {
                state->processes[j].start_time     = local[i].start_time;
                state->processes[j].finish_time     = local[i].finish_time;
                state->processes[j].turnaround_time = local[i].turnaround_time;
                state->processes[j].waiting_time    = local[i].waiting_time;
                break;
            }
        }
    }

    print_results(local, n);

    return 0;
}