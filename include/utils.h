#ifndef UTILS_H
#define UTILS_H

#include "scheduler.h"

// String utilities ─────────────────────────────────────────────────
void str_to_upper(char *s);

// Gantt helpers ────────────────────────────────────────────────────
/* Append a single Gantt entry [start, end) for the given pid label.
 * queue_level: MLFQ queue at time of execution, or -1 if not applicable.
 * Used by non-preemptive schedulers (FCFS, SJF) and RR which work in
 * full slices rather than per-tick.                                      */
void gantt_append(SchedulerState *state, const char *pid, int start, int end);

/* Coalesce-append a single tick for the given pid label.
 * If the last Gantt entry has the same pid, its end_time is extended by 1
 * rather than creating a new entry.  Used by per-tick schedulers (STCF,
 * MLFQ) to keep the Gantt log compact.
 * queue_level: MLFQ queue at time of execution, or -1 if not applicable. */
void gantt_coalesce(SchedulerState *state, const char *pid,
                    int tick, int queue_level);

#endif /* UTILS_H */