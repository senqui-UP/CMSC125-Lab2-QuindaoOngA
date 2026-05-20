// Metrics Calculation

#ifndef METRICS_H
#define METRICS_H

struct SchedulerState; /* forward declaration */
/* Print the full scheduler report:
 *   - algorithm header
 *   - per-process table (PID | AT | BT | FT | TT | WT | RT)
 *   - averages
 *   - context switches
 *   - CPU utilization
 *   - idle time
 *   - convoy effect warning (FCFS only)                                  */
void print_metrics_table(struct SchedulerState *state);

#endif /* METRICS_H */