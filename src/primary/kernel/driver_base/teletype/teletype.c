#include "teletype.h"
#include <modules/modules.h>
#include <memory/malloc.h>
#include <driver_base/driver_base.h>
#include <basedevice/devicelogic.h>

// Currently active teletype drivers that have a use.
TeletypeDriver* teletype_drivers = null;
TeletypeDriver* active_teletype_driver = null;

int teletype_driver_count = 0;

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
int register_teletype_driver(AstatineDriverBase* driver1, Device* device) {
    if (driver1->device_type != DEVICE_TYPE_TTYPE) {
        return -2;
    }
    if (device == null || device->owned) {
        return -3;
    }

    TeletypeDriverFile* driver = (TeletypeDriverFile*)driver1;

    TeletypeDriver* teletype_driver = kmalloc(sizeof(TeletypeDriver));
    teletype_driver->base.kfp = get_kernel_function_pointers();
    teletype_driver->base.driver_type = driver->base.driver_type;
    teletype_driver->base.device_type = driver->base.device_type;
    teletype_driver->base.probe = driver->base.probe;
    teletype_driver->base.check = driver->base.check;
    teletype_driver->base.init = driver->base.init;
    teletype_driver->base.deinit = driver->base.deinit;
    memcpy(&teletype_driver->functions, &driver->functions, sizeof(TeletypeDriverFunctions));
    teletype_driver->base.device = device;

    device->owned = true;
    device->name = driver->base.name;

    teletype_driver_count++;
    if (teletype_drivers == null) {
        teletype_drivers = kmalloc(sizeof(TeletypeDriver) * teletype_driver_count);
    } else {
        teletype_drivers = krealloc(teletype_drivers, sizeof(TeletypeDriver) * (teletype_driver_count));
    }

    memcpy(&teletype_drivers[teletype_driver_count - 1], teletype_driver, sizeof(TeletypeDriver));
    teletype_driver = &teletype_drivers[teletype_driver_count - 1];

    if (teletype_driver->base.init((AstatineDriver*)teletype_driver) != 0) {
        teletype_driver_count--;
        kfree(teletype_driver);
        teletype_driver = null;
        return -4;
    }

    active_teletype_driver = teletype_driver;

    return 0;
}

