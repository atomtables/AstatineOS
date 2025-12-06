#include "device.h"
#include <modules/modules.h>
#include <memory/malloc.h>
#include <modules/modules.h>
#include <driver_base/driver_base.h>
#include <modules/dynarray.h>

struct dynarray* devices;

// Luckily, this resizes the array of devices
// and also reallocates memory for the device
// so we don't have to worry about size differences.
Device* register_device(Device* device) {
    if (!devices) devices = dynarray_create(sizeof(Device));
    int index = dynarray_add(devices, device);
    return (Device*)dynarray_get(devices, index);
}

void unregister_device(u32 id) {
    for (int i = 0; i < devices->count; i++) {
        if (((Device*)(devices->elements))[i].id == id) {
            dynarray_remove(devices, i);
            return;
        }
    }
}

bool search_for_supported_device(AstatineDriverFile* driver, Device* * out_device) {
    if (!driver->check) {
        return false;
    }
    for (int i = 0; i < devices->count; i++) {
        Device* device = dynarray_get(devices, i);
        if (device->type != driver->device_type || device->type != DEVICE_TYPE_UNKNOWN) {
            continue;
        }
        if (device->conn != driver->driver_type || device->conn != CONNECTION_TYPE_UNKNOWN) {
            continue;
        }
        // driver is responsible for owning device
        // unowning a device can be done by the driver
        // but will be done after calling deinit just in case
        if (device->owned) {
            continue;
        }
        if (driver->check(device, get_kernel_function_pointers())) {
            *out_device = device;
            return true;
        }
    }
    return false;
}

u32 get_unique_device_id() {
    static u32 current_id = 1;
    return current_id++;
}
