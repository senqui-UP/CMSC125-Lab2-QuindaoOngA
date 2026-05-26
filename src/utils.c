// Utility Functions

#include <stdio.h>
#include <string.h>
#include "../include/utils.h"

// str_to_upper ─────────────────────────────────────────────────────
// Convert string to uppercase in-place
void str_to_upper(char *s)
{
    for (; *s; s++) {
        if (*s >= 'a' && *s <= 'z')
            *s = (char)(*s - 32);
    }
}

// gantt_append ─────────────────────────────────────────────────────
/* Append a new [start, end) Gantt entry for pid.
 * Used by FCFS, SJF (full-slice), and RR (quantum slice).
 * queue_level = -1 for non-MLFQ schedulers.                         */
void gantt_append(SchedulerState *state, const char *pid, int start, int end)
{
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

// gantt_coalesce ───────────────────────────────────────────────────
/* Per-tick coalescing append.
 * If the last Gantt entry has the same pid, extend its end_time by 1.
 * Otherwise create a new entry covering [tick, tick+1).
 * Used by STCF and MLFQ which simulate one tick at a time.
 * queue_level = -1 for non-MLFQ schedulers.                         */
void gantt_coalesce(SchedulerState *state, const char *pid,
                    int tick, int queue_level)
{
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
    e->queue_level = queue_level;
}
// next_arrival_time ────────────────────────────────────────────────
/* Find the next future arrival time.
 *
 * Used when:
 *   CPU becomes idle and scheduler must jump forward.
 *
 * flags[] meaning:
 *   done[]      for SJF/STCF
 *   enqueued[]  for RR/MLFQ
 *
 * Returns:
 *   earliest future arrival time
 *   -1 if no future arrivals remain
 */
int next_arrival_time(const Process local[],
                      const int flags[],
                      int n,
                      int current_time)
{
    int earliest = -1;

    for (int i = 0; i < n; i++) {

        // Skip:
        //   finished/enqueued processes
        //   already-arrived processes
        if (flags[i] ||
            local[i].arrival_time <= current_time)
        {
            continue;
        }

        // Update earliest future arrival
        if (earliest == -1 ||
            local[i].arrival_time < earliest)
        {
            earliest = local[i].arrival_time;
        }
    }

    return earliest;
}