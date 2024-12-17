//
// Created by Adithiya Venkatakrishnan on 23/07/2024.
//

#ifndef MEMORY_H
#define MEMORY_H

#define MEM_BLOCK_BYTE_SIZE 32
#define MEM_BLOCK_START     0x100000
#define MEM_BLOCK_END       0x1fffff

extern void init_mem();

extern void*    malloc(int bytes);
extern void*    calloc(int bytes);
extern void*    realloc(void* ptr, u32 bytes);

extern void     free(void* ptr, const int bytes);

#endif //MEMORY_H
