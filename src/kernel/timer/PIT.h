//
// Created by Adithiya Venkatakrishnan on 31/8/2024.
//

#ifndef PIT_H
#define PIT_H

#include <modules/modules.h>

#define PIT_A 0x40
#define PIT_B 0x41
#define PIT_C 0x42
#define PIT_CONTROL 0x43

#define PIT_MASK 0xFF
#define PIT_SET 0x36

#define PIT_HZ 1193181
#define DIV_OF_FREQ(_f) (PIT_HZ / (_f))
#define FREQ_OF_DIV(_d) (PIT_HZ / (_d))
#define REAL_FREQ_OF_FREQ(_f) (FREQ_OF_DIV(DIV_OF_FREQ((_f))))

// number chosen to be integer divisor of PIC frequency
#define TIMER_TPS 500

typedef struct {
    u64 start;
    u64 end;
    void(* ret)();
} PACKED process_wait_state;

u64 timer_get();
void timer_init();

int run_every_second(void(* ret)());
void stop_run_every_second(int i);

void sleep(int ms);
int wait_and_do(const u64 ms, void(* ret)()); // legacy version ig?

#endif //PIT_H
