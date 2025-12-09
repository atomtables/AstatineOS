#include "discovery.h"
#include <modules/modules.h>
#include <basedevice/devicelogic.h>
#include <driver_base/driver_base.h>
#include <memory/malloc.h>

void discover_isa_devices() {
    PlatformDevice device;
    device.base.name = "VGA Text Display";
    device.base.type = DEVICE_TYPE_TTYPE;
    device.base.conn = CONNECTION_TYPE_IO;
    device.base.size = sizeof(PlatformDevice);
    device.base.owned = false;
    // This is the unique platform ID for the VGA text device
    device.platform_id = ISA_DEVICE_VGA_TEXT;
    
    register_device((Device*)&device, null);
}