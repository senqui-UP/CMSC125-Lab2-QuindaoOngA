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
// gantt coalescing: extend last entry if same PID, else append new entry
static void gantt_coalesce(SchedulerState *state, const char *pid, int tick) {
    if (state->gantt_count > 0) {
        GanttEntry *last = &state->gantt[state->gantt_count - 1];
        if (strcmp(last->pid, pid) == 0) {
            last->end_time = tick + 1;
            return;
        }
    }
    if (state->gantt_count >= state->gantt_capacity) {
        fprintf(stderr, "Warning: Gantt log full, entry dropped\n");
        return;
    }
    GanttEntry *e = &state->gantt[state->gantt_count++];
    snprintf(e->pid, sizeof(e->pid), "%s", pid);
    e->start_time  = tick;
    e->end_time    = tick + 1;
    e->queue_level = -1;
}
 
// pick process with shortest remaining_time among arrived, unfinished
// Tiebreak: lower PID. Returns index into local[], or -1 if none ready
static int pick_stcf(const Process local[], const int done[], int n, int current_time) {
    int best = -1;
    for (int i = 0; i < n; i++) {
        if (done[i] || local[i].arrival_time > current_time)        continue;
        if (best == -1)                                           { best = i; continue; }
        if (local[i].remaining_time < local[best].remaining_time) { best = i; continue; }
        if (local[i].remaining_time == local[best].remaining_time &&
            strcmp(local[i].pid, local[best].pid) < 0)              best = i;
    }
    return best;
}
 
// find the next arrival time among unfinished processes ------------ */
static int next_arrival(const Process local[], const int done[], int n, int current_time) {
    int earliest = -1;
    for (int i = 0; i < n; i++) {
        if (done[i] || local[i].arrival_time <= current_time) continue;
        if (earliest == -1 || local[i].arrival_time < earliest)
            earliest = local[i].arrival_time;
    }
    return earliest;
} 

// schedule_stcf ────────────────────────────────────────────────────
int schedule_stcf(SchedulerState *state) {
    if (!state || state->num_processes <= 0) {
        fprintf(stderr, "STCF Error: empty or null workload\n");
        return -1;
    }
 
    int n = state->num_processes;
 
    // Local working copy
    Process local[MAX_PROCESSES];
    memcpy(local, state->processes, sizeof(Process) * (size_t)n);
 
    int done[MAX_PROCESSES] = {0};
    int completed    = 0;
    int current_time = 0;
    char prev_pid[16] = "IDLE";

    while (completed < n) {
        int idx = pick_stcf(local, done, n, current_time);
 
        // Idle: no process has arrived yet
        if (idx == -1) {
            int jump = next_arrival(local, done, n, current_time);
            if (jump == -1) {
                break;
            }
            gantt_coalesce(state, "IDLE", current_time);
            state->idle_time++;
            current_time++;
            if (current_time < jump) {
                state->gantt[state->gantt_count - 1].end_time = jump;
                state->idle_time += jump - current_time;
                current_time = jump;
            }
            snprintf(prev_pid, sizeof(prev_pid), "IDLE");
            continue;
        }
 
        Process *p = &local[idx];
 
        // Context switch: process → process only
        if (strcmp(prev_pid, "IDLE") != 0 && strcmp(prev_pid, p->pid) != 0)
            state->context_switches++;
        snprintf(prev_pid, sizeof(prev_pid), "%s", p->pid);

        // Record start_time on very first execution
        if (p->start_time == -1)
            p->start_time = current_time;
 
        // Run for 1 tick 
        gantt_coalesce(state, p->pid, current_time);
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
 
    // Write computed metrics
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (strcmp(state->processes[j].pid, local[i].pid) == 0) {
                state->processes[j].start_time     = local[i].start_time;
                state->processes[j].finish_time     = local[i].finish_time;
                state->processes[j].turnaround_time = local[i].turnaround_time;
                state->processes[j].waiting_time    = local[i].waiting_time;
                state->processes[j].remaining_time  = local[i].remaining_time;
                break;
            }
 
    state->total_time          = current_time;
    state->completed_processes = n;
 
    return 0;
}