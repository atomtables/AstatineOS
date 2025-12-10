#include <modules/modules.h>
#include <ps2/keyboard.h>
#include <systemcalls/calls/calls.h>
#include <driver_base/teletype/teletype.h>
#include <modules/strings.h>

int teletype_setmode(struct fd* self, u8 mode) {
    // teletypes only support one mode for now
    return 0;
}

extern struct fop teletype_fops;

struct teletype_packet {
    u8 yes;
    u32 x;
    u32 y;
    char* buffer;
    u32 size;
    u8 with_color;
    u8 color;
    u8 move_cursor;
};

int teletype_open(struct fd* self, char* identifier, u8 mode) {
    // nothing special to do
    self->exists = true;
    self->fops = &teletype_fops;
    return 0;
}

int teletype_write(struct fd* self, const void* buffer, u32 size) {
    struct teletype_packet* buf = (struct teletype_packet*)buffer;
    active_teletype_driver->functions.set_string(
        active_teletype_driver,
        buf->x,
        buf->y,
        buf->buffer,
        buf->with_color ? buf->color : 0x0f
    );
    static struct TeletypeMode mode;
    if (!mode.width) active_teletype_driver->functions.get_mode(active_teletype_driver, &mode);
    buf->x += strlen(buf->buffer);
    if (mode.width && buf->x >= mode.width) {
        buf->x = 0;
        buf->y += 1;
    }
    if (buf->move_cursor) {
        active_teletype_driver->functions.set_cursor_position(active_teletype_driver, buf->x, buf->y);
    }
    return size;
}

// Read from the teletype (keyboard input)
int teletype_read(struct fd* self, void* buffer, u32 size) {
    char* out = (char*)buffer;
    for (u32 i = 0; i < size; i++) {
        char ch = (char)wait_for_keypress();
        out[i] = ch;
    }
    return size;
}

struct fop teletype_fops = {
    .read = teletype_read,
    .write = teletype_write,
    .open = teletype_open,
    .close = null,
    .setmode = teletype_setmode
};