#ifndef ASTATINE_TYPES_H
#define ASTATINE_TYPES_H

typedef unsigned char     u8;
typedef signed char      i8;
typedef unsigned short    u16;
typedef signed short     i16;
typedef unsigned int    u32;
typedef signed int     i32;
typedef unsigned long long    u64;
typedef signed long long     i64;

typedef u8              bool;
#define true            1
#define false           0

typedef float       f32;
typedef double      f64;

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

#define ASTATINE_DRIVER(t) \
    struct t __attribute__((section(".astatine_driver"))) driver

#define ASTATINE_DRIVER_ENTRYPOINT() \
    __typeof__(driver) main() { \
        return driver; \
    }

#endif