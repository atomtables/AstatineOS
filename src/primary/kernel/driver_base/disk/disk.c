#include "disk.h"
#include <modules/modules.h>
#include <memory/malloc.h>
#include <driver_base/driver_base.h>
#include <basedevice/devicelogic.h>
#include <timer/PIT.h>
#include <modules/dynarray.h>
#include <modules/strings.h>


// Type: DiskDriver
Dynarray* disk_drivers = null;
// VERY TEMPORARY: just test out one disk driver for now.
DiskDriver* active_disk_driver = null;

// This function (as well as any other driver register function) will be ran under two conditions:
// - 1. the driver is for an ISA (or any non-hot-pluggable) device
//      and the driver itself is expected to discover the device
//      More importantly: the driver was just loaded and needs to
//      discover its own device.
// - 2. the kernel ran the DriverBase->check function on a newly
//      discovered device and found that a driver of this type can
//      manage that device
// This assumes that there is an active device, and the location of that device
// is passed to the driver registration function.
int register_disk_driver(AstatineDriverFile* driver1, Device* device) {
    DiskDriverFile* driver = (DiskDriverFile*)driver1;

    DiskDriver disk_driver = {0};
    disk_driver.base.kfp = get_kernel_function_pointers();
    disk_driver.base.driver_type = driver->base.driver_type;
    disk_driver.base.device_type = driver->base.device_type;
    disk_driver.base.probe = driver->base.probe;
    disk_driver.base.check = driver->base.check;
    disk_driver.base.init = driver->base.init;
    disk_driver.base.deinit = driver->base.deinit;
    memcpy(&disk_driver.functions, &driver->functions, sizeof(DiskDriverFunctions));
    disk_driver.base.device = device;
    device->owned = true;
    device->attached_driver = (AstatineDriver*)&disk_driver;
    if (!disk_drivers) disk_drivers = dynarray_create(sizeof(DiskDriver));
    u32 ind = dynarray_add(disk_drivers, &disk_driver);
    DiskDriver* disk_driver_addr = dynarray_get(disk_drivers, ind);
    if (!disk_driver_addr->base.init || disk_driver_addr->base.init((AstatineDriver*)disk_driver_addr) != 0) {
        device->owned = false;
        device->attached_driver = null;
        dynarray_remove(disk_drivers, ind);
        disk_driver_addr = null;
        return -4;
    }

    if (!active_drivers) active_drivers = dynarray_create(sizeof(AstatineDriver*));
    dynarray_add(active_drivers, &disk_driver_addr);
    active_disk_driver = disk_driver_addr;

    char buf[64] = "Loaded driver: ";
    xtoa_padded((u32)active_disk_driver, buf + strlen(buf)-1);

    return 0;
}

