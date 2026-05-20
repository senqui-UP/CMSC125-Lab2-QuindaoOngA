// Gantt Chart Rendering

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../include/gantt.h"
#include "../include/scheduler.h"

/* Reads the GanttEntry log from SchedulerState and renders a two-line ASCII timeline:
 *   [P1][P2][IDLE][P3]
 *   0   2   4     6   9
 *
 * Scaling:
 *   If total_time <= 80: 1 char per tick
 *   Else: scale = ceil(total_time / 80), each slice = max(1, dur / scale)
 * IDLE periods are rendered as [IDLE] and counted in the timeline. */

// print_gantt_chart ────────────────────────────────────────────────
void print_gantt_chart(SchedulerState *state)
{
    if (!state || state->gantt_count == 0) {
        printf("  (no Gantt data)\n");
        return;
    }

    int total = state->total_time;
    if (total <= 0) {
        // Fallback: derive from last entry
        total = state->gantt[state->gantt_count - 1].end_time;
    }

    // Determine scale
    int scale = 1;
    if (total > 80)
        scale = (int)ceil((double)total / 80.0);

    // Row 1: process blocks
    printf("  ");
    for (int i = 0; i < state->gantt_count; i++) {
        GanttEntry *e   = &state->gantt[i];
        int duration    = e->end_time - e->start_time;
        int width       = duration / scale;
        if (width < 1) width = 1;

        // Build label: "[PID]"
        char label[24];
        snprintf(label, sizeof(label), "[%s]", e->pid);

        int label_len = (int)strlen(label);

        if (width >= label_len) {
            // Label fits — pad with spaces on the right
            printf("%s", label);
            for (int s = label_len; s < width; s++) printf(" ");
        } else {
            // Not enough room — just print the label, overflow is fine
            printf("%s", label);
        }
    }
    printf("\n");

    // Row 2: time markers
    printf("  ");
    int cursor = 0;         // character position on the line

    for (int i = 0; i < state->gantt_count; i++) {
        GanttEntry *e   = &state->gantt[i];
        int duration    = e->end_time - e->start_time;
        int width       = duration / scale;
        if (width < 1) width = 1;

        // Print start time of this slice
        char timebuf[16];
        snprintf(timebuf, sizeof(timebuf), "%d", e->start_time);
        int tlen = (int)strlen(timebuf);

        printf("%s", timebuf);
        cursor += tlen;

        // Pad to end of this block
        int pad = width - tlen;
        for (int s = 0; s < pad; s++) { printf(" "); cursor++; }
    }

    // Print the final end time
    printf("%d", total);
    printf("\n");

    if (scale > 1)
        printf("  (scale: 1 char = %d ticks)\n", scale);
}