// Shortest Time to Completion First Implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/scheduler.h"

/* ------------------------------------------------------------------
 * Algorithm:
 *   1. Every time unit, scan all arrived, unfinished processes
 *   2. Pick the process with the smallest remaining_time. Run it for exactly 1 tick
 *   3. If new process arrives with smaller remaining_time, it preempts immediately on the next tick.
 *   4. If equal remaining_time, lower PID has priority.
 *
 * Gantt coalescing:
 *   extends current entry's end time if the same PID continues running
 *   only create a new entry on a process switch or the first tick
 *
 * Complexity: O(n^2)
 * ------------------------------------------------------------------ */

// Helpers ──────────────────────────────────────────────────────────
// extend last Gantt entry if same PID, else append a new one
static void gantt_append_or_extend(SchedulerState *state, int pid, int tick) {
    // Coalesce: same PID as last entry, just push the end forward
    if (state->gantt_count > 0) {
        GanttEntry *last = &state->gantt[state->gantt_count - 1];
        if (last->pid == pid) {
            last->end = tick + 1;
            return;
        }
    }
 
    // New entry needed (also handles the very first tick: gantt_count == 0)
    if (state->gantt_count >= MAX_GANTT_ENTRIES) {
        fprintf(stderr, "Warning: Gantt log full, entry dropped\n");
        return;
    }
    state->gantt[state->gantt_count].pid   = pid;
    state->gantt[state->gantt_count].start = tick;
    state->gantt[state->gantt_count].end   = tick + 1;
    state->gantt_count++;
}
 
// pick process with shortest remaining_time among arrived, unfinished
// Tiebreak: lower PID. Returns index into local[], or -1 if none ready
static int pick_stcf(const Process local[], const int done[], int n,
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
        // Shorter remaining time wins
        if (local[i].remaining_time < local[best].remaining_time) {
            best = i;
            continue;
        }
        // If tied: lower PID wins
        if (local[i].remaining_time == local[best].remaining_time &&
            local[i].pid < local[best].pid) {
            best = i;
        }
    }
    return best;
}
 
// find the next arrival time among unfinished processes ------------ */
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
 
// print results table sorted by PID
static void print_results(Process local[], int n) {
    // Sort by PID for readable output
    for (int i = 0; i < n - 1; i++)
        for (int j = i + 1; j < n; j++)
            if (local[j].pid < local[i].pid) {
                Process tmp = local[i];
                local[i]    = local[j];
                local[j]    = tmp;
            }
 
    printf("\n--- STCF Results ---\n");
    printf("%-6s %-6s %-6s %-6s %-6s %-6s %-6s\n",
           "PID", "AT", "BT", "FT", "TT", "WT", "RT");
    printf("%-6s %-6s %-6s %-6s %-6s %-6s %-6s\n",
           "---", "--", "--", "--", "--", "--", "--");
 
    double total_tt = 0, total_wt = 0, total_rt = 0;
    for (int i = 0; i < n; i++) {
        const Process *p = &local[i];
        int rt = p->start_time - p->arrival_time;
        printf("%-6d %-6d %-6d %-6d %-6d %-6d %-6d\n",
               p->pid, p->arrival_time, p->burst_time,
               p->finish_time, p->turnaround_time, p->waiting_time, rt);
        total_tt += p->turnaround_time;
        total_wt += p->waiting_time;
        total_rt += rt;
    }
 
    printf("\nAverage Turnaround Time : %.2f\n", total_tt / n);
    printf("Average Waiting Time    : %.2f\n",   total_wt / n);
    printf("Average Response Time   : %.2f\n",   total_rt / n);
}
 
// schedule_stcf ────────────────────────────────────────────────────
int schedule_stcf(SchedulerState *state) {
    if (!state || state->n <= 0) {
        fprintf(stderr, "STCF Error: empty or null workload\n");
        return -1;
    }
 
    int n = state->n;
 
    // Local working copy
    Process local[MAX_PROCESSES];
    memcpy(local, state->processes, sizeof(Process) * (size_t)n);
 
    int done[MAX_PROCESSES] = {0};
    int completed    = 0;
    int current_time = 0;
 
    while (completed < n) {
 
        int idx = pick_stcf(local, done, n, current_time);
 
        // Idle: no process has arrived yet
        if (idx == -1) {
            int jump = next_arrival(local, done, n, current_time);
            if (jump == -1)         // if no more processes, should not happen
                break;
            gantt_append_or_extend(state, -1, current_time);
            // advance tick by tick so coalescing works correctly
            current_time++;
            // if idle, jump remaining idle ticks in one step
            if (current_time < jump) {
                // extend the idle entry directly to the jump point
                state->gantt[state->gantt_count - 1].end = jump;
                current_time = jump;
            }
            continue;
        }
 
        Process *p = &local[idx];
 
        // Record start_time on very first execution
        if (p->start_time == -1)
            p->start_time = current_time;
 
        // Run for 1 tick 
        gantt_append_or_extend(state, p->pid, current_time);
        current_time++;
        p->remaining_time--;
 
        // Check completion 
        if (p->remaining_time == 0) {
            p->finish_time     = current_time;
            p->turnaround_time = p->finish_time - p->arrival_time;
            p->waiting_time    = p->turnaround_time - p->burst_time;
            done[idx] = 1;
            completed++;
        }
    }
 
    // Write computed metrics back to the caller's process array
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (state->processes[j].pid == local[i].pid) {
                state->processes[j].start_time     = local[i].start_time;
                state->processes[j].finish_time     = local[i].finish_time;
                state->processes[j].turnaround_time = local[i].turnaround_time;
                state->processes[j].waiting_time    = local[i].waiting_time;
                state->processes[j].remaining_time  = local[i].remaining_time;
                break;
            }
        }
    }
 
    print_results(local, n);
 
    return 0;
}