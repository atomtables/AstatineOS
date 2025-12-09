#ifndef ASTATINE_TELETYPE_H
#define ASTATINE_TELETYPE_H

#if __has_include ("types.h")
#include "types.h"
#endif
#if __has_include (<modules/modules.h>)
#include <modules/modules.h>
#endif

#if __has_include (<basedevice/device.h>)
#include <basedevice/device.h>
#endif
#if __has_include ("device.h")
#include "device.h"
#endif

#if __has_include (<driver_base/driver_base.h>)
#include <driver_base/driver_base.h>
#endif
#if __has_include ("drivers.h")
#include "drivers.h"
#endif

typedef struct TeletypeDriverFunctions TeletypeDriverFunctions;
typedef struct TeletypeDriverFile TeletypeDriverFile;
// This is the active initialised driver
typedef struct TeletypeDriver TeletypeDriver;

// Note: any implementation of a teletype driver
// may only have 2-byte wide characters. if this
// is not true, the framebuffer must be set to 0
// and the driver must handle all character drawing
struct TeletypeMode {
    u32     width;
    u32     height;

    // these cannot be set
    u8*     cells;
    u8      cells_valid;
    // addr = framebuffer_addr + y * pitch + x * bytes_per_pixel
} PACKED;

struct TeletypeDriverFunctions {
    // Get the width, height, framebuffer of display
    bool(*  get_mode)(struct TeletypeDriver* self, struct TeletypeMode* mode);

    // Set a pixel at the specified coordinates to the specified color.
    // most likely won't be used anyway
    bool(*  set_char)(struct TeletypeDriver* self, u32 x, u32 y, u8 character, u8 color);
    
    // Get the color of a pixel at the specified coordinates.
    // shouldn't get any use anyway
    u16(*   get_char)(struct TeletypeDriver* self, u32 x, u32 y);
    // Clear the screen to the specified attribute byte.
    // just in case it's more efficient.
    // Driver can just loop over screen with set_pixel if not.
    bool(*  clear_screen)(struct TeletypeDriver* self, u8 color);
    
    bool(*  set_cursor_position)(struct TeletypeDriver* self, u32 x, u32 y);

    bool(*  set_string)(struct TeletypeDriver* self, u32 x, u32 y, const char* str, u8 color);
};
struct TeletypeDriverFile {
    AstatineDriverFile base;
    TeletypeDriverFunctions functions;
};
struct TeletypeDriver {
    AstatineDriver base;
    TeletypeDriverFunctions functions;
};

#endif