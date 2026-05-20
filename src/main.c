#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/process.h"
#include "../include/scheduler.h"
#include "../include/gantt.h"
#include "../include/metrics.h"

/* main.c ───────────────────────────────────────────────────────────
 * CLI entry point, argument parsing, scheduler dispatcher, and main loop
 * Supported algorithms (case-insensitive): FCFS, SJF, STCF, RR, MLFQ
 * Flow:
 *   parse_args()
 *   load_processes()
 *   build SchedulerState
 *   dispatch → algorithm simulates, populates state
 *   print_metrics_table(&state)   ← metrics.c
 *   print_gantt_chart(&state)     ← gantt.c          */


// Forward Declarations ─────────────────────────────────────────────
static void parse_args(int argc, char *argv[],
                       char **algorithm, char **input, int *quantum);
static int  dispatch(const char *algorithm, SchedulerState *state);

// Utility (defined in utils.c)
void str_to_upper(char *s);

// For process module (defined in process.c)
int  load_processes(const char *filepath, Process processes[], int max);
void print_processes(const Process processes[], int n);

// main -------------------------------------------------------------
int main(int argc, char *argv[]) {
    char *algorithm = NULL;
    char *input     = NULL;
    int   quantum   = 1;        // default for RR

    parse_args(argc, argv, &algorithm, &input, &quantum);

    // Basic validation for now
    if (!algorithm) {
        fprintf(stderr, "Error: --algorithm is required\n");
        return EXIT_FAILURE;
    }
    if (!input) {
        fprintf(stderr, "Error: --input is required\n");
        return EXIT_FAILURE;
    }

    // Load workload
    Process processes[MAX_PROCESSES];
    int n = load_processes(input, processes, MAX_PROCESSES);
    if (n <= 0) {
        fprintf(stderr, "Error: no processes loaded from '%s'\n", input);
        return EXIT_FAILURE;
    }

    printf("Loaded %d process(es) from '%s'\n", n, input);
    print_processes(processes, n);

    // Build SchedulerState
    GanttEntry gantt[MAX_GANTT_ENTRIES];
    memset(gantt, 0, sizeof(gantt));

    str_to_upper(algorithm);

    SchedulerState state;
    memset(&state, 0, sizeof(state));
    state.processes      = processes;
    state.num_processes  = n;
    state.quantum        = quantum;
    state.gantt          = gantt;
    state.gantt_count    = 0;
    state.gantt_capacity = MAX_GANTT_ENTRIES;
    state.idle_time      = 0;
    state.context_switches = 0;
    state.boosts         = 0;
    snprintf(state.algorithm, sizeof(state.algorithm), "%s", algorithm);

    // Dispatch — algorithm simulates, populates state
    int result = dispatch(algorithm, &state);
    if (result != 0) return EXIT_FAILURE;

    // Output — centralized in metrics.c and gantt.c
    print_metrics_table(&state);
    print_gantt_chart(&state);

    return (result == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* parse_args -------------------------------------------------------
 * Walks argv looking for --algorithm, --input, --quantum.
 * Unknown flags are silently ignored here; */
static void parse_args(int argc, char *argv[],
                       char **algorithm, char **input, int *quantum)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--algorithm") == 0 && i + 1 < argc) {
            *algorithm = argv[++i];
        } else if (strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
            *input = argv[++i];
        } else if (strcmp(argv[i], "--quantum") == 0 && i + 1 < argc) {
            *quantum = atoi(argv[++i]);
        }
    }
}

/* dispatch ---------------------------------------------------------
 * Maps the algorithm string to the correct schedule_*() function */
static int dispatch(const char *algorithm, SchedulerState *state)
{
    if (strcmp(algorithm, "FCFS") == 0) {
        return schedule_fcfs(state);
    } else if (strcmp(algorithm, "SJF") == 0) {
        return schedule_sjf(state);
    } else if (strcmp(algorithm, "STCF") == 0) {
        return schedule_stcf(state);
    } else if (strcmp(algorithm, "RR") == 0) {
        return schedule_rr(state);
    } else if (strcmp(algorithm, "MLFQ") == 0) {
        return schedule_mlfq(state);
    } else {
        fprintf(stderr, "Error: unknown algorithm '%s'\n", algorithm);
        fprintf(stderr, "Valid options: FCFS, SJF, STCF, RR, MLFQ\n");
        return -1;
    }
}