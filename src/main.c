#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/process.h"
#include "../include/scheduler.h"

/* main.c ───────────────────────────────────────────────────────────
 * CLI entry point, argument parsing, scheduler dispatcher, and main loop
 * Supported algorithms (case-insensitive): FCFS, SJF, STCF, RR, MLFQ    */


// Forward Declarations ─────────────────────────────────────────────
// Forward declarations for functions defined below
static void parse_args(int argc, char *argv[],
                       char **algorithm, char **input, int *quantum);
static void dispatch(const char *algorithm,
                     Process processes[], int n, int quantum);

// Utility forward declaration (defined in utils.c)
void str_to_upper(char *s);

// Forward declarations for process module (defined in process.c)
int  load_processes(const char *filepath, Process processes[], int max);
void print_processes(const Process processes[], int n);

// main -------------------------------------------------------------
int main(int argc, char *argv[])
{
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

    // Dispatch to scheduler
    str_to_upper(algorithm);
    dispatch(algorithm, processes, n, quantum);

    return EXIT_SUCCESS;
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
static void dispatch(const char *algorithm,
                     Process processes[], int n, int quantum)
{
    if (strcmp(algorithm, "FCFS") == 0) {
        schedule_fcfs(processes, n, quantum);
    } else if (strcmp(algorithm, "SJF") == 0) {
        schedule_sjf(processes, n, quantum);
    } else if (strcmp(algorithm, "STCF") == 0) {
        schedule_stcf(processes, n, quantum);
    } else if (strcmp(algorithm, "RR") == 0) {
        schedule_rr(processes, n, quantum);
    } else if (strcmp(algorithm, "MLFQ") == 0) {
        schedule_mlfq(processes, n, quantum);
    } else {
        fprintf(stderr, "Error: unknown algorithm '%s'\n", algorithm);
        fprintf(stderr, "Valid options: FCFS, SJF, STCF, RR, MLFQ\n");
        exit(EXIT_FAILURE);
    }
}