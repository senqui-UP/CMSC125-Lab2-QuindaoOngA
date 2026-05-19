// Gantt Chart Generation

#ifndef GANTT_H
#define GANTT_H

#include "process.h"

// One contiguous block of CPU time in the execution timeline
typedef struct {
    int pid;        // PID of running process, or -1 for idle
    int start;      // Inclusive start time of this slot
    int end;        // Exclusive end time of this slot
} GanttEntry;

// Maximum Gantt entries: upper bound for long/preemptive simulations
#define MAX_GANTT_ENTRIES 4096

// Renders a text-based Gantt chart from the completed process array
void print_gantt(const GanttEntry *gantt, int count);

#endif /* GANTT_H */