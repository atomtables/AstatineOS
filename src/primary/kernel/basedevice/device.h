#ifndef BASEDEVICE_DEVICE_H
#define BASEDEVICE_DEVICE_H

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

#if __has_include (<driver_base/driver_base_structures.h>)
#include <driver_base/driver_base_structures.h>
#endif
#if __has_include ("drivers.h")
#include "drivers.h"
#endif

enum IsaDevices {
    ISA_DEVICE_VGA_TEXT = 0,
    ISA_DEVICE_OTHER = 1
};

// This is also used by driver_base to
// identify what device a driver is for
enum DeviceType {
    // unknown device
    DEVICE_TYPE_UNKNOWN     = 0,
    // a framebuffer device/display
    // to write to (a screen)
    DEVICE_TYPE_DISPLAY     = 1,
    // a glass teletype (console)
    // text-mode device
    DEVICE_TYPE_TTYPE       = 2,
    // just a general serial stream out
    DEVICE_TYPE_SERIAL      = 3
};

enum ConnectionType {
    // There would realistically never be a scenario where
    // we don't know how a device is connected but apparently
    // legacy code is a W thing.
    CONNECTION_TYPE_UNKNOWN,
    CONNECTION_TYPE_IO,
    CONNECTION_TYPE_PCI,
    CONNECTION_TYPE_USB,
    // Why the hell would we have an other connection type
    // if we had a unknown one ts is actually fried.
    CONNECTION_TYPE_OTHER
};

// This doesn't actually provide any WAY of connecting
// with a device, but rather represents a device using identifiers
// that only it would have anyway. This means that items like
// a PCI IDE controller, or a PIC, can be represented by this struct
// and connected appropriately.
typedef struct Device {
    // These are kernel-level identifiers
    // these don't mean anything for identifying
    // the actual device, but rather to keep track of
    // a device throughout functions.
    char*   name;
    int     type;
    int     conn;
    u32     id;
    u32     size;

    bool    owned;
    struct AstatineDriver* attached_driver;
} Device;

// Legacy devices without a specific bus type
// like OMFG ISA IS JUST AN IO PORT TS CANNOT BE REAL
typedef struct PlatformDevice {
    Device  base;
    int     platform_id;
} PlatformDevice;

// Copilot made ts and gave me a public code warning lmao
typedef struct PCIDevice {
    Device  base;
    u8      bus;
    u8      device;
    u8      function;
    u16     vendor_id;
    u16     device_id;
    u16     command;
    u16     status;
    u8      revision_id;
    u8      prog_if;
    u8      subclass;
    u8      class_code;
    u8      cache_line_size;
    u8      latency_timer;
    u8      header_type;
    u8      bist;
    u32     bar0;
    u32     bar1;
    u32     bar2;
    u32     bar3;
    u32     bar4;
    u32     bar5;
    u32     cardbus_cis_pointer;
    u16     subsystem_vendor_id;
    u16     subsystem_id;
    u32     expansion_rom_base_address;
    u8      capabilities_pointer;
    u8      reserved1[3];
    u32     reserved2;
    u8      interrupt_line;
    u8      interrupt_pin;
    u8      min_grant;
    u8      max_latency;
} PCIDevice;

#endif