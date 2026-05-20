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
static void  print_help(const char *prog);
static void  parse_args(int argc, char *argv[], char **algorithm,
                        char **input, int *quantum, int *compare);
static void  validate_args(const char *algorithm, const char *input,
                           int compare, int quantum, const char *prog);
static void  init_scheduler_state(SchedulerState *state,
                                  Process *processes, int n,
                                  GanttEntry *gantt, int capacity,
                                  int quantum, const char *algorithm,
                                  int verbose);
static int   run_algorithm(const char *algorithm, SchedulerState *state);
static void  run_compare(Process *original, int n, int quantum,
                         const char *input_file);

// Utility (defined in utils.c)
void str_to_upper(char *s);

// For process module (defined in process.c)
int  load_processes(const char *filepath, Process processes[], int max);
void print_processes(const Process processes[], int n);
void copy_processes(const Process *src, Process *dst, int n);

// main -------------------------------------------------------------
int main(int argc, char *argv[]) {
    char *algorithm = NULL;
    char *input     = NULL;
    int   quantum   = 2;        // default for RR
    int   compare   = 0;        // default for comparison mode

    parse_args(argc, argv, &algorithm, &input, &quantum, &compare);
    validate_args(algorithm, input, compare, quantum, argv[0]);

    // Uppercase algorithm for consistency
    if (algorithm) str_to_upper(algorithm);

    // Load workload
    Process processes[MAX_PROCESSES];
    int n = load_processes(input, processes, MAX_PROCESSES);
    if (n <= 0) {
        fprintf(stderr, "Error: No processes loaded from '%s'.\n", input);
        return EXIT_FAILURE;
    }
 
    printf("Loaded %d process(es) from '%s'\n", n, input);
    print_processes(processes, n);
 
    // Dispatch
    if (compare) {
        run_compare(processes, n, quantum, input);
    } else {
        GanttEntry gantt[MAX_GANTT_ENTRIES];
        SchedulerState state;
        init_scheduler_state(&state, processes, n, gantt,
                             MAX_GANTT_ENTRIES, quantum, algorithm, 1);
 
        int result = run_algorithm(algorithm, &state);
        if (result != 0) return EXIT_FAILURE;
 
        print_metrics_table(&state);
        print_gantt_chart(&state);
    }
 
    return EXIT_SUCCESS;
}

/* init_scheduler_state ---------------------------------------------
/ Initialise every field of a SchedulerState cleanly.
/ Call before every algorithm run — never assume zero-init. */
static void init_scheduler_state(SchedulerState *state,
                                 Process *processes, int n,
                                 GanttEntry *gantt, int capacity,
                                 int quantum, const char *algorithm,
                                 int verbose) {
    state->processes          = processes;
    state->num_processes      = n;
    state->quantum            = quantum;
    state->gantt              = gantt;
    state->gantt_count        = 0;
    state->gantt_capacity     = capacity;
    state->current_time       = 0;
    state->total_time         = 0;
    state->idle_time          = 0;
    state->context_switches   = 0;
    state->completed_processes = 0;
    state->boosts             = 0;
    state->verbose            = verbose;
    snprintf(state->algorithm, sizeof(state->algorithm), "%s", algorithm);
}

// run_algorithm ----------------------------------------------------
// Dispatch to the correct schedule_*() function by name
static int run_algorithm(const char *algorithm, SchedulerState *state) {
    if      (strcmp(algorithm, "FCFS") == 0) return schedule_fcfs(state);
    else if (strcmp(algorithm, "SJF")  == 0) return schedule_sjf(state);
    else if (strcmp(algorithm, "STCF") == 0) return schedule_stcf(state);
    else if (strcmp(algorithm, "RR")   == 0) return schedule_rr(state);
    else if (strcmp(algorithm, "MLFQ") == 0) return schedule_mlfq(state);
    else {
        fprintf(stderr, "Error: Unknown algorithm '%s'.\n", algorithm);
        fprintf(stderr, "       Valid options: FCFS, SJF, STCF, RR, MLFQ\n");
        return -1;
    }
}
 
