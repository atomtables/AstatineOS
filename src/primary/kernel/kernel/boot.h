//
// Created by Adithiya Venkatakrishnan on 27/6/2026.
//

#ifndef ASTATINEOS_BOOT_H
#define ASTATINEOS_BOOT_H

#include "modules/modules.h"

struct astatine_boot_configuration {
    enum astatine_boot_display: u8 {
        BOOT_DISPLAY_TEXTMODE = 0,
        BOOT_DISPLAY_320x200 = 1
    } boot_display_as;
} PACKED;

extern struct astatine_boot_configuration boot_vars;

#endif //NETWORKOS_BOOT_H
