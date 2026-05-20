// Multi-Level Feedback Queue (MLFQ) Implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/scheduler.h"

/* ------------------------------------------------------------------
 * Design:
 *   Three priority queues (0 = highest, 2 = lowest).
 *   Per-tick simulation with arrival checking, boost checking, and
 *   higher-priority preemption every tick.
 *
 * Core rules:
 *   1. New processes enter Q0.
 *   2. Exhaust allotment → demote to next queue (reset time_in_queue,
 *      quantum_used). Completion on same tick as allotment expiry → no
 *      demotion; completion takes priority.
 *   3. Every boost_period ticks → all unfinished processes move to Q0;
 *      time_in_queue and quantum_used reset.
 *   4. Lowest queue (Q2) has infinite allotment — never demoted further.
 *      Still preemptible by higher queues.
 *   5. Higher-priority preemption: if a higher queue becomes non-empty
 *      mid-slice, current process is preempted immediately next tick.
 *      quantum_used resets (partial slice lost); no demotion for preempt.
 *
 * Important constraint:
 *   burst_time is NEVER read for scheduling decisions.
 *   Only remaining_time, time_in_queue, and allotment drive the logic.
 *
 * Default config (overridable via --mlfq-config in Step 6):
 *   Q0: quantum=2,  allotment=4
 *   Q1: quantum=4,  allotment=8
 *   Q2: quantum=8,  allotment=-1 (infinite)
 *   boost_period=20
 * ------------------------------------------------------------------ */

#define MLFQ_MAX_QUEUES 8
#define INFINITE_ALLOTMENT -1
#define MLFQ_QUEUE_SIZE   (MAX_PROCESSES * 4) // Ready Queue (circular index array — same design as RR)

typedef struct {
    int quantums[MLFQ_MAX_QUEUES];     // time slice per queue level  
    int allotments[MLFQ_MAX_QUEUES];   // max allotment ticks (-1 = infinite)
    int num_queues;                    // number of queue levels
    int boost_period;                  // ticks between priority boosts 
} MLFQConfig;
 
// Default config
static MLFQConfig default_config(void) {
    MLFQConfig cfg;
    cfg.num_queues    = 3;
    cfg.boost_period  = 20;
 
    cfg.quantums[0]   = 2;   cfg.allotments[0] = 4;
    cfg.quantums[1]   = 4;   cfg.allotments[1] = 8;
    cfg.quantums[2]   = 8;   cfg.allotments[2] = INFINITE_ALLOTMENT;
 
    return cfg;
}
  
typedef struct {
    int data[MLFQ_QUEUE_SIZE];
    int head;
    int tail;
    int count;
} MLFQQueue;
 
static void mq_init(MLFQQueue *q) {
    q->head  = 0;
    q->tail  = 0;
    q->count = 0;
}
 
static int mq_empty(const MLFQQueue *q) { 
    return q->count == 0;
}
 
static void mq_enqueue(MLFQQueue *q, int idx) {
    q->data[q->tail] = idx;
    q->tail          = (q->tail + 1) % MLFQ_QUEUE_SIZE;
    q->count++;
}
 
static int mq_dequeue(MLFQQueue *q) {
    int idx = q->data[q->head];
    q->head = (q->head + 1) % MLFQ_QUEUE_SIZE;
    q->count--;
    return idx;
}

// Helpers ──────────────────────────────────────────────────────────
// Gantt: coalesce same-pid ticks into one entry
static void gantt_coalesce(SchedulerState *state, const char *pid,
                           int tick, int qlevel) {
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
    e->queue_level = qlevel;
}
 
// Enqueue all processes that have arrived by `time` and not yet enqueued, in PID order, into Q0
static void enqueue_arrivals(MLFQQueue queues[], int enqueued[],
                             Process local[], int n, int time) {
    int added = 1;
    while (added) {
        added    = 0;
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (enqueued[i])                                         continue;
            if (local[i].arrival_time > time)                        continue;
        if (best == -1 || strcmp(local[i].pid, local[best].pid) < 0) best = i;
        }
        if (best != -1) {
            local[best].queue_level  = 0;
            local[best].time_in_queue = 0;
            local[best].quantum_used  = 0;
            mq_enqueue(&queues[0], best);
            enqueued[best] = 1;
            added          = 1;
        }
    }
}
 
