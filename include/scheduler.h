#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

// Schedule Interfaces ──────────────────────────────────────────────
/* All schedule_*() functions share the same signature:
 *   processes  : array of Process structs loaded from the workload file
 *   n          : number of processes in the array
 *   quantum    : time slice for RR (ignored by non-preemptive algorithms) */

// Non-preemptive ---------------------------------------------------
void schedule_fcfs(Process processes[], int n, int quantum);
void schedule_sjf (Process processes[], int n, int quantum);

// Preemptive -------------------------------------------------------
void schedule_stcf(Process processes[], int n, int quantum);
void schedule_rr  (Process processes[], int n, int quantum);

// Multi-Level Feedback Queue ---------------------------------------
void schedule_mlfq(Process processes[], int n, int quantum);

#endif /* SCHEDULER_H */