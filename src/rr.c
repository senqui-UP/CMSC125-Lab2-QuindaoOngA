//Round Robin Implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/scheduler.h"

/* ------------------------------------------------------------------
 * Algorithm:
 *   1. Processes are enqueued in arrival order
 *   2. CPU runs the front of the queue for min(quantum, remaining_time) ticks. At the end of each slice:
 *       a. Enqueue any processes that arrived during the slice (in PID order)
 *       b. Requeue the current process at the back if it is not finished
 *      Ensures that a process that just used its quantum does not jump ahead of processes that arrived during that same slice
 *   3.Context switches are counted only when CPU switches between two valid processes
 *   4. Idle process does NOT count as a context switch
 *   5. For simultaneous arrivals: lower PID is enqueued first
 *
 * Complexity: O(n^2) worst case per scheduling round
 * ------------------------------------------------------------------ */

// circular ready queue backed by fixed-size index array
// stores indices into local[] process array
#define QUEUE_SIZE (MAX_PROCESSES * 2)
 
typedef struct {
    int data[QUEUE_SIZE];
    int head;
    int tail;
    int count;
} ReadyQueue;
 
static void queue_init(ReadyQueue *q) {
    q->head  = 0;
    q->tail  = 0;
    q->count = 0;
}
 
static int queue_empty(const ReadyQueue *q) {
    return q->count == 0;
}
 
static void queue_enqueue(ReadyQueue *q, int idx) {
    q->data[q->tail] = idx;
    q->tail          = (q->tail + 1) % QUEUE_SIZE;
    q->count++;
}
 
static int queue_dequeue(ReadyQueue *q) {
    int idx  = q->data[q->head];
    q->head  = (q->head + 1) % QUEUE_SIZE;
    q->count--;
    return idx;
}
 
// Helpers ──────────────────────────────────────────────────────────
// append a new Gantt entry
static void gantt_append(SchedulerState *state, const char *pid,
                         int start, int end) {
    if (state->gantt_count >= state->gantt_capacity) {
        fprintf(stderr, "Warning: Gantt log full, entry dropped\n");
        return;
    }
    GanttEntry *e = &state->gantt[state->gantt_count++];
    snprintf(e->pid, sizeof(e->pid), "%s", pid);
    e->start_time  = start;
    e->end_time    = end;
    e->queue_level = -1;
}

// enqueue all processes that have arrived by time (in PID order) that have not yet been enqueued. Uses enqueued[] for tracking
static void enqueue_arrivals(ReadyQueue *q, Process local[], int enqueued[],
                             int n, int time) {
    // Simple insertion: find the lowest-PID unqueued arrival <= time
    // Repeat until no more candidates — preserves PID order regardless of the order processes appear in local[]
    int added = 1;
    while (added) {
        added = 0;
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (enqueued[i] || local[i].arrival_time > time) continue;
            if (best == -1 || local[i].pid < local[best].pid) best = i;
        }
        if (best != -1) { queue_enqueue(q, best); enqueued[best] = 1; added = 1; }
    }
}

// find the next arrival time among un-enqueued processes
static int next_arrival(const Process local[], const int enqueued[], int n,
                        int current_time) {
    int earliest = -1;
    for (int i = 0; i < n; i++) {
        if (enqueued[i])
            continue;
        if (local[i].arrival_time <= current_time)
            continue;
        if (earliest == -1 || local[i].arrival_time < earliest)
            earliest = local[i].arrival_time;
    }
    return earliest;
}

// schedule_rr ──────────────────────────────────────────────────────
int schedule_rr(SchedulerState *state) {
    if (!state || state->num_processes <= 0) {
        fprintf(stderr, "RR Error: empty or null workload\n");
        return -1;
    }

    int quantum = state->quantum;
    if (quantum <= 0) {
        fprintf(stderr, "RR Error: quantum must be > 0 (got %d)\n", quantum);
        return -1;
    }

    int n = state->num_processes;

    // Local working copy
    Process local[MAX_PROCESSES];
    memcpy(local, state->processes, sizeof(Process) * (size_t)n);

    ReadyQueue q;
    queue_init(&q);

    int enqueued[MAX_PROCESSES] = {0};          // Tracks which processes have entered the ready queue, prevents duplicates
    int completed        = 0;
    int current_time     = 0;
    char prev_pid[16] = "IDLE";

    // Enqueue all processes that have already arrived at t=0
    enqueue_arrivals(&q, local, enqueued, n, current_time);

    while (completed < n) {

        // Idle: nothing in queue yet
        if (queue_empty(&q)) {
            int jump = next_arrival(local, enqueued, n, current_time);
            if (jump == -1)                     // if no more processes, should not happen
                break;
            gantt_append(state, "IDLE", current_time, jump);
            state->idle_time += jump - current_time;
            current_time = jump;
            // prev_pid stays -1 — idle to process is not a context switch
            snprintf(prev_pid, sizeof(prev_pid), "IDLE");
            enqueue_arrivals(&q, local, enqueued, n, current_time);
            continue;
        }

        // Dequeue next process
        int idx      = queue_dequeue(&q);
        Process *p   = &local[idx];

        char cur_pid[16];
        snprintf(cur_pid, sizeof(cur_pid), "%d", p->pid);

        // Count context switch (only process → process)
        if (strcmp(prev_pid, "IDLE") != 0 && strcmp(prev_pid, cur_pid) != 0)
            state->context_switches++;
        snprintf(prev_pid, sizeof(prev_pid), "%s", cur_pid);

        // Record start_time on very first execution
        if (p->start_time == -1)
            p->start_time = current_time;

        // Run for min(quantum, remaining_time) ticks
        int slice = (p->remaining_time < quantum) ? p->remaining_time : quantum;
        int slice_end = current_time + slice;

        gantt_append(state, cur_pid, current_time, slice_end);
        p->remaining_time -= slice;
        current_time       = slice_end;

        // Enqueue arrivals that came in during this slice (before requeueing current process)
        enqueue_arrivals(&q, local, enqueued, n, current_time);

        // Finished process
        if (p->remaining_time == 0) {
            p->finish_time     = current_time;
            p->turnaround_time = p->finish_time - p->arrival_time;
            p->waiting_time    = p->turnaround_time - p->burst_time;
            completed++;
        } else {
            queue_enqueue(&q, idx);
        }
    }

    // Write computed metrics back to the caller's process array
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (state->processes[j].pid == local[i].pid) {
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