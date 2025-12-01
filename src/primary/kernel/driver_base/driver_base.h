#ifndef VERIFICATION_H
#define VERIFICATION_H

#include <modules/modules.h>
#include <interrupt/isr.h>
#include <fat32/fat32.h>
#include "driver_base_structures.h"

int verify_driver(u8 items[128]);

struct KernelFunctionPointers* get_kernel_function_pointers();

extern AstatineDriverFile** available_drivers;
extern u32 available_driver_count;

int attempt_install_driver(File* file);

#endif