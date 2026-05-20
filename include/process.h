#ifndef PROCESS_H
#define PROCESS_H

// Process Data Structure ───────────────────────────────────────────

typedef struct {
    // Input fields (populated by workload parser) ------------------
    char pid[16];                // Process ID   
    int arrival_time;       // Clock time when process enters the ready queue  
    int burst_time;         // Total CPU time required 

    //Runtime fields (used during simulation by algorithms) ---------
    int remaining_time;     // Remaining burst — used by STCF and RR           
    int priority;           // Queue level — used by MLFQ                      
    int time_in_queue;      // Time spent in current MLFQ queue level   
    int queue_level;        // Current MLFQ queue (0 = highest priority)      */
    int quantum_used;       // Ticks used in current slice (resets on preempt)*/


    // Computed fields (written by metrics module) ------------------
    int start_time;        // Clock time of first CPU execution (-1 = unset)
    int finish_time;        // Clock time when process finishes    
    int turnaround_time;    // finish_time - arrival_time      
    int waiting_time;       // turnaround_time - burst_time     
} Process;

// Maximum number of processes supported in a single workload
#define MAX_PROCESSES 256

#endif /* PROCESS_H */