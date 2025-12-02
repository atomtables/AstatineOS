#ifndef DRIVER_BASE_STRUCTURES_H
#define DRIVER_BASE_STRUCTURES_H

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

struct KernelFunctionPointers {
    // allocate bytes in memory
    // no page allocation required
    void* (*kmalloc)(u32 size);
    void* (*kmalloc_aligned)(u32 size, u32 alignment);
    void  (*kfree)(void* ptr);
    void* (*krealloc)(void* ptr, u32 size);

    // install irqs
    void  (*PIC_install)(u32 i, void (*handler)(struct registers *));

    // paging
    void  (*allow_null_page_read)();
    void  (*disallow_null_page)();
};

// This is an active instance of a driver.
typedef struct AstatineDriver {
    u32     driver_type;
    u32     device_type;
    // if not null, then driver is initialised
    struct Device* device;
    struct KernelFunctionPointers* kfp;

    // Some drivers will want to discover their own devices
    // like ISA drivers that don't use plug-and-play.
    // or other legacy hardware like the pcspk.
    bool    (*probe)(struct Device* device, struct KernelFunctionPointers* kfp);

    // Other drivers will rather just get the list of devices
    // and look for ones that they can manage.
    bool    (*check)(struct Device* device, struct KernelFunctionPointers* kfp);

    // Assuming a device that this can manage has been found,
    // it means we can add that device to the global list,
    // and initialise the driver for that device.
    int     (*init) (struct AstatineDriver* self);

    // If the driver is deemed unnecessary, the deinit function
    // will release the device.
    void    (*deinit)(struct AstatineDriver* self);

    // Other functions are determined by the driver handler
    // so teletype-drivers will have the ability to draw characters,
    // display drivers can set pixels, etc.
} AstatineDriver;

// This AstatineDriverFile is the base struct
// for reading a driver from an ELF file.
// This is loaded from the .astatine_driver section
// and installs all related properties such as functions.
// This is NOT the active driver instance, which will be
// created by the driver loader and installed into the
// global driver instance.
// That way, when a new device has been detected, the
// driver can pick it up and create a new instance for it.
// That new instance will be of the type AstatineDriver.
typedef struct AstatineDriverFile {
    u32     size;
    // "ASTATINE"
    char    sig[8]; 
    // redundant
    enum ConnectionType     
            driver_type;
    // is this a PCI, ISA, etc. driver?
    enum DeviceType     
            device_type;
    // driver name
    char    name[32]; 
    // driver version
    char    version[16];
    // driver author
    char    author[32];
    // driver description
    char    description[64]; 
    // reserved for future use
    char    reserved[32]; 

    // reserved for driver verification/signature
    u8      verification[64]; 

    // Some drivers will want to discover their own devices
    // like ISA drivers that don't use plug-and-play.
    // or other legacy hardware like the pcspk.
    bool    (*probe)(struct Device* device, struct KernelFunctionPointers* kfp);

    // Other drivers will rather just get the list of devices
    // and look for ones that they can manage.
    bool    (*check)(struct Device* device, struct KernelFunctionPointers* kfp);
    // Assuming a device that this can manage has been found,
    // it means we can add that device to the global list,
    // and initialise the driver for that device.
    int     (*init) (struct AstatineDriver* self);

    // If the driver is deemed unnecessary, the deinit function
    // will release the device.
    void    (*deinit)(struct AstatineDriver* self);

    // Other functions are determined by the driver handler
    // so teletype-drivers will have the ability to draw characters,
    // display drivers can set pixels, etc.
} AstatineDriverFile;

#endif
