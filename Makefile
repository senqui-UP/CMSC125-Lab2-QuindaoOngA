# schedsim CPU Scheduling Simulator

CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 -g
TARGET  = schedsim

# Source files ──────────────────────────────────────────────────────
SRCS    = src/main.c     \
          src/process.c  \
          src/fcfs.c     \
          src/sjf.c      \
          src/stcf.c     \
          src/rr.c       \
          src/mlfq.c     \
          src/metrics.c  \
          src/gantt.c    \
          src/utils.c

OBJS    = $(SRCS:.c=.o)

# Include path ──────────────────────────────────────────────────────
INCLUDES = -I include

# Targets ───────────────────────────────────────────────────────────

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

# Pattern rule: compile each .c to .o
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

# Convenience target — edit args as needed
run: all
	./$(TARGET) --algorithm FCFS --input tests/workload1.txt