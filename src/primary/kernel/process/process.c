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
    // PID of the process
    u32 pid;
    // Parent PID of the process
    u32 ppid;
    // how many threads it's holding onto
    u32 thread_count;
    // the thread ids
    thread thread_ids[256];
} process;

process processes[256] = {0};
process* current_process = null;
u32 max_pid = 0;

void process_init() {
    memset(&processes[0], 0, sizeof(processes));
}

void process_start(char* path) {
    processes[0].pid = max_pid++;
    if (current_process) processes[0].ppid = current_process->pid;
    else processes[0].ppid = 0; // no parent


}