/* run_compare ------------------------------------------------------
/ Run all 5 algos on independent copies of the workload.
/ Print only the comparison table — no individual reports */
static void run_compare(Process *original, int n, int quantum,
                        const char *input_file) {
    // RR label includes quantum for clarity in the table
    char rr_label[32];
    snprintf(rr_label, sizeof(rr_label), "RR (q=%d)", quantum);
 
    const char *algorithms[] = { "FCFS", "SJF", "STCF", rr_label, "MLFQ" };
    const char *dispatch[]   = { "FCFS", "SJF", "STCF", "RR",     "MLFQ" };
    int count = 5;
 
    ComparisonResult results[5];
 
    for (int i = 0; i < count; i++) {
        // Fresh process copy — base fields only, runtime fields clean
        Process copy[MAX_PROCESSES];
        copy_processes(original, copy, n);
 
        // Fresh Gantt buffer
        GanttEntry gantt[MAX_GANTT_ENTRIES];
 
        // Fresh state — verbose=0 suppresses MLFQ trace and report output
        SchedulerState state;
        init_scheduler_state(&state, copy, n, gantt, MAX_GANTT_ENTRIES,
                             quantum, algorithms[i], 0);
 
        int ret = run_algorithm(dispatch[i], &state);
        if (ret != 0) {
            fprintf(stderr, "Warning: %s failed — skipping.\n", algorithms[i]);
            snprintf(results[i].algorithm, sizeof(results[i].algorithm),
                     "%s (ERR)", algorithms[i]);
            results[i].avg_tt = results[i].avg_wt = results[i].avg_rt = 0.0;
            results[i].context_switches = 0;
            results[i].cpu_utilization  = 0.0;
            continue;
        }
 
        results[i] = extract_result(&state);
        // Override algorithm label to include RR quantum
        snprintf(results[i].algorithm, sizeof(results[i].algorithm),
                 "%s", algorithms[i]);
    }
 
    print_comparison_table(results, count, input_file, quantum);
}

/* parse_args -------------------------------------------------------
/ Walks argv looking for --algorithm, --input, --quantum.
/ Unknown flags are silently ignored here; */
static void parse_args(int argc, char *argv[], char **algorithm,
                       char **input, int *quantum, int *compare) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_help(argv[0]);
            exit(EXIT_SUCCESS);
        } else if (strcmp(argv[i], "--algorithm") == 0 && i + 1 < argc) {
            *algorithm = argv[++i];
        } else if (strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
            *input = argv[++i];
        } else if (strcmp(argv[i], "--quantum") == 0 && i + 1 < argc) {
            *quantum = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--compare") == 0) {
            *compare = 1;
        } else if (strncmp(argv[i], "--input=", 8) == 0) {
            *input = argv[i] + 8;
        } else if (strncmp(argv[i], "--algorithm=", 12) == 0) {
            *algorithm = argv[i] + 12;
        } else if (strncmp(argv[i], "--quantum=", 10) == 0) {
            *quantum = atoi(argv[i] + 10);
        }
    }
}

// validate_args ----------------------------------------------------
static void validate_args(const char *algorithm, const char *input,
                          int compare, int quantum, const char *prog) {
    int err = 0;
 
    if (!input) {
        fprintf(stderr, "Error: --input is required.\n");
        fprintf(stderr, "       Run '%s --help' for usage.\n", prog);
        err = 1;
    }
 
    if (!compare && !algorithm) {
        fprintf(stderr, "Error: --algorithm or --compare is required.\n");
        fprintf(stderr, "       Run '%s --help' for usage.\n", prog);
        err = 1;
    }
 
    // Validate algorithm name if provided and not in compare mode
    if (!compare && algorithm) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%s", algorithm);
        str_to_upper(tmp);
        if (strcmp(tmp, "FCFS") != 0 && strcmp(tmp, "SJF")  != 0 &&
            strcmp(tmp, "STCF") != 0 && strcmp(tmp, "RR")   != 0 &&
            strcmp(tmp, "MLFQ") != 0) {
            fprintf(stderr, "Error: Unknown algorithm '%s'.\n", algorithm);
            fprintf(stderr, "       Valid options: FCFS, SJF, STCF, RR, MLFQ\n");
            err = 1;
        }
 
        // RR-specific quantum check
        if (strcmp(tmp, "RR") == 0 && quantum <= 0) {
            fprintf(stderr, "Error: RR quantum must be greater than 0"
                            " (got %d).\n", quantum);
            err = 1;
        }
    }
 
    if (err) exit(EXIT_FAILURE);
}
 
// print help -------------------------------------------------------
static void print_help(const char *prog)
{
    printf("Usage: %s [OPTIONS]\n\n", prog);
    printf("Options:\n");
    printf("  --algorithm <ALG>   Scheduling algorithm to run\n");
    printf("                      Choices: FCFS, SJF, STCF, RR, MLFQ\n");
    printf("  --input <file>      Workload input file (required)\n");
    printf("  --quantum <n>       Time quantum for RR/MLFQ (default: 2)\n");
    printf("  --compare           Run all algorithms and print comparison\n");
    printf("  --help, -h          Show this help message\n");
    printf("\n");
    printf("Workload file format (one process per line):\n");
    printf("  <PID> <ArrivalTime> <BurstTime>\n");
    printf("  Lines starting with '#' are treated as comments.\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s --algorithm FCFS --input tests/workload1.txt\n", prog);
    printf("  %s --algorithm RR --input tests/workload1.txt --quantum 4\n", prog);
    printf("  %s --algorithm MLFQ --input tests/workload1.txt\n", prog);
    printf("  %s --compare --input tests/workload1.txt\n", prog);
    printf("  %s --compare --input tests/workload1.txt --quantum 3\n", prog);
    printf("\n");
    printf("Output:\n");
    printf("  --algorithm mode : full report + Gantt chart\n");
    printf("  --compare mode   : comparison table only\n");
}