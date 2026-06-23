//
// Created by Adithiya Venkatakrishnan on 23/07/2024.
//

#ifndef MEMORY_H
#define MEMORY_H

#include <modules/modules.h>

extern void init_mem();

extern u32 alloc_frames(u32 length, u32 before, u32 after);
extern u32 alloc_frame();
extern void free_frames(u32 base, u32 length);
extern void free_frame(u32 base);


#endif //MEMORY_H
