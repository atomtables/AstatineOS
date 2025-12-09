#ifndef DISK_H
#define DISK_H

#include <modules/modules.h>
#include <driver_base/disk/disk_structures.h>

// DISK
// ==========
// This represents any device (physical or virtual) that
// provides random-access block storage capabilities.
// Examples include physical hard drives, SSDs, RAM disks, etc.
// Disks are made up of sectors, which are the smallest
// individually addressable units of storage on the disk.
// Disks do not extend a device, but rather attach to a device.
// ==========
// struct Disks are created when a disk driver MOUNTS the disk.
// The driver initialises itself, attaching to the device or discovering
// a new one. When the device is successfully initialised, the operating system
// can request the disk to be mounted. This creates a struct Disk, which is stored
// and set under a mountpoint.
// ==========
// DiskDriver has a field called struct Disk which points to the Disk for which
// the drive is currently managing. This Disk is exclusively only for filesystems
// to have an abstracted way of reading/writing sectors without needing to know
// about the underlying hardware or driver mechanisms.
// ==========
// Which means this has no use until a filesystem wants to come onto the drive,
// in which case the filesystem abstraction will load the partition map, make sure
// the filesystem is compatible, then provide read/write commands to the filesystem driver
// relative to the partition.
typedef struct Disk {
    // Partitions are logical devices
    // and this is a linked list.
    struct Partition* partitions;
    int partition_count;
    struct DiskDriver* driver;

    // sector count
    u64 sectors;
    // sector size
    u32 sectsize;

    u8 reserved[32];
} Disk;

// We don't have a garbage collector so we don't care about infinite references
// Just remember if we're forgetting about a partition to free its memory, then
// replace the pointer with null in Disk.
typedef struct Partition {
    // partition number
    // in the case the entire disk is a partition,
    // this will still be zero.
    int number;
    struct Disk* owner;
    struct Filesystem* fs;

    struct Partition* before;
    struct Partition* next;
    // If partition type = 0, then it means
    // this is uninitialised space
    u8 type;
    u64 start_sector;
    u64 sector_count;
} Partition;

#endif