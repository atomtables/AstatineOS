#include <basedevice/devicelogic.h>
void discover_pci_devices() {
    PCIDevice device;
    device.base.name = "VGA Text Display";
    device.base.type = DEVICE_TYPE_CONTROLLER;
    device.base.conn = CONNECTION_TYPE_PCI;
    device.base.size = sizeof(PCIDevice);
    device.base.owned = false;
    // BARS for the PCI IDE controller
    device.bars[0] = 0x1F0; // Primary Command Block
    device.bars[1] = 0x3F6; // Primary Control Block
    device.bars[2] = 0x170; // Secondary Command Block
    device.bars[3] = 0x376; // Secondary Control Block
    device.bars[4] = 0;     // Bus Master IDE (not used here)
    device.class_code = 0x01; // Mass Storage Controller
    device.subclass = 0x01;   // IDE Controller
    
    register_device((Device*)&device, null);
}