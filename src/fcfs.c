// First Come First Serve Implementation

#include <stdio.h>
#include "../include/process.h"
#include "../include/scheduler.h"

/* ------------------------------------------------------------------
 * Algorithm:
 *   1. Sort processes by arrival_time (lower PID wins, workload files written in PID order by convention)
 *   2. Walk through the sorted list
 *      If the CPU is idle (current_time < arrival), insert an IDLE Gantt entry and jump the clock forward
 *   3. Run each process to completion. Record start_time, then compute finish_time, turnaround_time, and waiting_time inline
 *
 * Complexity: O(n log n) for the sort, O(n) for the simulation pass.
 * ------------------------------------------------------------------ */

// Helpers ----------------------------------------------------------
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

// comparator for qsort — sort by arrival_time, break ties by pid
static int cmp_arrival(const void *a, const void *b) {
    const Process *pa = (const Process *)a;
    const Process *pb = (const Process *)b;
    if (pa->arrival_time != pb->arrival_time)
        return pa->arrival_time - pb->arrival_time;
    return pa->pid - pb->pid;                       // lower PID wins on simultaneous arrival
}

// print results table
static void print_results(const Process *processes, int n) {
    printf("\n--- FCFS Results ---\n");
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

// schedule_fcfs ----------------------------------------------------
int schedule_fcfs(SchedulerState *state) {
    if (!state || state->n <= 0) {
        fprintf(stderr, "FCFS Error: empty or null workload\n");
        return -1;
    }

    int n = state->n;

    // Work on a local sorted copy so the caller's array order is unchanged
    Process sorted[MAX_PROCESSES];
    memcpy(sorted, state->processes, sizeof(Process) * (size_t)n);
    qsort(sorted, (size_t)n, sizeof(Process), cmp_arrival);

    int current_time = 0;

    for (int i = 0; i < n; i++) {
        Process *p = &sorted[i];

        // Idle gap: CPU waits for next process to arrive
        if (current_time < p->arrival_time) {
            gantt_append(state, -1, current_time, p->arrival_time);
            current_time = p->arrival_time;
        }

        // First execution: record start_time
        p->start_time = current_time;

        // Run to completion (non-preemptive)
        gantt_append(state, p->pid, current_time, current_time + p->burst_time);
        current_time += p->burst_time;

        // Record completion and compute metrics inline
        p->finish_time      = current_time;
        p->turnaround_time  = p->finish_time - p->arrival_time;
        p->waiting_time     = p->turnaround_time - p->burst_time;
    }

    // Write computed metrics back to the caller's process array
    for (int i = 0; i < n; i++) {
        // Match sorted[i] back to original by PID
        for (int j = 0; j < n; j++) {
            if (state->processes[j].pid == sorted[i].pid) {
                state->processes[j].start_time     = sorted[i].start_time;
                state->processes[j].finish_time     = sorted[i].finish_time;
                state->processes[j].turnaround_time = sorted[i].turnaround_time;
                state->processes[j].waiting_time    = sorted[i].waiting_time;
                break;
            }
        }
    }

    print_results(sorted, n);

    return 0;
}