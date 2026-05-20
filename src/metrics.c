// Scheduling Metrics Calculation

#include <stdio.h>
#include <string.h>
#include "../include/metrics.h"
#include "../include/scheduler.h"

/* --------------------------------------------------------------------------
 * Output format:
 *   Algorithm: FCFS
 *   Process | AT | BT | FT | TT | WT | RT
 *   ----------------------------------------
 *   P1      | 0  | 8  | 8  | 8  | 0  | 0
 *   ...
 *   ----------------------------------------
 *   Averages:
 *   TT: 15.25 | WT: 8.75 | RT: 8.75
 *
 *   Context Switches : 0
 *   CPU Utilization  : 100.00%
 *   Idle Time        : 0 ticks
 *
 *   [convoy warning if applicable]
 * -------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------
 * convoy_check — FCFS only
 *   first process BT > avg_BT * 2
 *   AND at least 2 later processes have BT < first process BT
 * ---------------------------------------------------------------------- */
static void convoy_check(SchedulerState *state)
{
    if (strncmp(state->algorithm, "FCFS", 4) != 0)
        return;

    int n = state->num_processes;
    if (n < 3) return;                  // need at least 3 for a meaningful convoy

    // Find the process with the earliest arrival (first to run)
    Process *first = &state->processes[0];
    for (int i = 1; i < n; i++) {
        Process *p = &state->processes[i];
        if (p->arrival_time < first->arrival_time ||
            (p->arrival_time == first->arrival_time &&
             strcmp(p->pid, first->pid) < 0))
            first = p;
    }

    // Average burst time across all processes
    double avg_bt = 0.0;
    for (int i = 0; i < n; i++)
        avg_bt += state->processes[i].burst_time;
    avg_bt /= n;

    // Count later processes with shorter burst than the first
    int shorter_count = 0;
    for (int i = 0; i < n; i++) {
        if (&state->processes[i] == first) continue;
        if (state->processes[i].burst_time < first->burst_time)
            shorter_count++;
    }

    if ((double)first->burst_time > avg_bt * 2.0 && shorter_count >= 2) {
        printf("\n  *** Potential Convoy Effect Detected ***\n");
        printf("  First process (P%s) BT=%d > 2 x avg BT (%.1f).\n",
               first->pid, first->burst_time, avg_bt);
        printf("  %d shorter job(s) waited behind it.\n", shorter_count);
    }
}

// print_metrics_table ----------------------------------------------
void print_metrics_table(SchedulerState *state)
{
    if (!state || state->num_processes <= 0) return;

    int n = state->num_processes;

    // Header
    printf("\nAlgorithm: %s", state->algorithm);
    if (strncmp(state->algorithm, "RR", 2) == 0)
        printf(" (Quantum: %d)", state->quantum);
    printf("\n\n");

    // Table header
    printf("%-8s | %-4s | %-4s | %-4s | %-4s | %-4s | %-4s\n",
           "Process", "AT", "BT", "FT", "TT", "WT", "RT");
    printf("%-8s-+-%-4s-+-%-4s-+-%-4s-+-%-4s-+-%-4s-+-%-4s\n",
           "--------", "----", "----", "----", "----", "----", "----");

    double total_tt = 0, total_wt = 0, total_rt = 0;

    // Sort a local index array by PID for consistent display
    int order[MAX_PROCESSES];
    for (int i = 0; i < n; i++) order[i] = i;
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (strcmp(state->processes[order[j]].pid,
                       state->processes[order[i]].pid) < 0) {
                int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
            }
        }
    }

    for (int i = 0; i < n; i++) {
        Process *p = &state->processes[order[i]];
        int rt     = p->start_time - p->arrival_time;
        char plabel[20];
        snprintf(plabel, sizeof(plabel), "P%s", p->pid);
        printf("%-8s | %-4d | %-4d | %-4d | %-4d | %-4d | %-4d\n",
               plabel,
               p->arrival_time, p->burst_time,
               p->finish_time, p->turnaround_time, p->waiting_time, rt);
        total_tt += p->turnaround_time;
        total_wt += p->waiting_time;
        total_rt += rt;
    }

    printf("%-8s-+-%-4s-+-%-4s-+-%-4s-+-%-4s-+-%-4s-+-%-4s\n",
           "--------", "----", "----", "----", "----", "----", "----");

    // Averages
    printf("Averages:\n");
    printf("  TT: %.2f | WT: %.2f | RT: %.2f\n\n",
           total_tt / n, total_wt / n, total_rt / n);

    // Statistics
    printf("  Context Switches : %d\n", state->context_switches);

    // CPU Utilization
    if (state->total_time > 0) {
        double utilization = 100.0 *
            (double)(state->total_time - state->idle_time) /
            (double)state->total_time;
        printf("  CPU Utilization  : %.2f%%\n", utilization);
    }
    printf("  Idle Time        : %d ticks\n", state->idle_time);

    if (state->boosts > 0)
        printf("  Priority Boosts  : %d\n", state->boosts);

    // Convoy detection (FCFS only)
    convoy_check(state);

    // Gantt chart
    printf("\nGantt Chart:\n");
}

