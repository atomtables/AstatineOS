#include <modules/modules.h>

// using u32 because OSDEV said it was a good thing to do
typedef struct {
    // length being size of the segment
    // below is bits 0-15
	u32 length_low             : 16;
    // base being start of the segment
    // below is bits 0-23
	u32 base_low               : 24;
    // next 8 bits make the Access Byte (configuration)
    // if the CPU accesses this segment, itll be set to 1
    // if the GDT is in a read-only page it will cause the CPU to crash
    // leave set to 4
	u32 accessed               :  1;
    // If 1, allow reading for code (if code seg) or allow writing for data (if data seg)
    // if the code and data segments are separate it could also mean that
    // the code cannot be written to. since we want to use x64 in the future which
    // doesn't support segmentation it's fine.
	u32 read_write             :  1; 
    // For data segments: direction: if 1 the segment will grow down from
    // the top of segment (so the base is actually the end of the seg)
    // For code segments: Conforming bit: if 1 then code in the segment can
    // only be run by lower or equal privilege levels. 0 only allows equal.
	u32 conforming_expand_down :  1;
    // if 1 then code, if 0 then data.
	u32 code                   :  1;
    // if 1 then defines a code/data segment, if 0 then defines TSS or other system segment
	u32 code_data_segment      :  1;
    // most important: Privilege Level
    // can choose between ring 0 (kernel) to ring 3 (user)
    // only do ring 0 and 3 the others are useless.
	u32 DPL                    :  2; // privilege level
	// does this seg exist? if so then 1
    u32 present                :  1;
    // other bits 16-19 of length
	u32 length_high            :  4;
    // reserved (we don't care about this)
	u32 available              :  1; 
    // is this 64-bit? answer is no
	u32 long_mode              :  1;
    // set to 1 to define segment as a 32-bit PM segment
	u32 is_32_bit              :  1; // 32-bit opcodes for code, uint32_t stack for data
	// granularity: if 1 then use 4k page addressing, 0 for byte addressing
    u32 granularity            :  1; 
    // other bits 24-31 of base
	u32 base_high              :  8;
} PACKED gdt_entry;


typedef struct tss_entry_struct {
	u32 prev_tss; // The previous TSS - with hardware task switching these form a kind of backward linked list.
	u32 esp0;     // The stack pointer to load when changing to kernel mode.
	u32 ss0;      // The stack segment to load when changing to kernel mode.
	// Everything below here is unused.
	u32 esp1; // esp and ss 1 and 2 would be used when switching to rings 1 or 2.
	u32 ss1;
	u32 esp2;
	u32 ss2;
	u32 cr3;
	u32 eip;
	u32 eflags;
	u32 eax;
	u32 ecx;
	u32 edx;
	u32 ebx;
	u32 esp;
	u32 ebp;
	u32 esi;
	u32 edi;
	u32 es;
	u32 cs;
	u32 ss;
	u32 ds;
	u32 fs;
	u32 gs;
	u32 ldt;
	u16 trap;
	u16 iomap_base;
} PACKED tss_entry_t;

tss_entry_t tss_entry;

void write_tss(gdt_entry *g) {
	// Compute the base and limit of the TSS for use in the GDT entry.
	u32 base = (u32) &tss_entry;
	u32 limit = sizeof tss_entry - 1;

	// Add a TSS descriptor to the GDT.
	g->length_low = limit;
	g->base_low = base;
	g->accessed = 1; // With a system entry (`code_data_segment` = 0), 1 indicates TSS and 0 indicates LDT
	g->read_write = 0; // For a TSS, indicates busy (1) or not busy (0).
	g->conforming_expand_down = 0; // always 0 for TSS
	g->code = 1; // For a TSS, 1 indicates 32-bit (1) or 16-bit (0).
	g->code_data_segment=0; // indicates TSS/LDT (see also `accessed`)
    // change to ring 0 pls
	g->DPL = 0; // ring 0, see the comments below
	g->present = 1;
	g->length_high = (limit & (0xf << 16)) >> 16; // isolate top nibble
	g->available = 0; // 0 for a TSS
	g->long_mode = 0;
	g->is_32_bit = 0; // should leave zero according to manuals.
	g->granularity = 0; // limit is in bytes, not pages
	g->base_high = (base & (0xff << 24)) >> 24; //isolate top byte

	// Ensure the TSS is initially zero'd.
	memset(&tss_entry, 0, sizeof tss_entry);

	tss_entry.ss0  = 0x10;  // Set the kernel stack segment.
	tss_entry.esp0 = 0x10000; // Set the kernel stack pointer.
	//note that CS is loaded from the IDT entry and should be the regular kernel code segment
}

void set_kernel_stack(u32 stack) { // Used when an interrupt occurs
	tss_entry.esp0 = stack;
}

void flush_tss(void);
void load_gdt(void* descriptor);

gdt_entry gdt_entries[6];

// we already have a GDT but it's the bare minimum and we can't 
// check on it. it's unknown to the kernel which is bad.
// it's also likely to get overwritten.
void gdt_init() {
    gdt_entry* gdt = gdt_entries;
    // reset the memory
    memset(gdt, 0, sizeof(gdt_entry) * 6);

    // Kernel Code
    gdt_entry* gdt_kernel_code = &gdt[1];
    gdt_kernel_code->length_low = 0xFFFF;
    gdt_kernel_code->length_high = 0xF;
    gdt_kernel_code->base_low = 0x0;
    gdt_kernel_code->base_high = 0x0;
    // config byte
    gdt_kernel_code->present = 1;
    gdt_kernel_code->DPL = 0;
    gdt_kernel_code->code_data_segment = 1;
    gdt_kernel_code->code = 1;
    gdt_kernel_code->conforming_expand_down = 0;
    gdt_kernel_code->read_write = 1;
    gdt_kernel_code->accessed = 1;
    // flags
    gdt_kernel_code->granularity = 1; 
    gdt_kernel_code->is_32_bit = 1;
    gdt_kernel_code->long_mode = 0;

    // Kernel Data (same as Code but with the code bit disabled)
    gdt_entry* gdt_kernel_data = &gdt[2];
    memcpy(gdt_kernel_data, gdt_kernel_code, sizeof(gdt_entry));
    gdt_kernel_data->code = 0;

    gdt_entry* gdt_user_code = &gdt[3];
    memcpy(gdt_user_code, gdt_kernel_code, sizeof(gdt_entry));
    gdt_user_code->DPL = 3; // ring 3

    gdt_entry* gdt_user_data = &gdt[4];
    memcpy(gdt_user_data, gdt_kernel_data, sizeof(gdt_entry));
    gdt_user_data->DPL = 3; // ring 3

    // now for the task segment
    write_tss(&gdt[5]); // TSS segment will be the fifth 

    static struct {
        u16 size;
        void* address;
    } PACKED gdt_descriptor;
    gdt_descriptor.size = sizeof(gdt_entry) * 6 - 1;
    gdt_descriptor.address = gdt;
    load_gdt(&gdt_descriptor);

    flush_tss();
}
