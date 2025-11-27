#include "teletype.h"
#include <modules/modules.h>
#include <memory/malloc.h>
#include <driver_base/driver_base.h>

TeletypeDriver* teletype_drivers = null;
TeletypeDriver* active_teletype_driver = null;

int teletype_driver_count = 0;

int install_teletype_driver(AstatineDriverBase* driver1) {
    if (!verify_driver(driver1->verification)) {
        return -1;
    }
    if (driver1->driver_type != 2) {
        return -2;
    }
    
    TeletypeDriver* driver = (TeletypeDriver*)driver1;
    if (!driver->exists(get_kernel_function_pointers())) {
        return -3;
    }

    teletype_driver_count++;
    if (teletype_drivers == null) {
        teletype_drivers = kmalloc(sizeof(TeletypeDriver) * teletype_driver_count);
    } else {
        teletype_drivers = krealloc(teletype_drivers, sizeof(TeletypeDriver) * (teletype_driver_count));
    }

    memcpy(&teletype_drivers[teletype_driver_count - 1], driver, sizeof(TeletypeDriver));
    driver = &teletype_drivers[teletype_driver_count - 1];

    if (driver->init(get_kernel_function_pointers()) != 0) {
        return -4;
    }

    active_teletype_driver = &teletype_drivers[teletype_driver_count - 1];
    return 0;
}