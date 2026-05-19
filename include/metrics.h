// Metrics Calculation

#ifndef METRICS_H
#define METRICS_H

#include "process.h"

// Calculates and populates finish_time, turnaround_time, waiting_time for each process
// Called after a scheduling algorithm completes.
void calculate_metrics(Process processes[], int n);

// Prints summary table of per-process metrics and averages
void print_metrics(const Process processes[], int n);

#endif /* METRICS_H */