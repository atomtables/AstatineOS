#ifndef PATA_DISK_H
#define PATA_DISK_H

#include <modules/modules.h>
#include <basedevice/subtypes/disk/disk.h>

void ide_initialize(struct AstatineDriver* ide_driver, u32 BAR0, u32 BAR1, u32 BAR2, u32 BAR3, u32 BAR4);
u8 ide_read_sectors(struct IDEDiskDevice* device, unsigned char numsects, unsigned int lba, void* buffer);
u8 ide_write_sectors(struct IDEDiskDevice* device, unsigned char numsects, unsigned int lba, void* buffer);

extern struct AstatineDriverFile pci_ide_controller_driver;
extern struct DiskDriverFile pci_ide_disk_driver;

#endif