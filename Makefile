# schedsim CPU Scheduling Simulator

CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 -g
TARGET  = schedsim
BUILD_DIR = build

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

OBJS      = $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Include path ──────────────────────────────────────────────────────
INCLUDES = -I include

# Targets ───────────────────────────────────────────────────────────

.PHONY: all clean test run FCFS SJF STCF RR MLFQ compare

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ -lm

# Compile each src/*.c -> build/*.o
$(BUILD_DIR)/%.o: src/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

test: all
	@bash tests/test_suite.sh

# Per-algorithm shortcuts ───────────────────────────────────────────
FCFS: all
	./$(TARGET) --algorithm FCFS --input tests/workload1.txt

SJF: all
	./$(TARGET) --algorithm SJF --input tests/workload1.txt

STCF: all
	./$(TARGET) --algorithm STCF --input tests/workload1.txt

RR: all
	./$(TARGET) --algorithm RR --input tests/workload1.txt --quantum 2

MLFQ: all
	./$(TARGET) --algorithm MLFQ --input tests/workload1.txt

compare: all
	./$(TARGET) --compare --input tests/workload1.txt