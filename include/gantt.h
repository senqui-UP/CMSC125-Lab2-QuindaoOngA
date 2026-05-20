// Gantt Chart Generation

#ifndef GANTT_H
#define GANTT_H

typedef struct {
    char pid[16];       // Process label e.g. "1", "12", or "IDLE"
    int  start_time;
    int  end_time;
    int  queue_level;   // MLFQ queue level at time of execution; -1 = N/A
} GanttEntry;

// Upper bound for Gantt log
#define MAX_GANTT_ENTRIES 4096

// Render ASCII Gantt chart from the entries stored in SchedulerState.
// forward-declared here to avoid circular include; implementation in src/gantt.c
struct SchedulerState;
void print_gantt_chart(struct SchedulerState *state);

#endif /* GANTT_H */