// new arrival time among processes not yet enqueued, or -1 if none
static int next_arrival_time(const Process local[], const int enqueued[],
                             int n, int current_time) {
    int earliest = -1;
    for (int i = 0; i < n; i++) {
        if (enqueued[i])                           continue;
        if (local[i].arrival_time <= current_time) continue;
        if (earliest == -1 || local[i].arrival_time < earliest)
            earliest = local[i].arrival_time;
    }
    return earliest;
}
 
// boost all unfinished processes to Q0, reset time_in_queue and quantum_used
static void do_boost(MLFQQueue queues[], Process local[], int done[],
                     int current_time, int current_idx)
{
    printf("[t=%d] PRIORITY BOOST — all processes → Q0\n", current_time);
 
    // Drain all queues into a temporary list, sorted by PID
    int to_boost[MAX_PROCESSES];
    int count = 0;
 
    for (int q = 0; q < MLFQ_MAX_QUEUES; q++) {
        while (!mq_empty(&queues[q])) {
            int idx = mq_dequeue(&queues[q]);
            // Avoid duplicating the currently running process
            if (idx == current_idx) continue;
            to_boost[count++] = idx;
        }
    }
 
    // Sort by PID for deterministic boost order 
    for (int i = 0; i < count - 1; i++)
        for (int j = i + 1; j < count; j++)
            if (strcmp(local[to_boost[j]].pid, local[to_boost[i]].pid) < 0) {
                int tmp      = to_boost[i];
                to_boost[i]  = to_boost[j];
                to_boost[j]  = tmp;
            }
 
    // Re-enqueue into Q0, reset counters
    for (int i = 0; i < count; i++) {
        int idx = to_boost[i];
        if (done[idx]) continue;
        local[idx].queue_level   = 0;
        local[idx].time_in_queue = 0;
        local[idx].quantum_used  = 0;
        mq_enqueue(&queues[0], idx);
    }
 
    // Also reset the currently running process if it exists
    if (current_idx >= 0) {
        local[current_idx].queue_level   = 0;
        local[current_idx].time_in_queue = 0;
        local[current_idx].quantum_used  = 0;
    }
}
 
// Highest non-empty queue index
static int highest_queue(const MLFQQueue queues[], int num_queues)
{
    for (int q = 0; q < num_queues; q++)
        if (!mq_empty(&queues[q]))
            return q;
    return -1;
}

