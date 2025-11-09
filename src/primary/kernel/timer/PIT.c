//
// Created by Adithiya Venkatakrishnan on 31/8/2024.
//

#include "PIT.h"

#include <display/simple/display.h>
#include <exception/exception.h>
#include <interrupt/interrupt.h>

static struct state {
    u64 frequency;
    u64 divisor;
    u64 ticks;
} state;

/// when the kernel sleeps, it pauses until an interrupt is received
/// so it is only necessary to store the start and end times once.
/// incase a sleep is needed during interrupt handling, there will be a separate
/// sleep_state struct
static struct sleep_state {
    volatile u64 start;
    volatile u64 end;
    volatile bool active;
} sleep_state;

process_wait_state process_wait_states[256];

void(* every_second_handlers[256])() = {null};

int add_process_wait_state(u64 start, u64 end, void(* ret)()) {
    for (int i = 0; i < 256; i++) {
        if (process_wait_states[i].start == (u64)-1) {
            process_wait_states[i].start = start;
            process_wait_states[i].end = end;
            process_wait_states[i].ret = ret;
            return i;
        }
    }
    return -1;
}

int run_every_second(void(* ret)()) {
    for (int i = 0; i < 256; i++) {
        if (every_second_handlers[i] == null) {
            every_second_handlers[i] = ret;
            return i;
        }
    }
    return -1;
}

void stop_run_every_second(int i) {
    every_second_handlers[i] = null;
}

int wait_and_do(const u64 ms, void(* ret)()) {
    const u64 start = timer_get();
    const u64 end = start + ms;
    return add_process_wait_state(start, end, ret);
}

static void timer_set(int hz) {
    outportb(PIT_CONTROL, PIT_SET);

    u16 d = (u16)(1193131.666 / hz);
    outportb(PIT_A, d & PIT_MASK);
    outportb(PIT_A, d >> 8 & PIT_MASK);
}

u64 timer_get() { return state.ticks; }

static u8 local_ticks = 0;

static void timer_handler(struct registers* regs) {
    // should be set to 1 but QEMU's PIT is inaccurate
    if (local_ticks++ == 1) {
        local_ticks = 0;
        state.ticks += 1;
    }
    if (sleep_state.active) {
        if ((u32)state.ticks >= (u32)sleep_state.end) {
            sleep_state.active = false;
        }
    }
    // need a better solution than this.
    for (int i = 0; i < 256; i++) {
        if (process_wait_states[i].start != (u64)-1 && state.ticks >= process_wait_states[i].end) {
            process_wait_states[i].ret();
            process_wait_states[i].start = (u64)-1;
            process_wait_states[i].end = (u64)-1;
            process_wait_states[i].ret = null;
        }
        if (((u32)state.ticks) % 1000 == 0 && every_second_handlers[i] != null) {
            every_second_handlers[i]();
        }
    }
}

void sleep(int ms) {
    u64 ticks = timer_get();

    sleep_state.active = true;
    sleep_state.start = ticks;
    sleep_state.end = ticks + ms;
    while (1) {
        asm ("nop");
        if (!sleep_state.active)
            break;
    }
}

void timer_init() {
    // initialise the process wait storage
    for (int i = 0; i < 256; i++) {
        process_wait_states[i].start = -1;
        process_wait_states[i].end = -1;
        process_wait_states[i].ret = null;
        every_second_handlers[i] = null;
    }

    const u64 freq = REAL_FREQ_OF_FREQ(TIMER_TPS);
    display.printf("PIT frequency set to %u Hz\n", freq);
    state.frequency = freq;
    state.divisor = DIV_OF_FREQ(freq);
    state.ticks = 0;
    sleep_state.end = 0;
    sleep_state.active = false;
    timer_set(freq);
    PIC_install(0, timer_handler);
}
