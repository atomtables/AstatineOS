#include "malloc.h"
#include <display/simple/display.h>
#include <timer/PIT.h>
#include <exception/exception.h>

// this is why you never trust a clanker
// all those companies replacing their coders with
// AI are finished. done. trust me after this they'll
// give up on ai

#define size_t u32

struct mem_bounds {
    u32 start;
    u32 end;
    u8  memfree[32768];
} mem = {
    .start = MEM_BLOCK_START,
    .end = MEM_BLOCK_END,
    .memfree = {0}
};

struct metadata {
    void*   address;
    u32     size;
    u32     start_block;
} __attribute__((packed)) _items_arr[1];
struct metadata* items = null;
size_t items_size = 0;
size_t items_count = 0;

/**
 * funnily enough all the code below was an APCSA collegeboard problem lmao
 */
int laughable = 1;

static bool is_block_free(const int block) {
    // since the one bit is stored in a byte, we need to divide by 8
    const int byte = block / 8;
    const int bit = block % 8;
    return !BIT_GET(mem.memfree[byte], bit);
}

static void free_block(const int block) {
    const int byte = block / 8;
    const int bit = block % 8;
    mem.memfree[byte] = BIT_SET(mem.memfree[byte], bit, 0);
}

static void reserve_block(const int block) {
    const int byte = block / 8;
    const int bit = block % 8;
    mem.memfree[byte] = BIT_SET(mem.memfree[byte], bit, 1);
}

static void _alloc_items() {
    items = &_items_arr[0];
    items_size = 1;
    struct metadata* new_items = kmalloc(sizeof(struct metadata) * 1024);
    struct metadata last_item = items[0];
    items = new_items;
    items[0] = last_item;
    items_size = 1024;
    items_count = 1;
}

static void _realloc_items() {
    struct metadata last_item = items[items_count - 1];
    items_count -= 1;
    items_size += 1024;
    struct metadata* new_items = krealloc(items, sizeof(struct metadata) * items_size);
    items = new_items;
    items[items_count] = last_item;
    items_count += 1;
}

u32 total_blocks = (MEM_BLOCK_END - MEM_BLOCK_START) / MEM_BLOCK_BYTE_SIZE;

void* kmalloc_aligned(const size_t bytes, const size_t alignment) {
    u32 blocks;
    if (bytes % MEM_BLOCK_BYTE_SIZE == 0) {
        blocks = bytes / MEM_BLOCK_BYTE_SIZE;
    } else {
        blocks = bytes / MEM_BLOCK_BYTE_SIZE + 1;
    }

    // find the first block of memory that is free
    u32 block = 0, other_block = 0, length = 0;
    int start = -1;
    for (int i = 0; i < total_blocks; i++) {
        if (is_block_free(i)) {
            if (start == -1) start = i;
            if ((start * MEM_BLOCK_BYTE_SIZE) % (alignment) != 0) {
                start = -1;
                length = 0;
                continue;
            }
            if (++length == blocks) {
                for (int j = start; j < start + blocks; j++)
                    reserve_block(j);
                if (!items) {
                    _alloc_items();
                }
                if (items_count >= items_size) {
                    // resize
                    _realloc_items();
                }
                void* addr = (void*)(MEM_BLOCK_START + start * MEM_BLOCK_BYTE_SIZE);
                items[items_count].address = addr;
                items[items_count].size = bytes;
                items[items_count].start_block = start;
                items_count++;
                return addr;
            }
        } else {
            start = -1;
            length = 0;
        }
    }
    // at this point, we have no memory to allocate
    // TODO: handle heap full
    return null;
}

/// FUNCTION: malloc
/// very rudimentary/alpha, will be improved at some point in the future.
void* kmalloc(const size_t bytes) {
    return kmalloc_aligned(bytes, 1);
}

void* kcalloc(const size_t bytes) {
    void* ptr = kmalloc(bytes);
    memset(ptr, 0, bytes);
    return ptr;
}

void* krealloc(void* ptr, size_t bytes) {
    void* new_addr = kmalloc(bytes);

    memcpy(new_addr, ptr, bytes);
    kfree(ptr);

    return new_addr;
}

/// FUNCTION: free
void kfree(void* ptr) {
    int bytes = -1, start_block = -1;
    for (size_t i = 0; i < items_count; i++) {
        if (items[i].address == ptr) {
            bytes = items[i].size;
            start_block = items[i].start_block;
            // shift all items down by one
            for (size_t j = i; j < items_count - 1; j++) {
                items[j] = items[j + 1];
            }
            items_count--;
            break;
        }
    }

    if (bytes == -1) {
        // while(1);
        // invalid pointer
        panic("Invalid pointer passed to kfree");
        return;
    }

    int blocks;
    if (bytes % MEM_BLOCK_BYTE_SIZE == 0) {
        blocks = bytes / MEM_BLOCK_BYTE_SIZE;
    } else {
        blocks = bytes / MEM_BLOCK_BYTE_SIZE + 1;
    }
    for (int i = start_block; i < start_block + blocks; i++) {
        free_block(i);
    }
}

/// someone else do me a favor and implement a freemem function
