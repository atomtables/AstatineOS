// write to a file or to a device
// 0 is stdin, 1 is stdout, 2 is stderr
#include "calls.h"
#include <modules/strings.h>
#include <interrupt/isr.h>
#include <display/simple/display.h>

// Unlike a string print, write should not stop at null bytes.
// Input should be sanitised before being sent, otherwise users may
// get weird output.
void write(struct registers* regs) {
    if (regs->ebx < (sizeof(open_fds) / sizeof(struct fd))) {
        struct fd* filedesc = &open_fds[regs->ebx];
        if (!filedesc->exists) {
            return;
            //return -1; // fd does not exist
        }
        if (filedesc->type == 0) {
            // We are opening some sort of device. This 
            // will not use the conventional file system driver
            // Let's check which file it is. 
            if (strcmp(filedesc->identifier, "/Devices/stdout") == 0) {
                // We are writing to a stdout.
                // This function is temporary since it's hard coded
                // but even in finished code, prevent writing to stdin.
                // Now we have a int fd (ebx), const void buf[count] (ecx), size_t count (edx)
                // We should print this straight into the console using printf.
                char* buf = (char*)regs->ecx;
                u32 count = regs->edx;
                u32 i;
                char temp[2] = {0};
                for (i = 0; i < count; i++) {
                    temp[0] = buf[i];
                    print(temp);
                }
                return;
                //return i;
            } else if (strcmp(filedesc->identifier, "/Devices/stderr") == 0) {
                // Same concept but we'll print in red instead.
                char* buf = (char*)regs->ecx;
                u32 count = regs->edx;
                u32 i;
                char temp[2] = {0};
                for (i = 0; i < count; i++) {
                    temp[0] = buf[i];
                    print_color(temp, COLOR_LIGHT_RED);
                }
                return;
                // return i;
            } else {
                return;// -1; // unsupported device
            }
        } else if (filedesc->type == 1) {
            return; //-1; // unsupported file type
        } else return;// -1;
    } else {
        // For this temporary implementation we will only allow 256 open file descriptors.
        return;// -1; // invalid fd
    }
}