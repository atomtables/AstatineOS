#ifndef BASEDEVICE_DEVICELOGIC_H
#define BASEDEVICE_DEVICELOGIC_H
#include "device.h"
#include <driver_base/driver_base_structures.h>

extern struct dynarray* devices;

bool search_for_supported_device(AstatineDriver* driver, Device* out_device);
Device* register_device(Device* device);

u32 get_unique_device_id();

#endif