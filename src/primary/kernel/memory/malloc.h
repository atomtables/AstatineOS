#ifndef MALLOC_H
#define MALLOC_H

#include <modules/modules.h>

#define MEM_BLOCK_BYTE_SIZE 32
#define MEM_BLOCK_START     0x100000
#define MEM_BLOCK_END       0x3fffff

void* kmalloc(u32 size);
void* kmalloc_aligned(const u32 bytes, const u32 alignment);
void kfree(void* ptr);
void* kcalloc(const u32 bytes);
void* krealloc(void* ptr, u32 bytes);

#endif //MALLOC_H