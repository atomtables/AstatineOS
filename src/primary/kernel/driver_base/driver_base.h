#ifndef VERIFICATION_H
#define VERIFICATION_H

#include <modules/modules.h>
#include <interrupt/isr.h>
#include <fat32/fat32.h>
#include "driver_base_structures.h"

typedef struct AstatineDriverIndex {
    bool    loaded;
    AstatineDriverFile*      
            active_instance;
    char*   path;
    u32     driver_type;
    u32     device_type;
} AstatineDriverIndex;

int verify_driver(u8 items[128]);

struct KernelFunctionPointers* get_kernel_function_pointers();

extern struct dynarray* available_drivers;
extern struct dynarray* active_drivers;
extern struct dynarray* loaded_drivers;

int attempt_install_driver(File* file, char* path);

u32 postregister_driver(AstatineDriverFile* driver);
int initialise_driver_with_subsystem(AstatineDriverFile* driver, Device* device);
int install_driver(AstatineDriverFile* driver, char* path);

#endif