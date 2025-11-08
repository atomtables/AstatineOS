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

typedef struct Command {
    char* name;
    void(* function)(int, char**);
} Command;

void echo(int argc, char** argv) {
    for (int i = 0; i < argc; i++) {
        display.printf("%s ", argv[i]);
    }
    display.printf("\n");
}

void onesecond() {
    sleep(1000);
    display.printf("1 second has passed\n");
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
        display.printf("Failed to open file '%s', error code %d\n", "/primary/musictrack", err);
        return;
    }
    int cnt = 0;
    int totalcnt = 0;
    int times = 0;
    uint8_t buf[512];
    while (1) {
        if (err) {
            display.printf("Failed to read file '%s', error code %d\n", "/primary/musictrack", err);
            return;
        }
        err = fat_file_read(&file, buf, 512, &cnt);
        memcpy((void*)0x200000 + times++ * 512, buf, cnt);
        totalcnt += cnt;
        if (cnt != 512)
            break;
    } 
    display.printf("\n");
    err = fat_file_close(&file);
    if (err) {
        display.printf("Failed to close file '%s', error code %d\n", "/primary/musictrack", err);
    }
    u8* musictrack = (u8*)0x200000;
    display.printf("loaded song data, playing...\n");
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
    display.printf("Mounting current filesystem...\n");
    DiskOps ops = {
        .read = read,
        .write = write,
    };
    int err = fat_probe(&ops, 1);
    if (err != 0) {
        display.printf("Failed to probe the partition for a FAT32 drive, error code %d.\n", err);
        return;
    }
    if ((err = fat_mount(&ops, 1, &fat, "primary")) != 0) {
        display.printf("Failed to mount the FAT32 filesystem, error code %d.\n", err);
        return;
    }
    display.printf("Mounted with prefix '/primary'.\n");
}

void cat(int argc, char** argv) {
    if (argc < 1) {
        display.printf("Usage: cat <filename>\n");
        return;
    }
    char* filename = argv[0];
    
    File file;
    int err = fat_file_open(&file, filename, FAT_READ);
    if (err) {
        display.printf("Failed to open file '%s', error code %d\n", filename, err);
        return;
    }
    int cnt = 0;
    uint8_t buf[81];
    while (1) {
        if (err) {
            display.printf("Failed to read file '%s', error code %d\n", filename, err);
            return;
        }
        err = fat_file_read(&file, buf, 80, &cnt);
        buf[cnt] = '\0';
        display.printf("%s\n", buf);
        if (cnt != 80)
            break;
    } 
    display.printf("\n");
    err = fat_file_close(&file);
    if (err) {
        display.printf("Failed to close file '%s', error code %d\n", filename, err);
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
    {"playmusic", playmusiccmd}
};

void ahsh() {
    display.printf("\n");

    // code doesn't have to be good, just functional

    while (1) {
        char* prompt = malloc(64);
        display.printf("-ahsh %p> ", prompt);
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
        display.printf("%p %s Command not found...\n", prompt_s.ret, prompt_s.ret[0]);

        complete:
            free(prompt_s.ret, prompt_s.size);
        free(prompt, 64);
    }
}
