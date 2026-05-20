#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include "gantt.h"

// Schedule Interfaces ──────────────────────────────────────────────
/* All schedule_*() functions share the same signature:
 *   processes  : array of Process structs loaded from the workload file
 *   n          : number of processes in the array
 *   quantum    : time slice for RR (ignored by non-preemptive algorithms) */

typedef struct SchedulerState {
    /* -- Workload ------------------------------------------------- */
    Process *processes;         // array of loaded processes               
    int      num_processes;     // number of processes                     
    int      quantum;           // time slice for RR / MLFQ                 
 
    /* -- Gantt log ------------------------------------------------ */
    GanttEntry *gantt;          // caller-allocated entry buffer           
    int         gantt_count;    // entries written so far                   
    int         gantt_capacity; // size of gantt buffer (MAX_GANTT_ENTRIES) 
 
    /* -- Simulation state ----------------------------------------- */
    int current_time;           // clock at end of simulation              
    int total_time;             // = current_time when simulation ends     
    int idle_time;              // total ticks CPU spent idle              
    int context_switches;       // process→process switches only          
    int completed_processes;    // number of processes that finished      
    int boosts;                 // MLFQ priority boost count (0 for others)
 
    /* -- Identity ------------------------------------------------- */
    char algorithm[32];         // name string, set by main before dispatch

    /* -- Output control ------------------------------------------- */
    int verbose;                // 1 = full output, 0 = suppress trace/report
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