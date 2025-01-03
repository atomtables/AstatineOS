//
// Created by Adithiya Venkatakrishnan on 2/1/2025.
//

#include "process.h"

#include <modules/modules.h>

typedef struct { // threads are like stripped processes. only runs when a target tick is reached.
    void(* function)();
    u32 args[4]; // up to 4 provided args since void* is 4 bytes and numbers are also all 4 bytes.
    u32 ppid;
    u32 target_tick;
} thread;

typedef struct process { // processes can call on new threads
    u32 pid;
    u32 ppid;
    u32 thread_count;
    thread thread_ids[256];
} process;

static process processes[256];

void process_init() {
    memset(&processes[0], 0, sizeof(processes));
}

