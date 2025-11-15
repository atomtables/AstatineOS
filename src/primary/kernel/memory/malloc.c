#include "memory.h"

// lowk got this from GPT because this is temporary and
// I need to just have something that can allocate areas for pages to exist.

typedef struct block_header {
    u32 size;
    struct block_header* next;
} block_header;

static u8* heap_start;
static u32 heap_size;

static block_header* free_list = null;

void kmalloc_init(void* start, u32 size) {
    heap_start = (u8*)start;
    heap_size  = size;

    free_list = (block_header*)start;
    free_list->size = size - sizeof(block_header);
    free_list->next = null;
}

void* kmalloc(u32 size) {
    block_header* prev = null;
    block_header* cur  = free_list;

    while (cur) {
        if (cur->size >= size) {
            // if splitting block
            if (cur->size > size + sizeof(block_header)) {
                u8* new_block_addr =
                    (u8*)cur + sizeof(block_header) + size;

                block_header* new_block = (block_header*)new_block_addr;
                new_block->size = cur->size - size - sizeof(block_header);
                new_block->next = cur->next;

                cur->size = size;
                cur->next = null;

                if (prev) prev->next = new_block;
                else      free_list = new_block;

            } else {
                // use the whole block
                if (prev) prev->next = cur->next;
                else      free_list = cur->next;
            }

            

            return (void*)cur + sizeof(block_header);
        }

        prev = cur;
        cur = cur->next;
    }

    return null; // out of memory
}

void kfree(void* ptr) {
    if (!ptr) return;

    block_header* block =
        (block_header*)((u8*)ptr - sizeof(block_header));

    block->next = free_list;
    free_list = block;
}


void* kcalloc(const int bytes) {
    void* ptr = kmalloc(bytes);
    memset(ptr, 0, bytes);
    return ptr;
}

void* krealloc(void* ptr, u32 bytes) {
    void* new_addr = kmalloc(bytes);

    memcpy(new_addr, ptr, bytes);
    kfree(ptr);

    return new_addr;
}
/// someone else do me a favor and implement a freemem function
