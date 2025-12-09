#ifndef BASEDEVICE_DEVICELOGIC_H
#define BASEDEVICE_DEVICELOGIC_H
#include "device.h"

extern struct dynarray* devices;

u32 get_unique_device_id();

bool search_for_supported_device(struct AstatineDriverFile* driver, struct Device** out_device);
struct Device* register_device(struct Device* device, u32 parent_id);
void unregister_device(u32 id);

#endif