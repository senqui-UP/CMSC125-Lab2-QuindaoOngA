// Process Loading and Management

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/process.h"

/* parses workload file into a Process array
 * prints loaded processes (debug aid)
 *
 * Workload file format (one process per line, whitespace-separated):
 *   <PID>  <ArrivalTime>  <BurstTime> */


/* Reads the workload file at `filepath` into `processes[]`.
 * Returns the number of processes loaded, or -1 on error.               */
int load_processes(const char *filepath, Process processes[], int max) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        fprintf(stderr, "Error: cannot open input file '%s'\n", filepath);
        return -1;
    }

    int count = 0;
    char line[256];

    while (fgets(line, sizeof(line), fp)) {
        // Skip comment lines and blank lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;

        if (count >= max) {
            fprintf(stderr, "Warning: workload exceeds MAX_PROCESSES (%d). "
                            "Truncating.\n", max);
            break;
        }

        Process *p = &processes[count];

        // Zero-initialise so computed/runtime fields start clean
        memset(p, 0, sizeof(Process));

        if (sscanf(line, "%15s %d %d",
                   p->pid,
                   &p->arrival_time,
                   &p->burst_time) != 3) {
            fprintf(stderr, "Warning: skipping malformed line: %s", line);
            continue;
        }

        // Initialise remaining_time to burst_time for algorithms
        p->remaining_time  = p->burst_time;
        p->priority        = 0;              // MLFQ starts all processes in queue 0        
        p->time_in_queue   = 0;
        p->start_time      = -1;             // -1 = not yet scheduled     
        p->finish_time     = 0;
        p->turnaround_time = 0;
        p->waiting_time    = 0;

        count++;
    }

    fclose(fp);
    return count;
}

// Dumps the loaded process table to stdout for debugging.
void print_processes(const Process processes[], int n) {
    printf("\n%-6s %-14s %-12s\n", "PID", "Arrival Time", "Burst Time");
    printf("%-6s %-14s %-12s\n", "---", "------------", "----------");
    for (int i = 0; i < n; i++) {
        printf("%-6s %-14d %-12d\n",
               processes[i].pid,
               processes[i].arrival_time,
               processes[i].burst_time);
    }
    printf("\n");
}