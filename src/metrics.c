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
 * Heuristic:
 *   first process BT > avg_BT * 2
 *   AND at least 2 later processes have BT < first process BT
 * ---------------------------------------------------------------------- */
static void convoy_check(SchedulerState *state)
{
    if (strncmp(state->algorithm, "FCFS", 4) != 0)
        return;

    int n = state->num_processes;
    if (n < 3) return; /* need at least 3 for a meaningful convoy */

    /* Find the process with the earliest arrival (first to run) */
    Process *first = &state->processes[0];
    for (int i = 1; i < n; i++)
        if (state->processes[i].arrival_time < first->arrival_time ||
            (state->processes[i].arrival_time == first->arrival_time &&
             state->processes[i].pid < first->pid))
            first = &state->processes[i];

    /* Average burst time across all processes */
    double avg_bt = 0.0;
    for (int i = 0; i < n; i++)
        avg_bt += state->processes[i].burst_time;
    avg_bt /= n;

    /* Count later processes with shorter burst than the first */
    int shorter_count = 0;
    for (int i = 0; i < n; i++) {
        if (&state->processes[i] == first) continue;
        if (state->processes[i].burst_time < first->burst_time)
            shorter_count++;
    }

    if ((double)first->burst_time > avg_bt * 2.0 && shorter_count >= 2) {
        printf("\n  *** Potential Convoy Effect Detected ***\n");
        printf("  First process (P%d) BT=%d > 2 x avg BT (%.1f).\n",
               first->pid, first->burst_time, avg_bt);
        printf("  %d shorter job(s) waited behind it.\n", shorter_count);
    }
}

/* --------------------------------------------------------------------------
 * print_metrics_table
 * ---------------------------------------------------------------------- */
void print_metrics_table(SchedulerState *state)
{
    if (!state || state->num_processes <= 0) return;

    int n = state->num_processes;

    /* -- Header ---------------------------------------------------------- */
    printf("\nAlgorithm: %s", state->algorithm);
    if (strncmp(state->algorithm, "RR", 2) == 0)
        printf(" (Quantum: %d)", state->quantum);
    printf("\n\n");

    /* -- Table header ---------------------------------------------------- */
    printf("%-8s | %-4s | %-4s | %-4s | %-4s | %-4s | %-4s\n",
           "Process", "AT", "BT", "FT", "TT", "WT", "RT");
    printf("%-8s-+-%-4s-+-%-4s-+-%-4s-+-%-4s-+-%-4s-+-%-4s\n",
           "--------", "----", "----", "----", "----", "----", "----");

    double total_tt = 0, total_wt = 0, total_rt = 0;

    /* Sort a local index array by PID for consistent display */
    int order[MAX_PROCESSES];
    for (int i = 0; i < n; i++) order[i] = i;
    for (int i = 0; i < n - 1; i++)
        for (int j = i + 1; j < n; j++)
            if (state->processes[order[j]].pid < state->processes[order[i]].pid) {
                int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
            }

    for (int i = 0; i < n; i++) {
        Process *p = &state->processes[order[i]];
        int rt     = p->start_time - p->arrival_time;
        char plabel[16];
        snprintf(plabel, sizeof(plabel), "P%d", p->pid);
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

    /* -- Averages -------------------------------------------------------- */
    printf("Averages:\n");
    printf("  TT: %.2f | WT: %.2f | RT: %.2f\n\n",
           total_tt / n, total_wt / n, total_rt / n);

    /* -- Statistics ------------------------------------------------------ */
    printf("  Context Switches : %d\n", state->context_switches);

    /* CPU Utilization */
    if (state->total_time > 0) {
        double utilization = 100.0 *
            (double)(state->total_time - state->idle_time) /
            (double)state->total_time;
        printf("  CPU Utilization  : %.2f%%\n", utilization);
    }
    printf("  Idle Time        : %d ticks\n", state->idle_time);

    if (state->boosts > 0)
        printf("  Priority Boosts  : %d\n", state->boosts);

    /* -- Convoy detection (FCFS only) ------------------------------------ */
    convoy_check(state);

    /* -- Gantt chart ----------------------------------------------------- */
    printf("\nGantt Chart:\n");
}