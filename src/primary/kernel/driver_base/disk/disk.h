#ifndef DBASE_DISK_H
#define DBASE_DISK_H

#include <driver_base/driver_base.h>
#include <modules/modules.h>
#include "disk_structures.h"

extern struct dynarray* disk_drivers;
extern struct DiskDriver* active_disk_driver;

int register_disk_driver(AstatineDriverFile* driver, Device* device);

#endif