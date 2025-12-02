#include "terminal.h"
#include <systemcalls/calls/calls.h>
#include <modules/modules.h>
#include <ps2/keyboard.h>
#include <display/simple/display.h>

struct fop terminal_fops = {0};

int terminal_read(struct fd* self, void* buffer, u32 size) {
    u32 i;
    static char item[2];
    for (i = 0; i < size; i++) {
        ((char*)buffer)[i] = (char)wait_for_keypress();
        item[0] = ((char*)buffer)[i];
        if (((char*)buffer)[i] == 0x08) {
            ((char*)buffer)[i] = '\0';
            ((char*)buffer)[i - 1] = '\0';
            i -= 2;
            item[0] = ' ';
        }
        print(item);
        if (((char*)buffer)[i] == '\n') {
            break;
        }
    }
    return i;
}

int terminal_write(struct fd* self, const void* buffer, u32 size) {
    u32 i;
    char temp[2] = {0};
    for (i = 0; i < size; i++) {
        temp[0] = ((char*)buffer)[i];
        if ((int)(self->internal) == 2) {
            print_color(temp, 0x4F); // red on white for stderr
        } else {
            print(temp);
        }
    }
    return i;
}


void terminal_install() {
    terminal_fops.read = terminal_read;
    terminal_fops.write = terminal_write;

    struct fd* stdin = &open_fds[0];
    stdin->exists = true;
    stdin->position = 0;
    stdin->internal = null;
    stdin->fops = &terminal_fops;
    
    struct fd* stdout = &open_fds[1];
    stdout->exists = true;
    stdout->position = 0;
    stdout->internal = null;
    stdout->fops = &terminal_fops;
    struct fd* stderr = &open_fds[2];
    stderr->exists = true;
    stderr->position = 0;
    stderr->internal = (void*)2;
    stderr->fops = &terminal_fops;
}

