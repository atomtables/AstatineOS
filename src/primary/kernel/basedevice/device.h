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
    DEVICE_TYPE_SERIAL      = 3,
    // a disk device (HDD/SSD/RAM Disk)
    DEVICE_TYPE_DISK        = 4,
    // a controller device (like an IDE controller)
    DEVICE_TYPE_CONTROLLER  = 5
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

    struct Device* parent;
    struct Device* child;
    struct Device* left_sibling;
    struct Device* right_sibling;
} Device;

// Legacy devices without a specific bus type
// like OMFG ISA IS JUST AN IO PORT TS CANNOT BE REAL
typedef struct PlatformDevice {
    Device  base;
    int     platform_id;
} PlatformDevice;

// Copilot made ts and gave me a public code warning lmao
typedef struct PCIDevice {
    // Base Device properties
    Device  base;
    u8      bus;                     // bus number
    u8      device;                  // device number
    u8      function;                // function number
    u8      devfn;                   // packed device/function like Linux devfn

    // Identification
    u16     vendor_id;               // vendor ID
    u16     device_id;               // device ID
    u16     subsystem_vendor_id;     // subsystem vendor ID
    u16     subsystem_id;            // subsystem device ID

    // Command / status
    u16     command;                 // command register
    u16     status;                  // status register

    // Class information
    u8      revision_id;             // revision ID
    u8      prog_if;                 // programming interface
    u8      subclass;                // subclass code
    u8      class_code;              // base class code

    // Header basics
    u8      cache_line_size;         // cache line size
    u8      latency_timer;           // latency timer
    u8      header_type;             // header type (bit 7 = multi-function)
    u8      bist;                    // built-in self-test

    // BARs (keep individual for compatibility)
    u32     bars[6];
    u32     bar_size[6];             // decoded BAR sizes
    u32     bar_flags[6];            // decoded BAR flags (I/O vs mem, prefetch, 64-bit)

    // Expansion ROM
    u32     expansion_rom_base_address;
    u32     expansion_rom_size;      // decoded ROM size

    // CardBus / capabilities
    u32     cardbus_cis_pointer;     // CardBus CIS pointer (legacy)
    u8      capabilities_pointer;    // head of capabilities list
    u8      reserved1[3];
    u32     reserved2;

    // Interrupt info
    u8      interrupt_line;          // legacy PIC/IOAPIC line
    u8      interrupt_pin;           // INTA#/INTB#/INTC#/INTD#
    u8      min_grant;               // minimum grant (legacy)
    u8      max_latency;             // maximum latency (legacy)

    // Bridge-specific (valid for header type 1/2)
    u8      primary_bus_number;      // primary bus number
    u8      secondary_bus_number;    // secondary bus number
    u8      subordinate_bus_number;  // highest bus number downstream
    u8      secondary_latency_timer; // secondary latency timer
    u8      io_base;                 // I/O base (lower bits)
    u8      io_limit;                // I/O limit (lower bits)
    u16     secondary_status;        // secondary status
    u16     memory_base;             // memory base (prefetchable off)
    u16     memory_limit;            // memory limit (prefetchable off)
    u16     prefetchable_memory_base;// prefetchable memory base
    u16     prefetchable_memory_limit;// prefetchable memory limit
    u32     prefetchable_base_upper32; // upper 32 bits of prefetch base
    u32     prefetchable_limit_upper32;// upper 32 bits of prefetch limit
    u16     io_base_upper16;         // upper 16 bits I/O base
    u16     io_limit_upper16;        // upper 16 bits I/O limit
    u16     bridge_control;          // bridge control register
} PCIDevice;

#endif