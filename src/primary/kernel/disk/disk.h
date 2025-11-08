#ifndef DISK_H
#define DISK_H

#include <modules/modules.h>

extern void ide_initialize(u32 BAR0, u32 BAR1, u32 BAR2, u32 BAR3, u32 BAR4);
extern u8 ide_read_sectors(unsigned char drive, unsigned char numsects, unsigned int lba, void* buffer);
extern u8 ide_write_sectors(unsigned char drive, unsigned char numsects, unsigned int lba, void* buffer);

#endif