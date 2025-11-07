#include <stdint.h>
#include "fat32.h"
extern void ata_lba_read(uint32_t lba, uint8_t sectors, void* buffer);

// thank you so much to https://github.com/strawberryhacker/fat32
// linker should link for 0x8000

struct LBA {
    uint8_t size;
    uint8_t reserved;
    uint16_t max_sectors;
    uint16_t offset;
    uint16_t segment;
    uint32_t low4;
    uint32_t high;
};

bool fake() {return 0;}
bool read(uint8_t* buf, uint32_t sect) {
    ata_lba_read((uint32_t)sect, 1, buf);
    return true;
}

void print(char* buf) {
    for (int i = 0; buf[i] != 0; i++) {
        volatile char* mem = (char*)(0xb8000+1024+(i*2));
        *(mem) = buf[i];
    }
}

void printn(char* buf, int x) {
    for (int i = 0; i < x; i++) {
        volatile char* mem = (char*)(0xb8000+1024+(i*2));
        *(mem) = buf[i];
    }
}

char* xtoa_padded(uint32_t number, char* str);
void main() {
    DiskOps ops = {
        .read = read,
        .write = fake,
    };
    Fat fat = {0};
    File file;
    int err = fat_probe(&ops, 1);
    if (err != 0) {
        print("failed probe");
        return;
    }
    if ((err = fat_mount(&ops, 1, &fat, "mnt")) != 0) {
        print("failed mount");
        return;
    }
    err = fat_file_open(&file, "/mnt/kernel.bin", FAT_READ);
    if (err) {
        char buf[64];
        xtoa_padded(err, buf);
        print(buf);
        return;
    }
    int cnt = 0;
    int times = 0;
    uint8_t buf[512];
    while (1) {
        if (err) {
            print("failed read");
            return;
        }
        err = fat_file_read(&file, buf, 512, &cnt);
        memcpy((void*)0x10000 + times++ * 512, buf, cnt);
        if (cnt != 512)
            break;
    }
    err = fat_file_close(&file);
    fat_umount(&fat);

    ((void(*)(void))0x10000)();
}