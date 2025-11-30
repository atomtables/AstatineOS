#include "device.h"
#include <modules/modules.h>
#include <memory/malloc.h>
#include <modules/modules.h>
#include <driver_base/driver_base.h>

Device** devices;
int device_count = 0;
static int device_size = 0;

// Luckily, this resizes the array of devices
// and also reallocates memory for the device
// so we don't have to worry about size differences.
Device* register_device(Device* device) {
    device_count++;
    if (devices == null) {
        device_size = 4;
        devices = kmalloc(sizeof(Device*) * device_size);
    } else if (device_count > device_size) {
        devices = krealloc(devices, sizeof(Device*) * device_size * 2);
        device_size *= 2;
    }

    devices[device_count - 1] = kmalloc(device->size);
    memcpy(devices[device_count - 1], device, device->size);
    return devices[device_count - 1];
}

void unregister_device(u32 id) {
    for (int i = 0; i < device_count; i++) {
        if (devices[i]->id == id) {
            kfree(devices[i]);
            // shift remaining devices down
            for (int j = i; j < device_count - 1; j++) {
                devices[j] = devices[j + 1];
            }
            device_count--;
            return;
        }
    }
}

bool search_for_supported_device(AstatineDriverBase* driver, Device* * out_device) {
    if (!driver->check) {
        return false;
    }
    for (int i = 0; i < device_count; i++) {
        if (driver->check(devices[i], get_kernel_function_pointers())) {
            *out_device = devices[i];
            return true;
        }
    }
    return false;
}

u32 get_unique_device_id() {
    static u32 current_id = 1;
    return current_id++;
}
