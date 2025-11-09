//
// Created by Adithiya Venkatakrishnan on 2/1/2025.
//

#include <display/simple/display.h>
#include <pcspeaker/pcspeaker.h>
#include <exception/exception.h>
#include <memory/memory.h>
#include <modules/strings.h>
#include <programs/basicbasic/basicbasic.h>
#include <programs/fungame/fungame.h>
#include <programs/netnotes/netnotes.h>
#include <ps2/keyboard.h>
#include <timer/PIT.h>
#include <fat32/fat32.h>
#include <disk/disk.h>
#include <stdarg.h>

typedef struct Command {
    char* name;
    void(* function)(int, char**);
} Command;


void syscall(u32 num, u32 arg1) {
    __asm__ volatile (
        "movl %0, %%eax\n"
        "movl %1, %%ebx\n"
        "int $0x30\n"
        : 
        : "r"(num), "r"(arg1)
        : "eax", "ebx"
    );
}

u8 buf[11] = {0};

void printf_user(const char* fmt, ...) {
    va_list args;
    va_start(args, 0);

    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%') {
            i++;
            switch (fmt[i]) {
            case 'd': {
                char* digits = itoa_signed(va_arg(args, i32), &buf[0]);
                syscall(1, (u32)digits);
                break;
            }
            case 'x': {
                char* digits = xtoa(va_arg(args, u32), &buf[0]);
                syscall(1, (u32)digits);
                break;
            }
            case 'p': {
                syscall(1, (u32)"0x");
                char* digits = xtoa_padded(va_arg(args, u32), &buf[0]);
                syscall(1, (u32)digits);
                break;
            }
            case 's': {
                syscall(1, (u32)va_arg(args, char*));
                break;
            }
            case 'c': {
                char arg = va_arg(args, i32);
                syscall(0, arg);
                break;
            }
            case 'u': {
                char* digits = itoa(va_arg(args, u32), &buf[0]);
                syscall(1, (u32)digits);
                break;
            }
            default: {
                syscall(0, '%');
                syscall(0, fmt[i]);
                break;
            }
            }
        }
        else if (fmt[i] == '\n') { syscall(0, '\n'); }
        else if (fmt[i] == 0x08) { syscall(0, 0x08); }
        else { syscall(0, fmt[i]); }
    }

    va_end(args);
}


void echo(int argc, char** argv) {
    for (int i = 0; i < argc; i++) {
        printf_user("%s ", argv[i]);
    }
    printf_user("\n");
}

void onesecond() {
    sleep(1000);
    printf_user("1 second has passed\n");
}

void sti() {
    __asm__ volatile ("sti");
}

void clear(int argc, char** argv) {
    display.clear_screen();
}

void div0() {
    DIV_BY_ZERO();
}

// don't actually know if this works + can't test but ig lmao sob
void playmusiccmd() {
    File file;
    int err = fat_file_open(&file, "/primary/musictrack", FAT_READ);
    if (err) {
        printf_user("Failed to open file '%s', error code %d\n", "/primary/musictrack", err);
        return;
    }
    int cnt = 0;
    int totalcnt = 0;
    int times = 0;
    uint8_t buf[512];
    while (1) {
        if (err) {
            printf_user("Failed to read file '%s', error code %d\n", "/primary/musictrack", err);
            return;
        }
        err = fat_file_read(&file, buf, 512, &cnt);
        memcpy((void*)0x200000 + times++ * 512, buf, cnt);
        totalcnt += cnt;
        if (cnt != 512)
            break;
    } 
    printf_user("\n");
    err = fat_file_close(&file);
    if (err) {
        printf_user("Failed to close file '%s', error code %d\n", "/primary/musictrack", err);
    }
    u8* musictrack = (u8*)0x200000;
    printf_user("loaded song data, playing...\n");
    pcs_play_8bit(musictrack, totalcnt);
}

extern void ata_lba_read(uint32_t lba, uint8_t sectors, void* buffer);
extern void ata_lba_write(uint32_t lba, uint8_t sectors, void* buffer);
bool write(const u8* buf, long unsigned int sect) {
    u8 err = ide_write_sectors(0, 1, sect, buf);
    return err == 0;
}
bool read(u8* buf, long unsigned int sect) {
    u8 err = ide_read_sectors(0, 1, sect, buf);
    return err == 0;
}

static Fat fat;

void mount() {
    printf_user("Mounting current filesystem...\n");
    DiskOps ops = {
        .read = read,
        .write = write,
    };
    int err = fat_probe(&ops, 1);
    if (err != 0) {
        printf_user("Failed to probe the partition for a FAT32 drive, error code %d.\n", err);
        return;
    }
    if ((err = fat_mount(&ops, 1, &fat, "primary")) != 0) {
        printf_user("Failed to mount the FAT32 filesystem, error code %d.\n", err);
        return;
    }
    printf_user("Mounted with prefix '/primary'.\n");
}

void cat(int argc, char** argv) {
    if (argc < 1) {
        printf_user("Usage: cat <filename>\n");
        return;
    }
    char* filename = argv[0];
    
    File file;
    int err = fat_file_open(&file, filename, FAT_READ);
    if (err) {
        printf_user("Failed to open file '%s', error code %d\n", filename, err);
        return;
    }
    int cnt = 0;
    uint8_t buf[81];
    while (1) {
        if (err) {
            printf_user("Failed to read file '%s', error code %d\n", filename, err);
            return;
        }
        err = fat_file_read(&file, buf, 80, &cnt);
        buf[cnt] = '\0';
        printf_user("%s\n", buf);
        if (cnt != 80)
            break;
    } 
    printf_user("\n");
    err = fat_file_close(&file);
    if (err) {
        printf_user("Failed to close file '%s', error code %d\n", filename, err);
    }
}

static Command commands[] = {
    {"echo", echo},
    {"beep", beep},
    {"sleep", onesecond},
    {"fungame", fungame},
    {"netnotes", netnotes},
    {"basicbasic", basicbasic},
    {"clear", clear},
    {"reboot", reboot},
    {"crash", div0},
    {"mount", mount},
    {"cat", cat},
    {"playmusic", playmusiccmd},
    {"sti", sti}
};

void ahsh() {
    // for now, avoid syscall and crap because 
    // ts is not worth it atp
    __asm__ volatile ("int $48");
    printf_user("\n");

    // code doesn't have to be good, just functional
    // this code is neither btw

    while (1) {
        char* prompt = malloc(64);
        printf_user("-ahsh %p> ", prompt);
        prompt = input(prompt, 64);
        StrtokA prompt_s = strtok_a(prompt, " ");
        for (u32 i = 0; i < sizeof(commands) / sizeof(Command); i++) {
            if (strcmp(commands[i].name, prompt_s.ret[0]) == 0) {
                if (commands[i].function) {
                    commands[i].function(prompt_s.count - 1, &prompt_s.ret[1]);
                }
                goto complete;
            }
        }
        printf_user("%p %s Command not found...\n", prompt_s.ret, prompt_s.ret[0]);

        complete:
            free(prompt_s.ret, prompt_s.size);
        free(prompt, 64);
    }
}
