#ifndef DYNARRAY_H
#define DYNARRAY_H

#include <modules/modules.h>

typedef struct dynarray {
    bool initialised;
    u32 sizeof_element;
    u32 capacity;
    u32 count;
    void* elements;
} Dynarray;

struct dynarray* dynarray_create(u32 sizeof_element);

void dynarray_destroy(struct dynarray* array);

void* dynarray_get(struct dynarray* array, u32 index);

int dynarray_add(struct dynarray* array, void* element);

void dynarray_remove(struct dynarray* array, u32 index);

#endif