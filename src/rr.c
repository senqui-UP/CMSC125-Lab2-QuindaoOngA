//Round Robin Implementation
#include <stdio.h>
#include "../include/process.h"
#include "../include/scheduler.h"

int schedule_rr(SchedulerState *state) {
    (void)state;
    printf("RR selected\n");
    return 0;
}