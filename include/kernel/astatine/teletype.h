#ifndef ASTATINE_TELETYPE_H
#define ASTATINE_TELETYPE_H

#include "types.h"

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

typedef struct TeletypeDriver {
    u32     size;
    // "ASTATINE"
    char    sig[8]; 
    // should be equal to 2
    u32     driver_type;
    // driver name
    char    name[32]; 
    // driver version
    char    version[16];
    // driver author
    char    author[32];
    // driver description
    char    description[64]; 
    // reserved for future use
    char    reserved[32]; 

    // reserved for driver verification/signature
    u8      verification[64]; 

    // Checks if the hardware required to use this
    // driver even exists on the system.
    // Returns bool.
    bool(*  exists)(struct KernelFunctionPointers* kfp);

    // Attempts to initialise the driver. A page of
    // active kernel memory is passed to the driver,
    // as well as the location to some kernel functions
    // are passed.
    // Returns 0 for success, non-zero for failure.
    int(*   init)(
        struct KernelFunctionPointers* kfp
    );

    // Shuts down the driver.
    void(*  deinit)(void);

    // Get the width, height, framebuffer of display
    bool(*  get_mode)(struct TeletypeMode* mode);

    // Set a pixel at the specified coordinates to the specified color.
    // most likely won't be used anyway
    bool(*  set_char)(u32 x, u32 y, u8 character, u8 color);
    
    // Get the color of a pixel at the specified coordinates.
    // shouldn't get any use anyway
    u16(*   get_char)(u32 x, u32 y);
    // Clear the screen to the specified attribute byte.
    // just in case it's more efficient.
    // Driver can just loop over screen with set_pixel if not.
    bool(*  clear_screen)(u8 color);

    bool(*  set_cursor_position)(u32 x, u32 y);
} TeletypeDriver;

#endif