// extract_result ---------------------------------------------------
// Build a ComparisonResult snapshot from a completed SchedulerState
ComparisonResult extract_result(SchedulerState *state)
{
    ComparisonResult r;
    snprintf(r.algorithm, sizeof(r.algorithm), "%s", state->algorithm);
 
    int n = state->num_processes;
    double total_tt = 0, total_wt = 0, total_rt = 0;
    for (int i = 0; i < n; i++) {
        total_tt += state->processes[i].turnaround_time;
        total_wt += state->processes[i].waiting_time;
        total_rt += state->processes[i].start_time
                    - state->processes[i].arrival_time;
    }
    r.avg_tt = total_tt / n;
    r.avg_wt = total_wt / n;
    r.avg_rt = total_rt / n;
    r.context_switches = state->context_switches;
    r.cpu_utilization  = (state->total_time > 0)
        ? 100.0 * (double)(state->total_time - state->idle_time)
                / (double)state->total_time
        : 0.0;
    return r;
}
 
// print_comparison_table -------------------------------------------
void print_comparison_table(const ComparisonResult *results, int count,
                            const char *input_file, int quantum)
{
    printf("\n=== Scheduler Comparison");
    if (input_file)
        printf(" (input: %s, quantum: %d)", input_file, quantum);
    printf(" ===\n\n");
 
    // Column header
    printf("%-12s | %-9s | %-9s | %-9s | %-5s | %-9s\n",
           "Algorithm", "Avg TT", "Avg WT", "Avg RT", "CS", "CPU%");
    printf("%-12s-+-%-9s-+-%-9s-+-%-9s-+-%-5s-+-%-9s\n",
           "------------", "---------", "---------",
           "---------", "-----", "---------");
 
    // Find best values per column 
    double best_tt = results[0].avg_tt;
    double best_wt = results[0].avg_wt;
    double best_rt = results[0].avg_rt;
    int    best_tt_idx = 0, best_wt_idx = 0, best_rt_idx = 0;
 
    for (int i = 1; i < count; i++) {
        if (results[i].avg_tt < best_tt) { best_tt = results[i].avg_tt; best_tt_idx = i; }
        if (results[i].avg_wt < best_wt) { best_wt = results[i].avg_wt; best_wt_idx = i; }
        if (results[i].avg_rt < best_rt) { best_rt = results[i].avg_rt; best_rt_idx = i; }
    }
 
    // Print rows 
    for (int i = 0; i < count; i++) {
        const ComparisonResult *r = &results[i];
 
        char tt_buf[16], wt_buf[16], rt_buf[16];
        snprintf(tt_buf, sizeof(tt_buf), "%.2f%s",
                 r->avg_tt, (i == best_tt_idx) ? "*" : " ");
        snprintf(wt_buf, sizeof(wt_buf), "%.2f%s",
                 r->avg_wt, (i == best_wt_idx) ? "*" : " ");
        snprintf(rt_buf, sizeof(rt_buf), "%.2f%s",
                 r->avg_rt, (i == best_rt_idx) ? "*" : " ");
 
        printf("%-12s | %-9s | %-9s | %-9s | %-5d | %.2f%%\n",
               r->algorithm, tt_buf, wt_buf, rt_buf,
               r->context_switches, r->cpu_utilization);
    }
 
    printf("%-12s-+-%-9s-+-%-9s-+-%-9s-+-%-5s-+-%-9s\n",
           "------------", "---------", "---------",
           "---------", "-----", "---------");
 
    // Best row
    printf("%-12s | %-9s | %-9s | %-9s\n",
           "Best",
           results[best_tt_idx].algorithm,
           results[best_wt_idx].algorithm,
           results[best_rt_idx].algorithm);
    printf("\n  * = best in column\n");
}