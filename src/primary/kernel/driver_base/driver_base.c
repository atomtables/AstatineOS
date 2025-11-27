#include "driver_base.h"
#include <modules/modules.h>
#include "teletype/teletype.h"
#include <fat32/fat32.h>
#include <memory/malloc.h>
#include <interrupt/PIC.h>
#include <memory/paging.h>
#include <display/simple/display.h>
#include <elfloader/elfloader.h>
#include <modules/strings.h>
#include <timer/PIT.h>

int verify_driver(u8 items[128]) {
    if (((u64*)(items))[0] != (u64)0xDEADBEEFDEADBEEFl) {
        return 0;
    } else {
        return 1;
    }
}

int(* drivers[])(AstatineDriverBase*) = {
    null,
    null,
    install_teletype_driver
};

struct KernelFunctionPointers* get_kernel_function_pointers() {
    static struct KernelFunctionPointers kfp;
    kfp.kmalloc = kmalloc;
    kfp.kmalloc_aligned = kmalloc_aligned;
    kfp.kfree = kfree;
    kfp.krealloc = krealloc;
    kfp.PIC_install = PIC_install;
    kfp.allow_null_page_read = allow_null_page_read;
    kfp.disallow_null_page = disallow_null_page;
    return &kfp;
}

static int temp;
int attempt_install_driver(File* file) {
    // Then we should read just the ELF header
    ELF_Header header;
    if (fat_file_read(file, &header, sizeof(ELF_Header), &temp) != 0) {
        printf("Failed to read the ELF header.\n");
        return 2;
    }

    // Now we should go through the list of 
    // program headers
    // seek to the program header offset
    ELF_Program_Header current_header;
    u32* addrs = kmalloc(sizeof(u32) * header.program_entry_count);
    u32 addrs_size = header.program_entry_count;
    u32 addrs_count = 0;
    // now for each file, read it in
    for (u32 i = 0; i < header.program_entry_count; i++) {
        // Sanity checks
        if (header.program_header_offset + i * sizeof(ELF_Program_Header) >= file->size) {
            printf("ELF program header offset out of bounds.\n");
            goto cleanup;
        }

        // seek to the program header
        fat_file_seek(file, header.program_header_offset + i * sizeof(ELF_Program_Header), FAT_SEEK_START);
        if (fat_file_read(file, &current_header, sizeof(ELF_Program_Header), &temp) != 0) {
            printf("Failed to read the ELF headers.\n");
            goto cleanup;
        }

        // Check if we can load this segment
        if (current_header.type != ELF_PT_LOAD) continue;

        if (current_header.size_mem < current_header.size_file) {
            printf("ELF segment size in memory is smaller than size in file.\n");
            goto cleanup;
        }

        // we should allocate virtual memory for this segment
        // at the address they want us to.
        for (u32 addr = current_header.virtual_addr;
             addr < current_header.virtual_addr + current_header.size_mem;
             addr += 0x1000) {
            addrs[addrs_count++] = addr;
            if (addrs_count > addrs_size) {
                addrs = krealloc(addrs, sizeof(u32) * addrs_size * 2);
                addrs_size *= 2;
            }
            if (!alloc_page(addr)) {
                printf("Failed to allocate page for ELF segment at address %x.\n", addr);
                goto cleanup;
            }
        }
        // Now we should read the segment data from the file
        fat_file_seek(file, current_header.offset_in_file, FAT_SEEK_START);
        if (fat_file_read(file, (void*)current_header.virtual_addr, current_header.size_file, &temp) != 0) {
            printf("Failed to read segment data.\n");
            goto cleanup;
        }

        // zero out the rest of the memory (elf spec)
        memset((void*)(current_header.virtual_addr + current_header.size_file),
                0,
                current_header.size_mem - current_header.size_file
        );
    }
    // now let's find the .astatine_driver section header
    u32 section_header_offset = header.section_header_offset;
    ELF_Section_Header* section_headers = kmalloc(header.section_entry_size * header.section_entry_count);
    fat_file_seek(file, section_header_offset, FAT_SEEK_START);
    if (fat_file_read(file, section_headers, header.section_entry_size * header.section_entry_count, &temp) != 0) {
        printf("Failed to read section headers.\n");
        goto cleanup;
    }
    for (u32 i = 0; i < header.section_entry_count; i++) {
        // seek to the section name
        char name_buf[32];
        fat_file_seek(file,
                      (int)(section_headers[header.section_string_table_section_index].offset + section_headers[i].name),
                      FAT_SEEK_START); 
        if (fat_file_read(file, name_buf, 31, &temp) != 0) {
            printf("Failed to read section name.\n");
            goto cleanup;
        }
        name_buf[31] = 0; // null terminate
        if (strcmp(name_buf, ".astatine_driver") == 0) {
            // ok so this is a driver
            // let's check which type
            // driver should be included in the data segment already
            AstatineDriverBase* driver_base = (AstatineDriverBase*)section_headers[i].addr;
            if (drivers[driver_base->driver_type] == null) {
                printf("This is an unsupported driver. Not loading...\n");
                goto cleanup;
            }
            int result = drivers[driver_base->driver_type](driver_base);
            if (result != 0) {
                printf("Failed to install driver. Error code: %d\n", result);
                goto cleanup;
            } else {
                printf("%p %p", active_teletype_driver, active_teletype_driver->clear_screen);
                printf("Successfully installed driver: %s v%s by %s\n",
                       driver_base->name,
                       driver_base->version,
                       driver_base->author);
            }
            goto safe;
        }
    }
    sleep(1000);
    cleanup:
    for (u32 i = 0; i < addrs_count; i++) {
        free_page(addrs[i]);
    }
    safe:
    if (addrs) kfree(addrs);
    if (section_headers) kfree(section_headers);
    return 0;
}