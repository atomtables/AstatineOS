#ifndef DBASE_TELETYPE_H
#define DBASE_TELETYPE_H

#include <driver_base/driver_base.h>
#include <modules/modules.h>
#include "teletype_structures.h"

extern struct dynarray* teletype_drivers;
extern TeletypeDriver* active_teletype_driver;

int register_teletype_driver(AstatineDriverFile* driver, Device* device);

#endif