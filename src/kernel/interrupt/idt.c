//
// Created by Adithiya Venkatakrishnan on 27/07/2024.
//

#include "idt.h"

#include <modules/modules.h>

struct IDTEntry {
    u16 offset_low;
    u16 selector;
    u8 __ignored;
    u8 type;
    u16 offset_high;
} PACKED;

struct IDTPointer {
    u16 limit;
    u32 base;
} PACKED;

static struct {
    struct IDTEntry entries[256];
    struct IDTPointer pointer;
} idt;

// in entry32.asm
extern void idt_load(u32);

void idt_set(u8 index, void(* base)(struct registers*), u16 selector, u8 flags) {
    idt.entries[index] = (struct IDTEntry) {
        .offset_low = (u32) base & 0xFFFF,
        .offset_high = (u32) base >> 16 & 0xFFFF,
        .selector = selector,
        .type = flags | 0x60,
        .__ignored = 0
    };
}

void idt_init() {
    idt.pointer.limit = sizeof(idt.entries) - 1;
    idt.pointer.base = (u32) &idt.entries[0];
    memset(&idt.entries[0], 0, sizeof(idt.entries));
    idt_load((u32) &idt.pointer);
}