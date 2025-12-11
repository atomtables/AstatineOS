#include <interrupt/isr.h>
#include <display/simple/display.h>
#include <elfloader/elfloader.h>
#include <modules/strings.h>
#include "calls.h"

int loadnew(struct registers* regs) {
    if (!regs->ebx) {
        regs->eax = -1; // invalid pointer
        regs->ecx = 0;
        regs->edx = 0;
        regs->esi = 0;
        return 0;
    }
    // copy path into kernel memory before freeing user pages
    const char* user_path = (const char*)regs->ebx;
    const u32 max_len = 256;
    u32 len = 0;
    while (len < max_len && user_path[len] != '\0') {
        len++;
    }
    if (len == max_len) {
        regs->eax = -1; // path too long / unterminated
        regs->ecx = 0;
        regs->edx = 0;
        regs->esi = 0;
        return 0;
    }

    char path[257];
    memcpy(path, user_path, len + 1);

    if (is_elf(path) != 0) {
        regs->eax = -1; // not an elf
        regs->ecx = 0;
        regs->edx = 0;
        regs->esi = 0;
        return 0;
    }
    extern u32* addrs_top;
    extern u32 count_top;
    extern u32 entrypoint_top;
    // we free and then restart the load process.
    elf_free(addrs_top, count_top);
    elf_load_and_run(path);
    while(1);
    return 0;
}