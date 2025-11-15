#ifndef MALLOC_H
#define MALLOC_H

#include <modules/modules.h>

void kmalloc_init(void* start, u32 size);
void* kmalloc(u32 size);
void kfree(void* ptr);
void* kcalloc(const int bytes);
void* krealloc(void* ptr, u32 bytes);

#endif //MALLOC_H