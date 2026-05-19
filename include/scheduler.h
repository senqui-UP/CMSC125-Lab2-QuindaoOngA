#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include "gantt.h"

// Schedule Interfaces ──────────────────────────────────────────────
/* All schedule_*() functions share the same signature:
 *   processes  : array of Process structs loaded from the workload file
 *   n          : number of processes in the array
 *   quantum    : time slice for RR (ignored by non-preemptive algorithms) */

 typedef struct {
     Process    *processes;    // Array of loaded processes
     int         n;            // Number of processes
     int         quantum;      // Time slice — used by RR and MLFQ
     GanttEntry *gantt;        // Caller-allocated gantt log buffer
     int         gantt_count;  // Number of entries written so far
 } SchedulerState;

// Non-preemptive ---------------------------------------------------
int schedule_fcfs(SchedulerState *state);
int schedule_sjf (SchedulerState *state);

// Preemptive -------------------------------------------------------
int schedule_stcf(SchedulerState *state);
int schedule_rr  (SchedulerState *state);

// Multi-Level Feedback Queue ---------------------------------------
int schedule_mlfq(SchedulerState *state);

#endif /* SCHEDULER_H */