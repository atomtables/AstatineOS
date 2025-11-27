#include <astatine/types.h>
#include <astatine/teletype.h>

bool    exists(struct KernelFunctionPointers* kfp);
int     init(struct KernelFunctionPointers* kfp);
void    deinit(void);
bool    get_mode(struct TeletypeMode* mode);
bool    set_char(u32 x, u32 y, u8 character, u8 color);
u16     get_char(u32 x, u32 y);
bool    clear_screen(u8 color);
bool    set_cursor_position(u32 x, u32 y);

ASTATINE_DRIVER(TeletypeDriver) = {
    .sig = "ASTATINE", 
    .driver_type = 2,
    .name = "VGA BIOS Text-mode Driver", 
    .version = "0.1",
    .author = "Adithiya Venkatakrishnan", 
    .description = "BIOS VGA text-mode driver (80x25).",
    .reserved = {0},

    .verification = {0xEF, 0xBE, 0xAD, 0xDE, 0xEF, 0xBE, 0xAD, 0xDE},
    .exists = exists,
    .init = init,
    .deinit = deinit,
    .get_mode = get_mode,
    .set_char = set_char,
    .get_char = get_char,
    .clear_screen = clear_screen,
    .set_cursor_position = set_cursor_position,
};

ASTATINE_DRIVER_ENTRYPOINT();

void outb(u16 port, u8 val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

bool exists(struct KernelFunctionPointers* kfp) {
    // What would make sense is to make a v86 call to
    // check the current video mode. But it's a horrible
    // pain to make a v86 call, especially since v86 isn't
    // even portable to long mode.

    // Assume that this driver will only be used if the
    // OS has booted in text mode.
    kfp->allow_null_page_read();
    u8 video_mode = *(u8*)0x449;
    if (video_mode == 3 || video_mode == 2) {
        return true;
    } else {
        return false;
    }
    kfp->disallow_null_page();
}

int init(struct KernelFunctionPointers* kfp) {
    // Nothing much to do here.
    // BIOS does everything for us
    *(struct KernelFunctionPointers**)driver.reserved = kfp;
    if (exists(kfp)) return 0;
    else return -1;
}

void deinit() {
    // Will never be ran
    // BIOS does everything for us
}

struct TeletypeMode mode = {
    .width = 80,
    .height = 25,
    .cells = (u8*)0xB8000,
    .cells_valid = 1,
};

bool get_mode(struct TeletypeMode* out) {
    // Read from BIOS memory area
    // Copy local mode struct to caller's struct
    u8* src = (u8*)&mode;
    u8* dst = (u8*)out;
    for (u32 i = 0; i < sizeof(struct TeletypeMode); i++) {
        dst[i] = src[i];
    }
    return true;
}

bool set_char(u32 x, u32 y, u8 character, u8 color) {
    if (x >= mode.width || y >= mode.height) {
        return false;
    }
    u32 index = y * mode.width + x;
    mode.cells[index * 2] = character;
    mode.cells[index * 2 + 1] = color;
    return true;
}

u16 get_char(u32 x, u32 y) {
    if (x >= mode.width || y >= mode.height) {
        return false;
    }
    u32 index = y * mode.width + x;
    u8 cell = mode.cells[index * 2];
    u8 color = mode.cells[index * 2 + 1];
    return (cell << 8) | color;
}

bool clear_screen(u8 color) {
    for (u32 i = 0; i < mode.width * mode.height; i += 2) {
        mode.cells[i] = 0;
        mode.cells[i + 1] = color;
    }
    return true;
}

bool set_cursor_position(u32 x, u32 y) {
    if (x >= mode.width || y >= mode.height) {
        return false;
    }
    u16 pos = (u16)(y * mode.width + x);
    // Send the high byte
    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8)((pos >> 8) & 0xFF));
    // Send the low byte
    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8)(pos & 0xFF));
    return true;
}