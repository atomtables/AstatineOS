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
#include <disk/pata/disk.h>
#include <gdt/gdt.h>
#include <systemcalls/syscall.h>
#include <display/simple/display.h>
#include <fat32/fat32.h>
#include <systemcalls/calls/calls.h>
#include <memory/paging.h>
#include <elfloader/elfloader.h>
#include <terminal/terminal.h>
#include <driver_base/driver_base.h>
#include <basedevice/devicelogic.h>
#include <basedevice/discovery/discovery.h>
#include <driver_base/disk/disk.h>

/* In our kernel, we can reserve memory
 * 0x100000-0x1FFFFF for the storage of heap data (like variables)
 * Store an array for 32-byte increments
*/

// YO THIS GUY ONLINE WAS ACT LEGIT :skull:

extern void ahsh();

static Fat fat;
bool write_stub(const u8* buf, unsigned int sect) {
    u8 err = active_disk_driver->functions.write(active_disk_driver, buf, sect);
    return err == 0;
}
bool read_stub(u8* buf, unsigned int sect) {
    if (!active_disk_driver) panic("No active disk driver!");
    u8 err = active_disk_driver->functions.read(active_disk_driver, buf, sect);
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

    for (int i = 0; i < 80; i++) {
        for (int j = 0; j < 25; j++) {
            *((u16*)0xb8000 + i + j * 80) = (u16)(' ' | (COLOR_WHITE<< 12));
        }
    }

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

    // adds items to the 0x100000-0x3FFFFFF region
    // to store paging files and memory management stuff.
    init_mem();
    STI();
    printf("Target complete: memory\n");


    discover_isa_devices();
    discover_pci_devices();
    install_driver(&pci_ide_controller_driver, null);
    install_driver((AstatineDriverFile*)&pci_ide_disk_driver, null);

    char buf[64] = "Loaded driver: ";
    xtoa_padded((u32)active_disk_driver, buf + strlen(buf)-1);

    printf("Target complete: disk\n");
    printf("Mounting primary partition...\n");
    if (mount() != 0) {
        printf("Failed to mount primary partition, critical error.");
        return -1;
    }

    syscall_install();

    // We are starting a new "process" now
    // Open the 3 file descriptors for stdin, stdout, stderr
    terminal_install();

    clear_screen();
    printf("Ready to load driver, enter path: ");
    char elf_path[127] = "/primary/drivers/textmode.adv";

    if (is_elf(elf_path) == 0) {
        printf("ELF file detected, loading...\n");
        File file;
        if (fat_file_open(&file, elf_path, FAT_READ) != 0) {
            printf("Failed to open ELF file: %s\n", elf_path);
            char* x = "Failed to load ELF file.";
            for (u32 i = 0; x[i] != 0x00; i++) {
                *((u8*)0xb8000 + i * 2) = x[i];
            }
            goto skiploading;
        }
        int errno;
        if ((errno = attempt_install_driver(&file, elf_path)) != 0) {
            printf("Failed to load and run ELF file.\n");
            char x[45] = "Failed to load and run ELF file.  ";
            itoa(errno, x + strlen(x) - 1);
            for (u32 i = 0; x[i] != 0x00; i++) {
                *((u8*)0xb8000 + i * 2) = x[i];
            }
            goto skiploading;
        }
    } else {
        printf("The specified file is not a valid ELF file.\n");
        char* x = "not an elf.";
        for (u32 i = 0; x[i] != 0x00; i++) {
            *((u8*)0xb8000 + i * 2) = x[i];
        }
        goto skiploading;
    }
    skiploading:

    while(1) {
        clear_screen();
        printf("Load a file: ");
        char elf_path[127] = "/primary/basicbasic.aex";
        input(elf_path, 127);
        if (is_elf(elf_path) == 0) {
            printf("ELF file detected, loading...\n");
            if (elf_load_and_run(elf_path) != 0) {
                printf("Failed to load and run ELF file.\n");
            }
        } else {
            printf("The specified file is not a valid ELF file.\n");
        }
    }

    reboot();
}