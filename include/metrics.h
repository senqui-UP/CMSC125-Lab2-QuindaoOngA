// Metrics Calculation

#ifndef METRICS_H
#define METRICS_H

struct SchedulerState;
 
// Per-algorithm result snapshot for compare mode 
typedef struct {
    char   algorithm[32];
    double avg_tt;
    double avg_wt;
    double avg_rt;
    int    context_switches;
    double cpu_utilization;
} ComparisonResult;
 
// Full single-algorithm report (algorithm mode)    
void print_metrics_table(struct SchedulerState *state);
 
// Comparison table across all algorithms (compare mode)
void print_comparison_table(const ComparisonResult *results, int count,
                            const char *input_file, int quantum);
 
// Extract a ComparisonResult from a completed SchedulerState 
ComparisonResult extract_result(struct SchedulerState *state);
 
#endif /* METRICS_H */