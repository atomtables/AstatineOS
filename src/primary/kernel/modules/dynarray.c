#include "modules.h"
#include "dynarray.h"
#include <memory/malloc.h>

static void dynarray_resize(struct dynarray* array, u32 new_capacity) {
    array->elements = krealloc(array->elements, array->sizeof_element * new_capacity);
    array->capacity = new_capacity;
}

struct dynarray* dynarray_create(u32 sizeof_element) {
    struct dynarray* array = kmalloc(sizeof(Dynarray));
    array->initialised = true;
    array->sizeof_element = sizeof_element;
    array->capacity = 4;
    array->count = 0;
    array->elements = kmalloc(sizeof_element * array->capacity);
    return array;
}

void dynarray_destroy(struct dynarray* array) {
    kfree(array->elements);
    kfree(array);
}

void* dynarray_get(struct dynarray* array, u32 index) {
    if (index >= array->count) {
        return null;
    }
    return (u8*)array->elements + (index * array->sizeof_element);
}

int dynarray_add(struct dynarray* array, void* element) {
    if (array->count >= array->capacity) {
        array->capacity *= 2;
        array->elements = krealloc(array->elements, array->sizeof_element * array->capacity);
    }
    memcpy((u8*)array->elements + (array->count * array->sizeof_element), element, array->sizeof_element);
    array->count++;
    return array->count - 1;
}

void dynarray_remove(struct dynarray* array, u32 index) {
    if (index >= array->count) {
        return;
    }
    if (index < array->count - 1) {
        memcpy((u8*)array->elements + (index * array->sizeof_element),
               (u8*)array->elements + ((index + 1) * array->sizeof_element),
               (array->count - index - 1) * array->sizeof_element);
    }
    if (array->count - 1 < array->capacity / 4 && array->capacity > 4) {
        dynarray_resize(array, array->capacity / 2);
    }
    array->count--;
}