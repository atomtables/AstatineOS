#include "memory.h"
#include "malloc.h"
#include "paging.h"
#include <modules/modules.h>
#include <display/simple/display.h>
#include <exception/exception.h>
#include <timer/PIT.h>

#include "modules/strings.h"

#define COM1_PORT 0x3F8
/* Serial Port Hardware Check Macros */
#define SERIAL_IS_TX_EMPTY(port) (inportb((port) + 5) & 0x20)
/* High-level Write Macros */
#define SERIAL_PUTC(port, c) do { \
while (!SERIAL_IS_TX_EMPTY(port)); \
outportb(port, c); \
} while(0)
#define SERIAL_PRINT(port, str) do { \
const char* __s = (str); \
while (*__s) { \
SERIAL_PUTC(port, *__s); \
__s++; \
} \
} while(0)

// Similar to how Linux does it,
// we can keep track of free memory pages
// based on memory maps.

// These are the datas provided by the BIOS
// Notable exclusion is the 0xA0000-0xFFFFF region
// which is usually reserved for MMIO like VGA, ACPI, etc.
// Only bytes marked specifically as usable RAM should be used.
typedef struct SMAP_entry {
	u32 BaseL; // base address uint64_t
	u32 BaseH;
	u32 LengthL; // length uint64_t
	u32 LengthH;
	u32 Type; // entry Type
	u32 ACPI; // extended
}__attribute__((packed)) SMAP_entry;

