#include "device.h"
#include "devicelogic.h"
#include <memory/malloc.h>
#include <modules/modules.h>
#include <driver_base/driver_base.h>
#include <modules/dynarray.h>

// Device**
struct dynarray* devices;

static Device* get_device_by_id(u32 id) {
    if (!devices) return null;
    if (!id) return null;
    for (int i = 0; i < devices->count; i++) {
        Device* device = *(Device**)dynarray_get(devices, i);
        if (device->id == id) {
            return device;
        }
    }
    return null;
}

// Luckily, this resizes the array of devices
// and also reallocates memory for the device
// so we don't have to worry about size differences.
Device* register_device(Device* device, u32 parent_id) {
    if (!device->size) return null;

    Device* parent = get_device_by_id(parent_id);

    if (!devices) devices = dynarray_create(sizeof(Device*));

    // Let's hope they set the size correctly
    Device* device_new = kmalloc(device->size);
    memcpy(device_new, device, device->size);
    device_new->parent = device_new->left_sibling 
        = device_new->right_sibling = device_new->child = null;
    device_new->attached_driver = null;

    // Let's make sure the ID doesn't conflict with another device
    bool id_conflict = true;
    while (id_conflict) {
        id_conflict = false;
        for (int i = 0; i < devices->count; i++) {
            Device* other_device = *(Device**)dynarray_get(devices, i);
            if (other_device == device_new) continue;
            if (other_device->id == device_new->id) {
                device_new->id = get_unique_device_id();
                id_conflict = true;
                break;
            }
        }
    }
    device_new->owned = false;

    if (parent) {
        device_new->parent = parent;
        // let's add it as a child to the parent
        if (!parent->child) {
            parent->child = device_new;
        } else {
            // add it as a sibling to the last child
            Device* sibling = parent->child;
            while (sibling->right_sibling) {
                sibling = sibling->right_sibling;
            }
            sibling->right_sibling = device_new;
            device_new->left_sibling = sibling;
        }
    }

    // We should now search for a driver that can manage this device
    if (available_drivers)
        for (int i = 0; i < available_drivers->count; i++) {
            AstatineDriverIndex* driver = dynarray_get(available_drivers, i);
            if (driver->device_type != device_new->type && driver->device_type != DEVICE_TYPE_UNKNOWN) {
                continue;
            }
            if (driver->driver_type != device_new->conn && driver->driver_type != CONNECTION_TYPE_UNKNOWN) {
                continue;
            }
            // ok so this driver may be compatible
            // for now let's pretend to load it in (I don't feel like doing that
            // yet because it's a little complex) by just getting the active instance
            // that it's loaded at
            if (!driver->loaded) {
                continue;
            }
            AstatineDriverFile* driver_file = driver->active_instance;
            // driver is responsible for owning device
            // unowning a device can be done by the driver
            // but will be done after calling deinit just in case
            if (device_new->owned) {
                continue;
            }
            if (driver_file->check != 0 && driver_file->check(device_new, get_kernel_function_pointers())) {
                // This driver base is able to manage this device
                // so we can create an instance of the driver
                // through the driver's manager registration. (like register_teletype_driver)
                if (initialise_driver_with_subsystem(driver_file, device_new) == 0) {
                    device_new->owned = true;
                    break;
                }
            } 
        }
        
    dynarray_add(devices, &device_new);
    
    return (Device*)device_new;
}

// This will run the deinit function of the driver
// so we shouldn't call that in deinit code of the
// user's driver.
void unregister_device(u32 id) {
    if (!devices) return;
    for (int i = 0; i < devices->count; i++) {
        Device* device = dynarray_get(devices, i);
        if (device->id == id) {
            if (device->owned && device->attached_driver && device->attached_driver->deinit) {
                device->attached_driver->deinit(device->attached_driver);
            }
            if (device->parent) {
                // remove from parent's child/sibling list
                for (Device* sibling = device->parent->child; sibling != null; sibling = sibling->right_sibling) {
                    if (sibling == device) {
                        if (sibling->left_sibling) {
                            sibling->left_sibling->right_sibling = sibling->right_sibling;
                        } else {
                            // this was the first child
                            device->parent->child = sibling->right_sibling;
                        }
                        if (sibling->right_sibling) {
                            sibling->right_sibling->left_sibling = sibling->left_sibling;
                        }
                        break;
                    }
                }
            }
            // We do not delete children of the device, that is the job
            // of the driver (which usually enumerates these children) anyway
            // Orphaned children are unfortunately a part of this operating system
            // =====
            // Just kidding, we do murder orphans unfortunately, which probably isn't the best
            // move...
            if (device->child) {
                for (Device* child = device->child; child != null;) {
                    Device* new_child = child->right_sibling;
                    unregister_device(child->id);
                    child = new_child;
                }
            } 
            
            device->parent = null;
            device->left_sibling = null;
            device->right_sibling = null;
            
            kfree(device);
            dynarray_remove(devices, i);
            return;
        }
    }
}

bool search_for_supported_device(AstatineDriverFile* driver, Device** out_device) {
    if (!devices) return false;
    if (!driver->check) {
        return false;
    }
    for (int i = 0; i < devices->count; i++) {
        Device* device = dynarray_get(devices, i);
        if (device->type != driver->device_type && device->type != DEVICE_TYPE_UNKNOWN) {
            continue;
        }
        if (device->conn != driver->driver_type && device->conn != CONNECTION_TYPE_UNKNOWN) {
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
    if (current_id == 0) current_id = 1;
    return current_id++;
}
