#ifndef BASEDEVICE_SUBTYPES_DISK_DISK_H
#define BASEDEVICE_SUBTYPES_DISK_DISK_H

#include <basedevice/device.h>

enum DiskDeviceCapabilities {
    DISK_DEVICE_CAPABILITY_READ_ONLY    = 0x1,
    DISK_DEVICE_CAPABILITY_REMOVABLE    = 0x2,
    DISK_DEVICE_CAPABILITY_HOTSWAP     = 0x4,
    DISK_DEVICE_CAPABILITY_SUPPORTS_DMA = 0x8,
    DISK_DEVICE_CAPABILITY_LBA         = 0x10,
    DISK_DEVICE_CAPABILITY_48BIT_LBA  = 0x20,
    DISK_DEVICE_CAPABILITY_NCQ        = 0x40,
    DISK_DEVICE_CAPABILITY_TRIM       = 0x80,
    DISK_DEVICE_CAPABILITY_SMART      = 0x100,
    DISK_DEVICE_CAPABILITY_POWER_MANAGEMENT = 0x200,
    DISK_DEVICE_CAPABILITY_WRITE_CACHE = 0x400,
    DISK_DEVICE_CAPABILITY_ATA_SECURITY  = 0x800,
};

// This is for the most part a synthesized
// device (created by RAM disks or IDE enumeration)
// The actual physical block device will first be 
// represented by its connection type before 
// being represented as a disk device.
// =========
// Device links to the base device, which you can
// then read normally to see if its a PCI, USB, etc. device.
typedef struct DiskDevice {
    Device  base;
    u64     sectors;
    u32     sector_size;
} DiskDevice;

// However, each DiskDevice will have a different method
// of connecting to the underlying hardware.
// Like an IDE disk has different parametres than a SATA disk
// which is WAY different than a USBMS disk.

typedef struct IDEDiskDevice {
    DiskDevice  base;
    // Primary (0) or Secondary (1) channel
    u8          channel;
    // Master (0) or Slave (1)
    u8          drive;
    // ATA (0) or ATAPI (1)
    u16         type;
    u16         signature;  // drive signature
    u16         capabilities;// features
    u32         command_sets;// command sets supported
    bool        dma_supported;
    // CHS (0) or LBA (1) or LBA48 (2)
    u8          lba48chs;      // supports 48-bit LBA
    // Also the NIEN
    u8          nIEN;
    // We also need the base I/xO ports for the disk
    u16         port_base;  // I/O Base. (BAR0/BAR2)
    u16         port_ctrl;  // Control Base (BAR1/BAR3)
    u16         port_bmide; // Bus Master IDE (BAR4/BAR4+8)
    // IDENTIFY strings
    char        model[40];
    char        serial[20];
    char        fwrev[8];
} IDEDiskDevice;

#endif