SMAP_entry* smap;
u32 smap_count;
//
// // simple bump allocator for frames
// u32 next_frame_addr = 0;
//
// // TODO: implement a buddy allocator for page frames
// // we need to implement a buddy allocator because the current
// // bump allocator might be bottom 10. as programs demand more
// // memory it becomes important to make sure the foundation is
// // rock solid.
// // the buddy allocator runs on a linked list type
// struct buddy_item {
//     struct buddy_item* prev;
//     struct buddy_item* next;
//     // we need to store the base address because it's probably easier
//     // than taking the sum of all addresses
//     void* base;
//     // Keeping in mind that 32-bit kernels can address a max of 4GB mem,
//     // and a memory frame is 4KB=0x1000 wide is about 0x100000 or 1048576 = 2^20
//     // we can make a max order of 20, (live math) which realistically means that
//     // the order only has to be 5 bits on a 32-bit system.
//     // i was wrong gemini said it can go up to 32, 5 bits is only 31, and the
//     // compiler's gonna require alignment anyway we aren't saving 3 bytes for anything
//     // useful
//     u8 order;
//     // one bit for allocation
//     bool allocated;
// } PACKED;
//
// struct buddy_item frame_allocator = {0};
//
// u32 look_for_usable_frame(u32 after) {
//     for (u32 i = 0; i < smap_count; i++) {
//         SMAP_entry* entry = &smap[i];
//         if (entry->Type == 1) { // usable RAM
//             u64 base = ((u64)entry->BaseH << 32) | entry->BaseL;
//             u64 length = ((u64)entry->LengthH << 32) | entry->LengthL;
//
//             // align base to next 4KB boundary
//             u64 aligned_base = (base + 0xFFF) & ~0xFFF;
//             // align length to 4KB boundary
//             u64 aligned_length = length - (aligned_base - base);
//             aligned_length &= ~0xFFF;
//
//             for (u64 addr = aligned_base; addr < aligned_base + aligned_length; addr += 0x1000) {
//                 if (addr >= after) {
//                     return addr;
//                 }
//             }
//         }
//     }
//     return 0; // out of memory
// }
//
// // gemini's code was wrong so i had to make my own
// // extremely inefficient one that has to multiply
// // and loop and do all this crap
// // but its better than a function that doesn't actually work
// u64 highestPowerOfTwoLessThanEqualTo64Bit(u64 n) {
//     if (n == 1) return 1;
//
//     u64 computation = 1;
//     u64 order = 0;
//     while (computation <= n) {
//         computation *= 2;
//         order++;
//     }
//     return 1ULL << (order - 1);
// }
// struct order_item_32 {u32 order, comp;};
// // struct order_item_32 lowestPowerOfTwoGreaterThanEqualTo32Bit(u32 n) {
// //     if (n == 1) return (struct order_item_32){.order = 0, .comp = 1};
// //
// //     u64 computation = 1;
// //     u64 order = 0;
// //     while (computation <= n) {
// //         computation *= 2;
// //         order++;
// //     }
// //     return (struct order_item_32){.order = (u32)(order - 1), .comp = (u32)(1ULL << (order - 1))};
// // }
// // gemini told me my code was wrong and to use this instead
// struct order_item_32 lowestPowerOfTwoGreaterThanEqualTo32Bit(u32 n) {
//     if (n <= 1) return (struct order_item_32){.order = 0, .comp = 1};
//     // Subtracting 1 handles exact powers of 2 correctly
//     u32 order = 32 - __builtin_clz(n - 1);
//     return (struct order_item_32){.order = order, .comp = 1U << order};
// }
//
// // u64 highestPowerOfTwoLessThan64Bit(u64 n) {
// //     if (n <= 1) return 0;
// //
// //     n--;
// //
// //     n |= n >> 1;
// //     n |= n >> 2;
// //     n |= n >> 4;
// //     n |= n >> 8;
// //     n |= n >> 16;
// //     n |= n >> 32;
// //
// //     return (n + 1) >> 1;
// // }
//
// // in general you want to avoid this function because
// // allocating a specific physical address is usually unnecessary
// // but we have this just in case
// bool alloc_frames_with_base(const void* base, u32 length) {
//     // this algorithm should do the following
//     // 1. find the order of the block that needs to be generated
//     // 2. look for the base and see how it needs to be split
//     // 3. make the block of that order
//     struct order_item_32 necessary_order = lowestPowerOfTwoGreaterThanEqualTo32Bit(length);
//     u32 target_order = necessary_order.order;
//     u32 block_size = (1 << target_order);
//     if (((u32)base & (block_size - 1)) != 0)
//         return false;
//     // let's see the current order of the node that has the base
//     // we'll have to traverse the linked list to find this
//     // keep in mind that the exact address we want will have to be between
//     // the base of the node and the base+length of the node
//     struct buddy_item* prev = NULL;
//     struct buddy_item* item = &frame_allocator;
//     bool found = false;
//
//     while (item != NULL) {
//         // Calculate block size using bit-shifting instead of pow()
//         u32 block_size = (1 << item->order);
//
//         // Check if the requested 'base' falls inside this block'4
//         if (!item->allocated && (u32)base >=(u32)item->base && (u32)base < (u32)item->base + block_size) {
//             found = true;
//             break;
//         }
//         char str[64] = "Checked block: base=";
//         xtoa_padded((u32)item->base, str + strlen(str));
//         *(str + strlen(str)) = ' ';
//         xtoa_padded(block_size, str + strlen(str));
//         *(str + strlen(str)) = '\r';
//         *(str + strlen(str)) = '\n';
//         *(str + strlen(str)) = '\0';
//         SERIAL_PRINT(COM1_PORT, str);
//         prev = item;
//         item = item->next;
//     }
//     if (!found) {
//         SERIAL_PRINT(COM1_PORT, "wraps");
//         return false;
//     }
//     // now we have to strategically split the base
//     // we can have a couple of different methods to implement this
//     // but i'm just gonna get GPT to write this part of the code for me
//
//     void* current_base = item->base;
//     u32 current_order = item->order;
//
//     char str[63] = "Targeting order: ";
//     itoa((u32)target_order, str + strlen(str));
//     *(str + strlen(str)) = ' ';
//     xtoa_padded((u32)length, str + strlen(str));
//     *(str + strlen(str)) = '\n';
//     *(str + strlen(str)) = '\0';
//     SERIAL_PRINT(COM1_PORT, str);
//
//     // so we just keep recursively splitting the section that we need
//     // until we get one that fits our order
//     while (current_order > target_order) {
//         current_order--;
//
//         // Create a buddy block for the second half of the split
//         struct buddy_item* buddy = kmalloc(sizeof(struct buddy_item));
//         buddy->base = (void*)((u32)current_base + (1 << current_order));
//         buddy->order = current_order;
//         buddy->allocated = 0;
//         // and this is the next block for our current item
//         buddy->prev = item;
//         buddy->next = item->next;
//         if (buddy->next) buddy->next->prev = buddy;
//         // now we modify item
//         item->order = current_order;
//         item->next = buddy;
//         item->allocated = 0;
//
//         // now if our current address is in between the base of our first
//         // block and the base of our second block, we use the first block as our next target
//         // otherwise we use the second block. inclusive against the first base, exclusive
//         // against the second one.
//         if ((u32)base < (u32)buddy->base) {
//             // i guess bro
//             item = item;
//             current_base = item->base;
//         } else {
//             item = buddy;
//             current_base = buddy->base;
//         }
//     }
//
//     // at this point item now contains a node that is exactly
//     // what we asked for.
//     // BABY THIS IS WHAT YOU CAME FOR
//     // LATELY SOMETHING SOMETHING RHYMING
//     // OOOOOHOOHOHOHOHOHOOOOOOOOH
//     item->allocated = 1;
//
//     struct buddy_item* current = &frame_allocator;
//     while (current != null) {
//         char item[63] = "Found buddy block: ";
//         xtoa_padded((u32)current->base, item + strlen(item));
//         *(item + strlen(item)) = ' ';
//         *(item + strlen(item)) = '\0';
//         itoa(current->order, item + strlen(item));
//         *(item + strlen(item)) = current->allocated ? 'T' : 'F';
//         *(item + strlen(item)) = '\n';
//         *(item + strlen(item)) = 0;
//
//         SERIAL_PRINT(COM1_PORT, item);
//         current = current->next;
//     }
//
//     // At this point, the block at 'current_base' is exactly target_order,
//     // contains 'base', and is already out of the free list.
//     return true;
// }
//
// void init_frame_alloc() {
//     // let's look for the first memory address after 0x400000 that's ok to use
//     next_frame_addr = 0x400000;//look_for_usable_frame(0x400000);
//     if (next_frame_addr == 0) {
//         // while(1);
//         panic("Out of memory while allocating frame");
//     }
//
//     struct buddy_item *current = 0;
//     // we need nodes for our buddy allocator to function
//     // and we know what memory is safe based on the
//     for (u32 i = 0; i < smap_count; i++) {
//         SMAP_entry *entry = &smap[i];
//         if (entry->Type == 1) {
//             // usable RAM
//             u64 base = ((u64) entry->BaseH << 32) | entry->BaseL;
//             u64 length = ((u64) entry->LengthH << 32) | entry->LengthL;
//
//             // align base to next 4KB boundary
//             u64 aligned_base = (base + 0xFFF) & ~0xFFF;
//             // align length to 4KB boundary
//             u64 aligned_length = length - (aligned_base - base);
//             aligned_length &= ~0xFFF;
//
//             // we need to exclude the region from 0x0 to 0x400000 since
//             // we use this region of memory as the central kernel
//             // allocation address
//             if (aligned_base + aligned_length <= 0x400000) {
//                 // this entire block is unusable
//                 continue;
//             }
//
//             // we need to trim the beginning of this block
//             u64 trim_amount = 0x400000 - aligned_base;
//             aligned_base += trim_amount;
//             aligned_length -= trim_amount;
//
//             // we have the base and length, so we can now allocate as much
//             // memory as we can into a single order block
//             while (aligned_length != 0) {
//                 u64 max_by_len = highestPowerOfTwoLessThanEqualTo64Bit(aligned_length);
//                 u64 max_by_align = (aligned_base == 0) ? max_by_len : (1ULL << __builtin_ctzll(aligned_base));
//                 u64 largest_length = (max_by_len < max_by_align) ? max_by_len : max_by_align;
//                 // we need to keep track of whether we initialised the frame_allocator
//                 // for the first time
//                 if (!current) {
//                     current = &frame_allocator;
//                     current->order = (u8) __builtin_ctzll(largest_length);
//                     current->allocated = 0;
//                     current->prev = 0;
//                     current->base = (void *) aligned_base;
//
//                     char item[63] = "Added first block: ";
//                     xtoa_padded((u32)aligned_base, item + strlen(item));
//                     *(item + strlen(item)) = ' ';
//                     *(item + strlen(item)) = '\0';
//                     xtoa_padded((u32)aligned_length, item + strlen(item));
//                     *(item + strlen(item)) = ' ';
//                     *(item + strlen(item)) = '\0';
//                     xtoa_padded((u32)largest_length, item + strlen(item));
//                     *(item + strlen(item)) = '\n';
//                     *(item + strlen(item)) = '\0';
//
//                     SERIAL_PRINT(COM1_PORT, item);
//                 }
//                 // if we initialised the current variable this is our next time
//                 // so we can run towards initialising the next variable
//                 // i'm not sure if at this point we even have malloc initialised?
//                 // yea never mind i read the code and malloc is a ticking time bomb
//                 // that initialises on usage so we're good.
//                 else {
//                     current->next = kmalloc(sizeof(struct buddy_item));
//                     current->next->prev = current;
//                     current = current->next;
//                     current->next = 0;
//                     current->allocated = 0;
//                     current->order = (u8) __builtin_ctzll(largest_length);
//                     current->base = (void *) aligned_base;
//
//                     char item[63] = "Added buddy block: ";
//                     xtoa_padded((u32)aligned_base, item + strlen(item));
//                     *(item + strlen(item)) = ' ';
//                     *(item + strlen(item)) = '\0';
//                     xtoa_padded((u32)aligned_length, item + strlen(item));
//                     *(item + strlen(item)) = ' ';
//                     *(item + strlen(item)) = '\0';
//                     xtoa_padded((u32)largest_length, item + strlen(item));
//                     *(item + strlen(item)) = '\n';
//                     *(item + strlen(item)) = '\0';
//
//                     SERIAL_PRINT(COM1_PORT, item);
//                 }
//                 aligned_length -= largest_length;
//                 aligned_base += largest_length;
//             }
//         }
//     }
//
//     current = &frame_allocator;
//     while (current != null) {
//         char item[63] = "Found buddy block: ";
//         xtoa_padded((u32)current->base, item + strlen(item));
//         *(item + strlen(item)) = ' ';
//         *(item + strlen(item)) = '\0';
//         itoa(current->order, item + strlen(item));
//         *(item + strlen(item)) = '\n';
//         *(item + strlen(item)) = '\0';
//
//         SERIAL_PRINT(COM1_PORT, item);
//         current = current->next;
//     }
//
//     // the physical frame allocator is now ready
//
//     // bool x = alloc_frames_with_base((void*)0, 0x400000);
//     // SERIAL_PUTC(COM1_PORT, x ? 'Y' : 'N');
// }
//
// // its time we make some more hardcore functions with a ton of
// // options, configurations, and etc.
// // u32 alloc_frames(
// //     u32 length,
// //     u32 before,
// //     u32 after
// // ) {
// //     if (before >= after) return null;
// //     // just make sure that if before and after are set, the conditions are met
// // #define IS_BEFORE(item) (before != null && before <= item)
// // #define IS_AFTER(item) (after != null && after >= item)
// //     // u32 frame = next_frame_addr;
// //     // next_frame_addr = look_for_usable_frame(next_frame_addr + 0x1000);
// //     // if (next_frame_addr == 0) {
// //     //     // while(1);
// //     //     panic("Out of memory while allocating frame");
// //     //     SERIAL_PRINT(COM1_PORT, "Out of memory while allocating frame");
// //     // }
// //     // return frame;
// //     // this algorithm should do the following
// //     // 1. find the order of the block that needs to be generated
// //     // 2. look for the base and see how it needs to be split
// //     // 3. make the block of that order
// //     struct order_item_32 necessary_order = lowestPowerOfTwoGreaterThanEqualTo32Bit(length);
// //     u32 target_order = necessary_order.order;
// //     u32 block_size = (1 << target_order);
// //     // let's see the current order of the node that has the base
// //     // we'll have to traverse the linked list to find this
// //     // keep in mind that the exact address we want will have to be between
// //     // the base of the node and the base+length of the node
// //     struct buddy_item* prev = NULL;
// //     struct buddy_item* item = &frame_allocator;
// //     bool found = false;
// //
// //     while (item != NULL) {
// //         // Calculate block size using bit-shifting instead of pow()
// //         u32 calcd_block_size = (1 << item->order);
// //
// //         // Make sure this fits the allocations the user has made explicit
// //         if (calcd_block_size >= block_size && !item->allocated && IS_BEFORE((u32)item->base) && IS_AFTER((u32)item->base)) {
// //             found = true;
// //             break;
// //         }
// //         char str[64] = "Checked block: base=";
// //         xtoa_padded((u32)item->base, str + strlen(str));
// //         *(str + strlen(str)) = ' ';
// //         xtoa_padded(block_size, str + strlen(str));
// //         *(str + strlen(str)) = '\r';
// //         *(str + strlen(str)) = '\n';
// //         *(str + strlen(str)) = '\0';
// //         SERIAL_PRINT(COM1_PORT, str);
// //         prev = item;
// //         item = item->next;
// //     }
// //     if (!found) {
// //         SERIAL_PRINT(COM1_PORT, "wraps");
// //         return false;
// //     }
// //     // now we have to strategically split the base
// //     // we can have a couple of different methods to implement this
// //     // but i'm just gonna get GPT to write this part of the code for me
// //
// //     void* current_base = item->base;
// //     u32 current_order = item->order;
// //
// //     char str[63] = "Targeting order: ";
// //     itoa((u32)target_order, str + strlen(str));
// //     *(str + strlen(str)) = ' ';
// //     xtoa_padded((u32)length, str + strlen(str));
// //     *(str + strlen(str)) = '\n';
// //     *(str + strlen(str)) = '\0';
// //     SERIAL_PRINT(COM1_PORT, str);
// //
// //     // so we just keep recursively splitting the section that we need
// //     // until we get one that fits our order
// //     while (current_order > target_order) {
// //         current_order--;
// //
// //         // Create a buddy block for the second half of the split
// //         struct buddy_item* buddy = kmalloc(sizeof(struct buddy_item));
// //         buddy->base = (void*)((u32)current_base + (1 << current_order));
// //         buddy->order = current_order;
// //         buddy->allocated = 0;
// //         // and this is the next block for our current item
// //         buddy->prev = item;
// //         buddy->next = item->next;
// //         if (buddy->next) buddy->next->prev = buddy;
// //         // now we modify item
// //         item->order = current_order;
// //         item->next = buddy;
// //         item->allocated = 0;
// //
// //         // determine which one has a closer order, and go with that one, so we don't have to fragment as much
// //         if (ABS(item->order - target_order) <= ABS(buddy->order - target_order)) {
// //             // i guess bro
// //             item = item;
// //             current_base = item->base;
// //         } else {
// //             item = buddy;
// //             current_base = buddy->base;
// //         }
// //     }
// //
// //     // at this point item now contains a node that is exactly
// //     // what we asked for.
// //     // BABY THIS IS WHAT YOU CAME FOR
// //     // LATELY SOMETHING SOMETHING RHYMING
// //     // OOOOOHOOHOHOHOHOHOOOOOOOOH
// //     item->allocated = 1;
// //
// //     struct buddy_item* current = &frame_allocator;
// //     while (current != null) {
// //         char item[63] = "Found buddy block: ";
// //         xtoa_padded((u32)current->base, item + strlen(item));
// //         *(item + strlen(item)) = ' ';
// //         *(item + strlen(item)) = '\0';
// //         itoa(current->order, item + strlen(item));
// //         *(item + strlen(item)) = current->allocated ? 'T' : 'F';
// //         *(item + strlen(item)) = '\n';
// //         *(item + strlen(item)) = 0;
// //
// //         SERIAL_PRINT(COM1_PORT, item);
// //         current = current->next;
// //     }
// //
// //     // At this point, the block at 'current_base' is exactly target_order,
// //     // contains 'base', and is already out of the free list.
// //     return true;
// // }
//
// // gemini revamped this function, i'm honestly not liking
// // this system for allocating memory pages at all anyways
// // so I might change it to just go for a global array type
// // thing
// // this does seem to work but id probably like something else anyway
// // since there's a lot of overhead with the buddy allocator system
// // it would make more sense to just write a aingle byte for each page
// // which would just be the flags for that page
// u32 alloc_frames(
//     u32 length,
//     u32 before,
//     u32 after
// ) {
//     // Sanity check: If bounds are inverted, fail immediately
//     if (before != 0 && after != 0 && before >= after) return 0;
//
//     // 1. Find the order of the block that needs to be generated
//     struct order_item_32 necessary_order = lowestPowerOfTwoGreaterThanEqualTo32Bit(length);
//     u32 target_order = necessary_order.order;
//     u32 block_size = (1 << target_order);
//
//     struct buddy_item* prev = NULL;
//     struct buddy_item* item = &frame_allocator;
//     bool found = false;
//     u32 aligned_start = 0;
//
//     // 2. Traverse the list to find a block large enough that can accommodate our constraints
//     while (item != NULL) {
//         if (!item->allocated && item->order >= target_order) {
//             u32 block_start = (u32)item->base;
//             u32 block_end = block_start + (1 << item->order);
//
//             // Determine the earliest address we can legally start at
//             u32 search_start = (before != 0 && before > block_start) ? before : block_start;
//
//             // Align search_start up to the target_order boundary
//             u32 alignment = (1 << target_order);
//             u32 candidate = (search_start + alignment - 1) & ~(alignment - 1);
//
//             // Verify if this naturally aligned candidate fits within this block and honors 'after'
//             if (candidate >= block_start &&
//                 (candidate + block_size) <= block_end &&
//                 (after == 0 || candidate <= after)) {
//
//                 aligned_start = candidate;
//                 found = true;
//                 break;
//             }
//         }
//
//         // Serial debug logging
//         char str[64] = "Checked block: base=";
//         xtoa_padded((u32)item->base, str + strlen(str));
//         strcat(str, " \r\n");
//         SERIAL_PRINT(COM1_PORT, str);
//
//         prev = item;
//         item = item->next;
//     }
//
//     if (!found) {
//         SERIAL_PRINT(COM1_PORT, "wraps");
//         return 0;
//     }
//
//     void* current_base = item->base;
//     u32 current_order = item->order;
//
//     char str[63] = "Targeting order: ";
//     itoa((u32)target_order, str + strlen(str));
//     strcat(str, " Base: ");
//     xtoa_padded(aligned_start, str + strlen(str));
//     strcat(str, "\n");
//     SERIAL_PRINT(COM1_PORT, str);
//
//     // 3. Strategically split the block down to the target order
//     while (current_order > target_order) {
//         current_order--;
//
//         // Create a buddy block for the second half of the split
//         struct buddy_item* buddy = kmalloc(sizeof(struct buddy_item));
//         u32 buddy_base_addr = (u32)current_base + (1 << current_order);
//
//         buddy->base = (void*)buddy_base_addr;
//         buddy->order = current_order;
//         buddy->allocated = 0;
//
//         // Insert buddy into the linked list right after 'item'
//         buddy->prev = item;
//         buddy->next = item->next;
//         if (item->next) item->next->prev = buddy;
//         item->next = buddy;
//
//         // Modify the current item to reflect the split
//         item->order = current_order;
//
//         // Deterministic routing: Choose the buddy half that contains our target address
//         if (aligned_start >= buddy_base_addr) {
//             // Target is in the right buddy block
//             item = buddy;
//             current_base = buddy->base;
//         } else {
//             // Target is in the left buddy block (item stays item)
//             current_base = item->base;
//         }
//     }
//
//     // BABY THIS IS WHAT YOU CAME FOR
//     // The node 'item' is now perfectly sized, aligned, and positioned!
//     item->allocated = 1;
//
//     // Post-allocation list dump for debugging
//     struct buddy_item* current = &frame_allocator;
//     while (current != NULL) {
//         char dump_str[63] = "Found buddy block: ";
//         xtoa_padded((u32)current->base, dump_str + strlen(dump_str));
//         strcat(dump_str, " Order: ");
//         itoa(current->order, dump_str + strlen(dump_str));
//         strcat(dump_str, current->allocated ? " [T]\n" : " [F]\n");
//
//         SERIAL_PRINT(COM1_PORT, dump_str);
//         current = current->next;
//     }
//
//     // Return the actual address of the allocated frame
//     return (u32)item->base;
// }
//
// u32 alloc_frame() {
//     return alloc_frames(0x1000, 0, 0);
// }
//
// __attribute__((noinline, optimize("O0")))
// void DEBUG_BREAKPOINT() {
//     NOP();
// }
//
// char    mainstr[63] = "Freeing block: ";
// void free_frames(
//     u32 base,
//     u32 length
// ) {
//     // let's find the node
//     struct buddy_item* item = &frame_allocator;
//
//     while (item != NULL) {
//         u32 block_start = (u32)item->base;
//         u32 block_end = block_start + (1 << item->order);
//
//         if (base >= block_start && (base + length) <= block_end) {
//             // we found the node that contains the base
//             break;
//         }
//         // prev = item;
//         item = item->next;
//     }
//     if (item == NULL) {
//         // we didn't find the node, this is a problem
//         panic("Attempted to free a frame that was not allocated");
//     }
//
//     // we should know the length of the frames we allocated
//     if (item->allocated && (1 << item->order) == length) {
//         item->allocated = 0;
//         // now start merging with buddies
//         bool buddy_was_allocated = false;
//         char    str[127] = "Freeing block: ";
//         do {
//             SERIAL_PUTC(COM1_PORT, (u32)item->base == 0x415000 ? 'Y' : 'N');
//             if ((u32)item->base == (u32)0x415000) {
//                 SERIAL_PRINT(COM1_PORT, "h2llo world");
//                 DEBUG_BREAKPOINT();
//             }
//             // for the active node, let's look for its buddy, and see if it's not allocated
//             // we can check which side the buddy is on by seeing if the node itself
//             // aligns on a boundary 1 power above its current boundary
//             // if it does, the buddy is to the right, otherwise its to the left
//
//             // write debug statements to get the addresses of
//             // item, item->prev, item -> next
//             strcpy(str, mainstr);
//             xtoa_padded((u32)item->base, str + strlen(str));
//
//             strcat(str, " Prev: ");
//             if ((u32)item->base == (u32)0x415000) {
//                 SERIAL_PRINT(COM1_PORT, "item->prev:");
//                 SERIAL_PUTC(COM1_PORT, item->prev ? 'Y' : 'N');
//                 while (1);
//             }
//             if (item->prev) {
//                 xtoa_padded((u32)item->prev, str + strlen(str));
//                 SERIAL_PRINT(COM1_PORT, str);
//                 xtoa_padded((u32)item->prev->base, str + strlen(str) - 8);
//             } else {
//                 strcat(str, "NULL");
//             }
//
//             if ((u32)item->base == (u32)0x415000) {
//                 while (1);
//             }
//
//             strcat(str, " Next: ");
//             if (item->next) {
//                 xtoa_padded((u32)item->next->base, str + strlen(str));
//             } else {
//                 strcat(str, "NULL");
//             }
//             strcat(str, "\n");
//             SERIAL_PRINT(COM1_PORT, str);
//
//             if (item->order >= 31) {
//                 buddy_was_allocated = true;
//                 break;
//             }
//             if ((u32)item->base % (1U << (item->order+1)) == 0) {
//                 // to the right
//                 u32 expected_buddy_base = (u32)item->base + (1 << item->order);
//                 if (item->next && !item->next->allocated && item->next->order == item->order && (u32)item->next->base == expected_buddy_base) {
//                     // merge with buddy
//                     struct buddy_item* buddy = item->next;
//                     item->order++;
//                     item->next = buddy->next;
//                     if (item->next) item->next->prev = item;
//                     kfree(buddy);
//                 } else {
//                     buddy_was_allocated = true;
//                 }
//             } else {
//                 // to the left
//                 u32 expected_buddy_base = (u32)item->base - (1 << item->order);
//                 if (item->prev && !item->prev->allocated && item->prev->order == item->order && (u32)item->prev->base == expected_buddy_base) {
//                     // merge with buddy
//                     struct buddy_item* buddy = item->prev;
//                     buddy->order++;
//                     buddy->next = item->next;
//                     if (buddy->next) buddy->next->prev = buddy;
//                     kfree(item);
//                     item = buddy;
//                 } else {
//                     buddy_was_allocated = true;
//                 }
//             }
//         } while (!buddy_was_allocated);
//     } else {
//         panic("Attempted to free a frame with incorrect length or that was not allocated");
//     }
//     // congrats we just freed frames!!!
// }
//
// void free_frame(u32 base) {
//     free_frames(base, 0x1000);
// }

