//
// Created by Adithiya Venkatakrishnan on 24/07/2024.
//

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <modules/modules.h>

extern void fatal_error(const int code, char* reason, void* function, u32** registers);

#endif //EXCEPTION_H
