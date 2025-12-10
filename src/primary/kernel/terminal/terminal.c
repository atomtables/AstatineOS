#include "terminal.h"
#include <systemcalls/calls/calls.h>
#include <modules/modules.h>
#include <ps2/keyboard.h>
#include <display/simple/display.h>

struct fop terminal_fops = {0};

static const u8 ANSI_DEFAULT_COLOR = VGA_TEXT_COLOR(COLOR_LIGHT_GREY, COLOR_BLACK);

#define STDIN_MODE_RAW    0x2
#define STDIN_MODE_NOECHO 0x4

static u8 apply_sgr_code(u8 current, int code) {
    static const u8 ansi_fg[8] = {
        COLOR_BLACK,      // 30
        COLOR_RED,        // 31
        COLOR_GREEN,      // 32
        COLOR_BROWN,      // 33 (yellow)
        COLOR_BLUE,       // 34
        COLOR_MAGENTA,    // 35
        COLOR_CYAN,       // 36
        COLOR_LIGHT_GREY  // 37
    };
    static const u8 ansi_fg_bright[8] = {
        COLOR_DARK_GREY,     // 90
        COLOR_LIGHT_RED,     // 91
        COLOR_LIGHT_GREEN,   // 92
        COLOR_YELLOW,        // 93
        COLOR_LIGHT_BLUE,    // 94
        COLOR_LIGHT_MAGENTA, // 95
        COLOR_LIGHT_CYAN,    // 96
        COLOR_WHITE          // 97
    };

    u8 fg = current & 0x0F;
    u8 bg = (current >> 4) & 0x0F;

    if (code == 0) return ANSI_DEFAULT_COLOR;
    if (code == 1) { if (fg < 8) fg += 8; return (bg << 4) | fg; }
    if (code == 22) { if (fg >= 8) fg -= 8; return (bg << 4) | fg; }
    if (code == 39) { fg = ANSI_DEFAULT_COLOR & 0x0F; return (bg << 4) | fg; }
    if (code == 49) { bg = ANSI_DEFAULT_COLOR >> 4; return (bg << 4) | fg; }

    if (code >= 30 && code <= 37) {
        fg = ansi_fg[code - 30];
        return (bg << 4) | fg;
    }
    if (code >= 90 && code <= 97) {
        fg = ansi_fg_bright[code - 90];
        return (bg << 4) | fg;
    }
    if (code >= 40 && code <= 47) {
        bg = ansi_fg[code - 40];
        return (bg << 4) | fg;
    }
    if (code >= 100 && code <= 107) {
        bg = ansi_fg_bright[code - 100];
        return (bg << 4) | fg;
    }

    return current;
}
static void flush_segment(char* segment, u32* len, u8 color) {
    if (*len == 0) return;
    segment[*len] = '\0';
    print_color(segment, color);
    *len = 0;
}

int terminal_setmode(struct fd* self, u8 mode) {
    if (!self) return -1;
    printf("Setting terminal mode to %x\n", mode);
    self->mode = mode;
    return 0;
}

int terminal_read(struct fd* self, void* buffer, u32 size) {
    char* out = (char*)buffer;
    bool raw = self && (self->mode & STDIN_MODE_RAW);
    bool echo = !(self && (self->mode & STDIN_MODE_NOECHO));

    if (raw) {
        for (u32 i = 0; i < size; i++) {
            char ch = (char)wait_for_keypress();
            out[i] = ch;
            if (echo) {
                char tmp[2] = { ch, '\0' };
                print_color(tmp, 0x0f);
            }
        }
        return size;
    }

    u32 i = 0;
    static char item[2];
    for (; i < size; i++) {
        char ch = (char)wait_for_keypress();
        out[i] = ch;
        item[0] = ch;

        if (ch == 0x08) { // backspace
            if (i > 0) {
                out[i] = '\0';
                out[i - 1] = '\0';
                i -= 2; // loop increment will advance to correct slot
                item[0] = ' ';
                if (echo) print_color(item, 0x0f);
            } else {
                i = (u32)-1; // will become 0 after loop increment
            }
            continue;
        }

        if (echo) print_color(item, 0x0f);
        if (ch == '\n') {
            i++; // include newline in count
            break;
        }
    }
    return i;
}

int terminal_write(struct fd* self, const void* buffer, u32 size) {
    (void)self;

    static u8 current_color = VGA_TEXT_COLOR(COLOR_LIGHT_GREY, COLOR_BLACK);
    char segment[128];
    u32 seg_len = 0;

    for (u32 i = 0; i < size; i++) {
        char ch = ((char*)buffer)[i];

        if (ch == '\x1b' && i + 1 < size && ((char*)buffer)[i + 1] == '[') {
            flush_segment(segment, &seg_len, current_color);
            i += 2;
            int codes[8] = {0};
            int code_count = 0;
            int value = 0;
            bool have_value = false;
            char terminator = 0;

            for (; i < size; i++) {
                char c = ((char*)buffer)[i];
                if (c >= '0' && c <= '9') {
                    value = value * 10 + (c - '0');
                    have_value = true;
                    continue;
                }
                if (c == ';') {
                    if (code_count < 8) codes[code_count++] = have_value ? value : 0;
                    value = 0;
                    have_value = false;
                    continue;
                }
                terminator = c;
                break;
            }

            if (have_value && code_count < 8) codes[code_count++] = value;

            if (terminator == 'm') {
                if (code_count == 0) current_color = ANSI_DEFAULT_COLOR;
                for (int k = 0; k < code_count; k++) {
                    current_color = apply_sgr_code(current_color, codes[k]);
                }
            }

            continue;
        }

        segment[seg_len++] = ch;
        if (seg_len == sizeof(segment) - 1) {
            flush_segment(segment, &seg_len, current_color);
        }
    }

    flush_segment(segment, &seg_len, current_color);
    return size;
}


void terminal_install() {
    terminal_fops.read = terminal_read;
    terminal_fops.write = terminal_write;
    terminal_fops.setmode = terminal_setmode;

    struct fd* stdin = &open_fds[0];
    stdin->exists = true;
    stdin->mode = 0;
    stdin->position = 0;
    stdin->internal = null;
    stdin->fops = &terminal_fops;
    
    struct fd* stdout = &open_fds[1];
    stdout->exists = true;
    stdout->mode = 0;
    stdout->position = 0;
    stdout->internal = null;
    stdout->fops = &terminal_fops;
    struct fd* stderr = &open_fds[2];
    stderr->exists = true;
    stderr->mode = 0;
    stderr->position = 0;
    stderr->internal = (void*)2;
    stderr->fops = &terminal_fops;
}