// screw everything up there this is the
// most stupid idea i ever had and it's just easier
// to write a 32-bit bitmap allocator

#define TOTAL_PAGES 1048576
u8 page_bitmap[TOTAL_PAGES / 8];
// let's rely on this value as a source of truth that
// is updated with every
u32 next_available_frame = 0x0;

inline void bitmap_set(u32 page_index) {
	page_bitmap[page_index / 8] |= (1 << (page_index % 8));
}

inline void bitmap_clear(u32 page_index) {
	page_bitmap[page_index / 8] &= ~(1 << (page_index % 8));
}

inline bool bitmap_test(u32 page_index) {
	return (page_bitmap[page_index / 8] & (1 << (page_index % 8))) != 0;
}

u32 alloc_frame() {
	if (next_available_frame) {
		u32 after_check = next_available_frame;
		if (!bitmap_test((next_available_frame + 0x1000) >> 12)) {
			// this should be clear
			next_available_frame += 0x1000;
		} else next_available_frame = 0x0;
		bitmap_set(after_check >> 12);
		return after_check;
	}
	// we can hardcode 0x400000 since that portion of memory is strictly only for kernel access
	// and identity-mapped from the get-go
	int i;
	for (i = 0x400000 >> 12; i < TOTAL_PAGES; i += 8) {
		if (page_bitmap[i / 8] != 0xff) break;
	}
	// will always land on a multiple of 8 so the only time
	// that the if condition is true is if the multiple of 8
	// is equal to total pages...
	u32 stop = i+8;
	if (stop > TOTAL_PAGES) return null;
	for (; i < stop; i++) {
		if (!bitmap_test(i)) {
			bitmap_set(i);
			if (!bitmap_test(i + 1)) next_available_frame = (i+1) << 12;
			else next_available_frame = 0x0;
			return i << 12;
		}
	}
	return null;
}

