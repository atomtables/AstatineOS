#ifndef VERIFICATION_H
#define VERIFICATION_H

#include <modules/modules.h>
#include <interrupt/isr.h>
#include <fat32/fat32.h>

int verify_driver(u8 items[128]);

struct KernelFunctionPointers {
    // allocate bytes in memory
    // no page allocation required
    void* (*kmalloc)(u32 size);
    void* (*kmalloc_aligned)(u32 size, u32 alignment);
    void  (*kfree)(void* ptr);
    void* (*krealloc)(void* ptr, u32 size);

    // install irqs
    void  (*PIC_install)(u32 i, void (*handler)(struct registers *));

    // paging
    void  (*allow_null_page_read)();
    void  (*disallow_null_page)();
};


struct KernelFunctionPointers* get_kernel_function_pointers();

typedef struct AstatineDriverBase {
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
} AstatineDriverBase;

int attempt_install_driver(File* file);

#endif