// schedule_mlfq ────────────────────────────────────────────────────
int schedule_mlfq(SchedulerState *state)
{
    if (!state || state->num_processes <= 0) {
        fprintf(stderr, "MLFQ Error: empty or null workload\n");
        return -1;
    }
 
    int n = state->num_processes;
    MLFQConfig cfg = default_config();
 
    // Local working copy
    Process local[MAX_PROCESSES];
    memcpy(local, state->processes, sizeof(Process) * (size_t)n);
 
    // Per-queue ready queues
    MLFQQueue queues[MLFQ_MAX_QUEUES];
    for (int q = 0; q < MLFQ_MAX_QUEUES; q++)
        mq_init(&queues[q]);
 
    int enqueued[MAX_PROCESSES] = {0};
    int done[MAX_PROCESSES]     = {0};
    int completed        = 0;
    int current_time     = 0;
    int boost_timer      = 0;
    int current_idx      = -1;
    char prev_pid[16]    = "IDLE";
 
    // Enqueue any processes already arrived at t=0
    enqueue_arrivals(queues, enqueued, local, n, current_time);
 
    printf("\n--- MLFQ Execution Trace ---\n");
 
    while (completed < n) {
 
        /* -- 1. Check boost -------------------------------------------- */
        if (cfg.boost_period > 0 && boost_timer > 0 &&
            boost_timer % cfg.boost_period == 0) {
            do_boost(queues, local, done, current_time, current_idx);
            state->boosts++;
            boost_timer = 0;            
            // If running process was moved to Q0, it continues this tick
            // will be re-evaluated next tick against other Q0 members
        }
 
        /* -- 2. Enqueue new arrivals ------------------------------------ */
        enqueue_arrivals(queues, enqueued, local, n, current_time);
 
        /* -- 3. Check for higher-priority preemption -------------------- */
        if (current_idx >= 0) {
            int hq = highest_queue(queues, cfg.num_queues);
            if (hq >= 0 && hq < local[current_idx].queue_level) {
                // Higher-priority queue has a process — preempt
                printf("[t=%d] PREEMPT P%s (Q%d) → higher priority Q%d ready\n",
                       current_time, local[current_idx].pid,
                       local[current_idx].queue_level, hq);
                // Put preempted process back — no demotion, quantum resets
                local[current_idx].quantum_used = 0;
                mq_enqueue(&queues[local[current_idx].queue_level],
                           current_idx);
                current_idx = -1;
            }
        }
 
        /* 4. Pick next process if CPU is idle ----------------------- */
        if (current_idx == -1) {
            int hq = highest_queue(queues, cfg.num_queues);
 
            if (hq == -1) {
                // All queues empty — CPU idle
                int jump = next_arrival_time(local, enqueued, n, current_time);
                if (jump == -1) break;
                gantt_coalesce(state, "IDLE", current_time, -1);
                // Extend idle entry to jump point
                state->gantt[state->gantt_count - 1].end_time = jump;
                state->idle_time  += jump - current_time;
                boost_timer       += jump - current_time;
                current_time       = jump;
                snprintf(prev_pid, sizeof(prev_pid), "IDLE");
                enqueue_arrivals(queues, enqueued, local, n, current_time);
                continue;
            }
 
            current_idx = mq_dequeue(&queues[hq]);
            Process *p  = &local[current_idx];
 
            // Context switch (process → process only)
            char cur_pid[16];
            snprintf(cur_pid, sizeof(cur_pid), "%s", p->pid);
            if (strcmp(prev_pid, "IDLE") != 0 && strcmp(prev_pid, cur_pid) != 0)
                state->context_switches++;
            snprintf(prev_pid, sizeof(prev_pid), "%s", cur_pid);
 
            // on first execution, record start_time
            if (p->start_time == -1) p->start_time = current_time;
 
            printf("[t=%d] RUN P%s (Q%d, remaining=%d)\n",
                   current_time, p->pid, p->queue_level, p->remaining_time);
        }
 
        /* -- 5. Run current process for 1 tick ------------------------- */
        Process *cp = &local[current_idx];
        char cp_pid[16];
        snprintf(cp_pid, sizeof(cp_pid), "%s", cp->pid);
        gantt_coalesce(state, cp_pid, current_time, cp->queue_level);
 
        cp->remaining_time--;
        cp->time_in_queue++;
        cp->quantum_used++;
        current_time++;
        boost_timer++;
 
        /* -- 6. Check completion first (takes priority over demotion) -- */
        if (cp->remaining_time == 0) {
            cp->finish_time     = current_time;
            cp->turnaround_time = cp->finish_time - cp->arrival_time;
            cp->waiting_time    = cp->turnaround_time - cp->burst_time;
            done[current_idx]   = 1;
            completed++;
            printf("[t=%d] FINISH P%s (Q%d) FT=%d TT=%d WT=%d\n",
                   current_time, cp->pid, cp->queue_level,
                   cp->finish_time, cp->turnaround_time, cp->waiting_time);
            snprintf(prev_pid, sizeof(prev_pid), "%s", cp_pid);
            current_idx = -1;
            continue;
        }
 
        int ql  = cp->queue_level;
        int alm = cfg.allotments[ql];
        int qnt = cfg.quantums[ql];
 
        /* -- 7. Check allotment exhaustion ----------------------------- */
        if (alm != INFINITE_ALLOTMENT && cp->time_in_queue >= alm) {
            int new_ql = (ql + 1 < cfg.num_queues) ? ql + 1 : ql;
            printf("[t=%d] DEMOTE P%s Q%d → Q%d\n",
                   current_time, cp->pid, ql, new_ql);
            cp->queue_level = new_ql;
            cp->time_in_queue = cp->quantum_used = 0;
            mq_enqueue(&queues[new_ql], current_idx);
            current_idx = -1;
            continue;
        }
 
        /* -- 8. Check quantum expiry ----------------------------------- */
        if (cp->quantum_used >= qnt) {
            cp->quantum_used = 0;
            // Requeue at back of same level — arrivals enqueued next tick
            mq_enqueue(&queues[ql], current_idx);
            current_idx = -1;
        }
        // else: process continues next tick in current slice
    }
 
    // Write metrics
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (strcmp(state->processes[j].pid, local[i].pid) == 0) {
                state->processes[j].start_time     = local[i].start_time;
                state->processes[j].finish_time     = local[i].finish_time;
                state->processes[j].turnaround_time = local[i].turnaround_time;
                state->processes[j].waiting_time    = local[i].waiting_time;
                state->processes[j].remaining_time  = local[i].remaining_time;
                state->processes[j].queue_level     = local[i].queue_level;
                break;
            }
 
    state->total_time          = current_time;
    state->completed_processes = n;

    return 0;
}