void free_frame(u32 address) {
	if (address & 0xfff) panic("attempted to free a frame not explicitly tied to a 4kb boundary");
	if (address < 0x400000) panic("attempted to free kernel frames");
	if (!bitmap_test(address >> 12)) panic("freed a physical frame that's not allocated");
	bitmap_clear(address >> 12);
	next_available_frame = address;
}

void init_mem() {
    // the BIOS should have dumped the memory map at 0x2000
    // so let's allocate memory so it's not going to get overwritten.
    smap_count = *(u32*)0x2000;
    SMAP_entry* smap_copy = kmalloc(sizeof(SMAP_entry) * smap_count);
    memcpy(smap_copy, (void*)0x2004, sizeof(SMAP_entry) * smap_count);
    smap = smap_copy;

    u32 total_mem = 0;
    for (u32 i = 0; i < smap_count; i++) {
        SMAP_entry* entry = &smap[i];
        if (entry->Type == 1) { // usable RAM
            u64 base = ((u64)entry->BaseH << 32) | entry->BaseL;
            u64 length = ((u64)entry->LengthH << 32) | entry->LengthL;
            total_mem += (u32)length;
            // printf("Usable RAM: Base 0x%x Length 0x%x\n", (u32)base, (u32)length);
        }
    }
    printf("Total Usable RAM: %d MiB\n", total_mem / 1024 / 1024);

    // set up paging first obv.
    paging_init();

    // we have free pages, but to allocate them, we need kmalloc
    // to have open space in the dymem region. This region is from
    // 0x100000 to 0x3FFFFF which is about 3MB. 
    // Using my current crappy implementation of kmalloc, it should be
    // autosetup.
    // kmalloc_init((void*)MEM_BLOCK_START, MEM_BLOCK_END - MEM_BLOCK_START);

    // Final thing we need is a frame address allocator
    // that can give out 4KB aligned physical addresses
    // for paging purposes.
    // We use the SMAP to avoid reserved regions.
	// holy crap i forgot to do the >> 12 im so stupid
	for (u32 i = 0; i < (0x400000>>12)/8; i++) {
		page_bitmap[i] = 0xFF;
	}
}
