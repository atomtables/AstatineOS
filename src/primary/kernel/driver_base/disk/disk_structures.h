#ifndef ASTATINE_TELETYPE_H
#define ASTATINE_TELETYPE_H

#if __has_include ("types.h")
#include "types.h"
#endif
#if __has_include (<modules/modules.h>)
#include <modules/modules.h>
#endif

#if __has_include (<basedevice/device.h>)
#include <basedevice/device.h>
#endif
#if __has_include ("device.h")
#include "device.h"
#endif

#if __has_include (<driver_base/driver_base.h>)
#include <driver_base/driver_base.h>
#endif
#if __has_include ("drivers.h")
#include "drivers.h"
#endif

typedef struct DiskDriverFunctions DiskDriverFunctions;
typedef struct DiskDriverFile DiskDriverFile;
// This is the active initialised driver
typedef struct DiskDriver DiskDriver;

struct DiskDriverFunctions {
    // mounting
    u32 (*mount)(DiskDriver* self);
    u32 (*unmount)(DiskDriver* self);

    // Simple read/write functions
    u32 (*read)(DiskDriver* self, u8* buf, u32 sect);
    u32 (*write)(DiskDriver* self, const u8* buf, u32 sect);

    // Multi-sector read/write
    u32 (*readmany)(DiskDriver* self, u8* buf, u32 sect, u32 count);
    u32 (*writemany)(DiskDriver* self, const u8* buf, u32 sect, u32 count);

    u32 (*stat)(DiskDriver* self, struct stat* statbuf);
};
struct DiskDriverFile {
    AstatineDriverFile base;
    DiskDriverFunctions functions;
};
// In the case for the DiskDriver, the init function should
// initialise the driver for the device. In the case of a 
// controller with multiple disks, it should discover all disks
// and register them as individual devices.
// ======
// So actually, the controller initialises each individual
// disk as a IDEDiskDevice or whatever. The Disk is an abstraction
// for filesystem drivers, meaning the idea of a Disk is just copying
// the DiskDriver, meaning we don't need it.
struct DiskDriver {
    AstatineDriver base;
    struct Disk* disk;
    DiskDriverFunctions functions;
};

#endif