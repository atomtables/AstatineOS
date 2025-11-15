//
// Created by Adithiya Venkatakrishnan on 23/07/2024.
//

#ifndef MEMORY_H
#define MEMORY_H

#define MEM_BLOCK_BYTE_SIZE 32
#define MEM_BLOCK_START     0x100000
#define MEM_BLOCK_END       0x3fffff

#include <modules/modules.h>

extern void init_mem();

extern u32 alloc_frame();

#endif //MEMORY_H
