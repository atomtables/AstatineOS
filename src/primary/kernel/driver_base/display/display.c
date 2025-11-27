#include "display.h"
#include <modules/modules.h>
#include <memory/malloc.h>
#include <driver_base/driver_base.h>

struct DisplayMode {
    u32 width;
    u32 height;
    u32 bpp; // bits per pixel

    // these cannot be set
    u32 pitch;
    u32 framebuffer;
    // addr = framebuffer_addr + y * pitch + x * bytes_per_pixel
    u32  red_mask;
    u32  green_mask;
    u32  blue_mask;
    u32  reserved_mask;
} PACKED;

// This is the base structure for display drivers;
// அல்லது GRAPHICAL drivers. This does not include any
// text mode specific functions, as those would be better
// grouped with a text-based framebuffer driver that could also support
// serial terminals.
typedef struct DisplayDriver {
    // "ASTATINE"
    char sig[8]; 
    // should be equal to 1
    u32  driver_type;
    // driver name
    char name[32]; 
    // driver version
    char version[16];
    // driver author
    char author[32];
    // driver description
    char description[64]; 
    // reserved for future use
    char reserved[32]; 

    // reserved for driver verification/signature
    u8   verification[64]; 

    // Checks if the hardware required to use this
    // driver even exists on the system.
    // Returns bool.
    int(* exists)(void);

    // Attempts to initialise the driver. A page of
    // active kernel memory is passed to the driver,
    // as well as the location to some kernel functions
    // are passed.
    // Returns 0 for success, non-zero for failure.
    int(* init)(struct KernelFunctionPointers* kfp);

    // Shuts down the driver.
    void(* deinit)(void);

    // Get the width, height, and color depth of the display.
    struct DisplayMode(* get_mode)(void);

    // Attempt to set the display mode to the specified mode.
    // A pointer to the DisplayMode struct is passed, and if
    // return 0/1, the mode change was successful to the parametres
    // that the driver must set in the struct.
    // Basically, if the provided settings work, then okay, otherwise
    // the driver will try to find the closest match and set those
    // in the struct
    // 0 for success, 1 for success with changes, non-zero/1 for fail.
    int(* set_mode)(struct DisplayMode* mode);

    // Set a pixel at the specified coordinates to the specified color.
    // most likely won't be used anyway
    void(* set_pixel)(u32 x, u32 y, u32 color);
    
    // Get the color of a pixel at the specified coordinates.
    // shouldn't get any use anyway
    u32(* get_pixel)(u32 x, u32 y);

    // Clear the screen to the specified color.
    // just in case it's more efficient.
    // Driver can just loop over screen with set_pixel if not.
    void(* clear_screen)(u32 color);
} DisplayDriver;

DisplayDriver** display_drivers = null;
int display_driver_count = 0;

int install_display_driver(DisplayDriver* driver) {
    if (!verify_driver(driver->verification)) {
        return -1;
    }
    if (driver->driver_type != 1) {
        return -2;
    }

    display_driver_count++;
    if (display_drivers == null) {
        display_drivers = kmalloc(sizeof(DisplayDriver*));
    } else {
        display_drivers = krealloc(display_drivers, sizeof(DisplayDriver*) * (display_driver_count));
    }

    display_drivers[display_driver_count - 1] = driver;
    return 0;
}