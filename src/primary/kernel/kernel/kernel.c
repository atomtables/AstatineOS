#include <display/advanced/graphics.h>
#include <exception/exception.h>
#include <fpu/fpu.h>
#include <interrupt/interrupt.h>
#include <ps2/keyboard.h>
#include <memory/memory.h>
#include <memory/malloc.h>
#include <modules/strings.h>
#include <pcspeaker/pcspeaker.h>
#include <ps2/controller.h>
#include <timer/PIT.h>
#include <disk/disk.h>
#include <gdt/gdt.h>
#include <systemcalls/syscall.h>
#include <display/simple/display.h>
#include <fat32/fat32.h>
#include <systemcalls/calls/calls.h>
#include <memory/paging.h>

/* In our kernel, we can reserve memory
 * 0x100000-0x1FFFFF for the storage of heap data (like variables)
 * Store an array for 32-byte increments
*/

// YO THIS GUY ONLINE WAS ACT LEGIT :skull:

extern void ahsh();
extern void entryuser32();

static Fat fat;

bool write_stub(const u8* buf, unsigned int sect) {
    u8 err = ide_write_sectors(0, 1, sect, (void*)buf);
    return err == 0;
}
bool read_stub(u8* buf, unsigned int sect) {
    u8 err = ide_read_sectors(0, 1, sect, (void*)buf);
    return err == 0;
}
u8 mount() {
    DiskOps ops = {
        .read = read_stub,
        .write = write_stub,
    };
    int err = fat_probe(&ops, 1);
    if (err != 0) {
        printf("Failed to probe the partition for a FAT32 drive, error code %d.\n", err);
        return 1;
    }
    if ((err = fat_mount(&ops, 1, &fat, "primary")) != 0) {
        printf("Failed to mount the FAT32 filesystem, error code %d.\n", err);
        return 1;
    }
    printf("Mounted with prefix '/primary'.\n");
    return 0;
}


// only blocking thread.
int main() {
    clear_screen();
    println_color("AstatineOS v0.3.0-alpha", COLOR_LIGHT_RED);

    gdt_init();
    printf("Target complete: gdt\n");

    idt_init();
    isr_init();
    PIC_init();
    printf("Target complete: interrupts\n");

    timer_init();
    printf("Target complete: timer\n");

    fpu_init();
    printf("Target complete: fpu\n");

    pcs_init();
    printf("Target complete: pcspeaker\n");

    ps2_controller_init();
    printf("Target complete: ps2 controller\n");

    keyboard_init();
    printf("Target complete: keyboard\n");

    // clear the 0x100000-0x1FFFFF region to prevent dynamic memory corruption
    printf("Setting up dymem region...");
    for (u32* ptr = (u32*)0x100000; ptr < (u32*)0x400000; ptr++) {
        *ptr = 0;
        if ((u32)ptr % 0x10000 == 0) {
            printf(".");
            sleep(1); // make it seem like it does something
        }
    }
    printf("Done\n");

    // now initialise disk
    ide_initialize(0x1F0, 0x3F6, 0x170, 0x376, 0x000);
    printf("Target complete: disk\n");
    printf("Mounting primary partition...\n");
    if (mount() != 0) {
        printf("Failed to mount primary partition, critical error.");
        return -1;
    }
    

    syscall_install();

    // We are starting a new "process" now
    // Open the 3 file descriptors for stdin, stdout, stderr
    open_fds[0].id = 0;
    open_fds[0].type = 0; // device
    open_fds[0].identifier = "/Devices/stdin";
    open_fds[1].id = 1;
    open_fds[1].type = 0; // device
    open_fds[1].identifier = "/Devices/stdout";
    open_fds[2].id = 2;
    open_fds[2].type = 0; // device
    open_fds[2].identifier = "/Devices/stderr";

    init_mem();
    printf("Target complete: memory\n");

    printf("just testing virtual memory\n");
    alloc_page(0xc0000000); // allocate a page at 1GB
    u32* test = (u32*)0xc0000000;
    printf("current val: %p", *test);
    *test = 0x12345678;
    printf(" new val: %p", *test);
    free_page(0xc0000000);
    sleep(1);
    printf("\n\nfreed page at 0xc0000000: %p\n", *((u32*)0xc0000000));

    while(1);
    entryuser32();

    reboot();
}