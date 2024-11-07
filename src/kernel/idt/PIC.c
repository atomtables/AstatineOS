//
// Created by Adithiya Venkatakrishnan on 27/07/2024.
//

#include "pic.h"

#include <idt/idt.h>
#include <idt/isr.h>

// PIC constants
#define PIC1 0x20
#define PIC1_OFFSET 0x20
#define PIC1_DATA (PIC1 + 1)

#define PIC2 0xA0
#define PIC2_OFFSET 0x28
#define PIC2_DATA (PIC2 + 1)

#define PIC_EOI 0x20
#define PIC_MODE_8086 0x01
#define ICW1_ICW4 0x01
#define ICW1_INIT 0x10

#define PIC_WAIT() do {         \
    asm ("jmp 1f\n"             \
    "1:\n"                      \
    "    jmp 2f\n"              \
    "2:");                      \
} while (0)

static void (*handlers[32])(struct registers *regs) = { 0 };

static void stub(struct registers *regs) {
    if (regs->int_no <= 47 && regs->int_no >= 32) {
        if (handlers[regs->int_no - 32]) {
            handlers[regs->int_no - 32](regs);
        }
    }

    // send EOI
    if (regs->int_no >= 0x40) outportb(PIC2, PIC_EOI);
    outportb(PIC1, PIC_EOI);
}

static void PIC_remap() {
    u8 mask1 = inportb(PIC1_DATA), mask2 = inportb(PIC2_DATA);

    outportb(PIC1, ICW1_INIT | ICW1_ICW4); // complicated way of saying 0x11
    PIC_WAIT();
    outportb(PIC2, ICW1_INIT | ICW1_ICW4);
    PIC_WAIT();

    outportb(PIC1_DATA, PIC1_OFFSET);
    PIC_WAIT();
    outportb(PIC2_DATA, PIC2_OFFSET);
    PIC_WAIT();

    outportb(PIC1_DATA, 0x04); // PIC2 at IRQ2
    PIC_WAIT();
    outportb(PIC2_DATA, 0x02); // Cascade indentity
    PIC_WAIT();

    outportb(PIC1_DATA, PIC_MODE_8086);
    PIC_WAIT();
    outportb(PIC1_DATA, PIC_MODE_8086);
    PIC_WAIT();

    outportb(PIC1_DATA, mask1);
    outportb(PIC2_DATA, mask2);
}

// static void PIC_set_mask(u32 i) {
//     u16 port = i < 8 ? PIC1_DATA : PIC2_DATA;
//     u8 value = inportb(port) | (1 << i);
//     outportb(port, value);
// }

static void PIC_clear_mask(u32 i) {
    u16 port = i < 8 ? PIC1_DATA : PIC2_DATA;
    u8 value = inportb(port) & ~(1 << i);
    outportb(port, value);
}

void PIC_install(u32 i, void (*handler)(struct registers *)) {
    CLI();
    handlers[i] = handler;
    PIC_clear_mask(i);
    STI();
}

void PIC_init() {
    PIC_remap();

    for (u32 i = 0; i < 16; i++) {
        isr_install(32 + i, stub);
    }
}