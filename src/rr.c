// Round Robin Implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/scheduler.h"
#include "../include/utils.h"

/* ------------------------------------------------------------------
 * Algorithm:
 *   1. Processes are enqueued in arrival order (lower PID first on ties).
 *   2. CPU runs the front of the queue for min(quantum, remaining_time)
 *      ticks. At the end of each slice:
 *       a. Enqueue any processes that arrived during the slice (PID order).
 *       b. Requeue the current process at the back if not finished.
 *      This ensures a process that just used its quantum does not jump
 *      ahead of processes that arrived during that same slice.
 *   3. Context switches counted only between two valid processes.
 *   4. Idle → process is NOT a context switch.
 *   5. Simultaneous arrivals: lower PID enqueued first.
 *
 * Complexity: O(n^2) worst case per scheduling round.
 * ------------------------------------------------------------------ */

// Circular ready queue backed by fixed-size index array.
// Stores indices into the local[] process array.
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
	// Prevent circular queue overflow
    if (q->count >= QUEUE_SIZE) {
        fprintf(stderr,
                "RR Error: ready queue overflow "
                "(QUEUE_SIZE=%d)\n",
                QUEUE_SIZE);
        exit(EXIT_FAILURE);
    }
    q->data[q->tail] = idx;
    q->tail          = (q->tail + 1) % QUEUE_SIZE;
    q->count++;
}

static int queue_dequeue(ReadyQueue *q) {
	// Prevent circular queue overflow
    if (q->count >= QUEUE_SIZE) {
        fprintf(stderr,
                "RR Error: ready queue overflow "
                "(QUEUE_SIZE=%d)\n",
                QUEUE_SIZE);
        exit(EXIT_FAILURE);
    }
    int idx = q->data[q->head];
    q->head = (q->head + 1) % QUEUE_SIZE;
    q->count--;
    return idx;
}

// Enqueue all processes that have arrived by `time` and not yet enqueued,
// in PID order. Uses enqueued[] for duplicate tracking.
static void enqueue_arrivals(ReadyQueue *q, Process local[], int enqueued[],
                             int n, int time) {
    int added = 1;
    while (added) {
        added    = 0;
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (enqueued[i] || local[i].arrival_time > time) continue;
            if (best == -1 || strcmp(local[i].pid, local[best].pid) < 0)
                best = i;
        }
        if (best != -1) {
            queue_enqueue(q, best);
            enqueued[best] = 1;
            added          = 1;
        }
    }
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

    int enqueued[MAX_PROCESSES] = {0};
    int completed    = 0;
    int current_time = 0;
    char prev_pid[16] = "IDLE";

    // Enqueue all processes already arrived at t=0
    enqueue_arrivals(&q, local, enqueued, n, current_time);

    while (completed < n) {

        // Idle: nothing in queue yet
        if (queue_empty(&q)) {
            int jump = next_arrival_time(local, enqueued, n, current_time);
            if (jump == -1) break;      // should not happen
            gantt_append(state, "IDLE", current_time, jump);
            state->idle_time += jump - current_time;
            current_time = jump;
            snprintf(prev_pid, sizeof(prev_pid), "IDLE");
            enqueue_arrivals(&q, local, enqueued, n, current_time);
            continue;
        }

        // Dequeue next process
        int idx    = queue_dequeue(&q);
        Process *p = &local[idx];

        // Context switch: process → process only
        if (strcmp(prev_pid, "IDLE") != 0 && strcmp(prev_pid, p->pid) != 0)
            state->context_switches++;
        snprintf(prev_pid, sizeof(prev_pid), "%s", p->pid);

        // First execution: record start_time
        if (p->start_time == -1)
            p->start_time = current_time;

        // Run for min(quantum, remaining_time) ticks
        int slice     = (p->remaining_time < quantum) ? p->remaining_time : quantum;
        int slice_end = current_time + slice;

        gantt_append(state, p->pid, current_time, slice_end);
        p->remaining_time -= slice;
        current_time       = slice_end;

        // Enqueue arrivals that came in during this slice (before requeueing)
        enqueue_arrivals(&q, local, enqueued, n, current_time);

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