#include <driver_base/driver_base_structures.h>
#include <basedevice/devicelogic.h>
#include <driver_base/driver_base.h>
#include <modules/dynarray.h>
#include "generic.h"

Dynarray* generic_drivers;

int register_astatine_generic_driver(AstatineDriverFile* driver1, Device* device) {
    AstatineDriver driver;
    driver.kfp = get_kernel_function_pointers();
    driver.driver_type = driver1->driver_type;
    driver.device_type = driver1->device_type;
    driver.probe = driver1->probe;
    driver.check = driver1->check;
    driver.init = driver1->init;
    driver.deinit = driver1->deinit;
    driver.device = device;
    device->owned = true;
    if (!generic_drivers) generic_drivers = dynarray_create(sizeof(AstatineDriver));
    dynarray_add(generic_drivers, &driver);
    AstatineDriver* driver_addr = dynarray_get(generic_drivers, generic_drivers->count - 1);

    if (driver_addr->init((AstatineDriver*)driver_addr) != 0) {
        device->owned = false;
        device->attached_driver = null;
        dynarray_remove(generic_drivers, generic_drivers->count - 1);
        driver_addr = null;
        return -4;
    }

    device->attached_driver = (AstatineDriver*)driver_addr;

    if (!active_drivers) active_drivers = dynarray_create(sizeof(AstatineDriver*));
    dynarray_add(active_drivers, &driver1);

    return 0;
}

