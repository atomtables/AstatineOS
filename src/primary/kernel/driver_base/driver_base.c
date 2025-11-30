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
#include <basedevice/device.h>
#include <basedevice/discovery/discovery.h>
#include <basedevice/devicelogic.h>

int verify_driver(u8 items[128]) {
    if (((u64*)(items))[0] != (u64)0xDEADBEEFDEADBEEFl) {
        return 0;
    } else {
        return 1;
    }
}
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

AstatineDriverBase** available_drivers;
u32 available_driver_count = 0;
static u32 available_driver_size = 0;

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
    
    // We load the section headers now for relocations and .astatine_driver
    ELF_Section_Header* section_headers = kmalloc(header.section_entry_size * header.section_entry_count);
    fat_file_seek(file, header.section_header_offset, FAT_SEEK_START);
    if (fat_file_read(file, section_headers, header.section_entry_size * header.section_entry_count, &temp) != 0) {
        printf("Failed to read section headers.\n");
        goto cleanup;
    }

    // For relocatable objects, we need to load symbol table and relocation
    // sections from the file since they're not in program headers.
    // First pass: load SYMTAB sections and process relocations
    typedef struct {
        u32 st_name;
        u32 st_value;
        u32 st_size;
        u8  st_info;
        u8  st_other;
        u16 st_shndx;
    } Elf32_Sym;

    // Cache for loaded symbol tables (indexed by section index)
    Elf32_Sym** symtab_cache = kcalloc(sizeof(Elf32_Sym*) * header.section_entry_count);

    // Process relocations
    for (u32 i = 0; i < header.section_entry_count; i++) {
        if (section_headers[i].type == SHT_RELA) {
            typedef struct {
                u32 r_offset;
                u32 r_info;
                i32 r_addend;
            } Elf32_Rela;

            // Load relocation entries from file
            Elf32_Rela* relocs = kmalloc(section_headers[i].size);
            fat_file_seek(file, section_headers[i].offset, FAT_SEEK_START);
            if (fat_file_read(file, relocs, section_headers[i].size, &temp) != 0) {
                printf("Failed to read RELA section.\n");
                kfree(relocs);
                continue;
            }

            // Load symbol table if not cached
            u32 symtab_idx = section_headers[i].link;
            if (symtab_cache[symtab_idx] == null) {
                symtab_cache[symtab_idx] = kmalloc(section_headers[symtab_idx].size);
                fat_file_seek(file, section_headers[symtab_idx].offset, FAT_SEEK_START);
                if (fat_file_read(file, symtab_cache[symtab_idx], section_headers[symtab_idx].size, &temp) != 0) {
                    printf("Failed to read symbol table.\n");
                    kfree(relocs);
                    continue;
                }
            }
            Elf32_Sym* symtab = symtab_cache[symtab_idx];

            // Target section for relocations
            u32 target_idx = section_headers[i].info;
            u32 target_base = section_headers[target_idx].addr;

            u32 reloc_count = section_headers[i].size / sizeof(Elf32_Rela);
            for (u32 j = 0; j < reloc_count; j++) {
                u32 reloc_type = relocs[j].r_info & 0xFF;
                u32 sym_idx = relocs[j].r_info >> 8;
                i32 addend = relocs[j].r_addend;

                // Address to patch = target section base + offset within section
                u32* reloc_addr = (u32*)(target_base + relocs[j].r_offset);

                // For relocatable objects, symbol value is section-relative
                // We need to add the loaded address of the symbol's section
                u32 sym_value = symtab[sym_idx].st_value;
                u16 sym_shndx = symtab[sym_idx].st_shndx;
                if (sym_shndx != 0 && sym_shndx < header.section_entry_count) {
                    sym_value += section_headers[sym_shndx].addr;
                }

                switch (reloc_type) {
                    case 1: // R_386_32: S + A
                        *reloc_addr = sym_value + addend;
                        break;
                    case 2: // R_386_PC32: S + A - P
                        *reloc_addr = sym_value + addend - (u32)reloc_addr;
                        break;
                    default:
                        printf("Unknown RELA type: %u\n", reloc_type);
                        break;
                }
            }
            kfree(relocs);

        } else if (section_headers[i].type == SHT_REL) {
            typedef struct {
                u32 r_offset;
                u32 r_info;
            } Elf32_Rel;

            // Load relocation entries from file
            Elf32_Rel* relocs = kmalloc(section_headers[i].size);
            fat_file_seek(file, section_headers[i].offset, FAT_SEEK_START);
            if (fat_file_read(file, relocs, section_headers[i].size, &temp) != 0) {
                printf("Failed to read REL section.\n");
                kfree(relocs);
                continue;
            }

            // Load symbol table if not cached
            u32 symtab_idx = section_headers[i].link;
            if (symtab_cache[symtab_idx] == null) {
                symtab_cache[symtab_idx] = kmalloc(section_headers[symtab_idx].size);
                fat_file_seek(file, section_headers[symtab_idx].offset, FAT_SEEK_START);
                if (fat_file_read(file, symtab_cache[symtab_idx], section_headers[symtab_idx].size, &temp) != 0) {
                    printf("Failed to read symbol table.\n");
                    kfree(relocs);
                    continue;
                }
            }
            Elf32_Sym* symtab = symtab_cache[symtab_idx];

            // Target section for relocations
            u32 target_idx = section_headers[i].info;
            u32 target_base = section_headers[target_idx].addr;

            u32 reloc_count = section_headers[i].size / sizeof(Elf32_Rel);
            for (u32 j = 0; j < reloc_count; j++) {
                u32 reloc_type = relocs[j].r_info & 0xFF;
                u32 sym_idx = relocs[j].r_info >> 8;

                // Address to patch = target section base + offset within section
                u32* reloc_addr = (u32*)(target_base + relocs[j].r_offset);
                // For REL, addend is stored at the relocation site
                i32 addend = (i32)*reloc_addr;

                // For relocatable objects, symbol value is section-relative
                u32 sym_value = symtab[sym_idx].st_value;
                u16 sym_shndx = symtab[sym_idx].st_shndx;
                if (sym_shndx != 0 && sym_shndx < header.section_entry_count) {
                    sym_value += section_headers[sym_shndx].addr;
                }

                switch (reloc_type) {
                    case 1: // R_386_32: S + A
                        *reloc_addr = sym_value + addend;
                        break;
                    case 2: // R_386_PC32: S + A - P
                        *reloc_addr = sym_value + addend - (u32)reloc_addr;
                        break;
                    default:
                        printf("Unknown REL type: %u\n", reloc_type);
                        break;
                }
            }
            kfree(relocs);
        }
    }

    // Now find and process .astatine_driver section
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
            if (!verify_driver(driver_base->verification)) {
                printf("Driver verification failed for %s v%s by %s\n",
                        driver_base->name,
                        driver_base->version,
                        driver_base->author);
                goto cleanup;
            }
            available_driver_count++;
            if (available_driver_size == 0) {
                available_driver_size = 4;
                available_drivers = kmalloc(sizeof(AstatineDriver*) * available_driver_size);
            } else if (available_driver_count > available_driver_size) {
                available_drivers = krealloc(available_drivers,
                                            sizeof(AstatineDriver*) * available_driver_size * 2);
                available_driver_size *= 2;
            }
            
            available_drivers[available_driver_count - 1] = driver_base;

            int postregister_driver(AstatineDriverBase* driver);
            postregister_driver(driver_base);
            printf("Successfully registered driver: %s v%s by %s\n",
                    driver_base->name,
                    driver_base->version,
                    driver_base->author);
            goto safe;
        }
    }
    sleep(1000);
    cleanup:
    for (u32 i = 0; i < addrs_count; i++) {
        free_page(addrs[i]);
    }
    safe:
    if (symtab_cache) {
        for (u32 i = 0; i < header.section_entry_count; i++) {
            if (symtab_cache[i] != null) {
                kfree(symtab_cache[i]);
            }
        }
        kfree(symtab_cache);
    }
    if (addrs) kfree(addrs);
    if (section_headers) kfree(section_headers);
    return 0;
}

int postregister_driver(AstatineDriverBase* driver) {
    // for any given driver we will need to probe the system
    // in the case that it needs to detect some hardware that
    // can otherwise not be detected
    Device* device = kmalloc(sizeof(Device*));
    if (driver->probe && driver->probe(device, get_kernel_function_pointers())) {
        printf("Driver %s probed and found its device.\n", driver->name);
        // We don't register here because either way the register_driver function
        // does that. It's somewhat inefficient but it means that 
        // =======
        // wait i just had a fucking brainfart of course the fucking registration
        // function can just take a device and driver if we're searching for the
        // fucking device anyway 
        // and when we search for the driver for a device WE STILL HAVE BOTH OF
        // THE FUCKING DEVICES TS CAN NOT BE REAL
        Device* temp = register_device(device);
        device = temp;
        goto success;
    }
    // During post-registration, we use the base to probe
    // all devices on the system that match the device type.
    for (int i = 0; i < device_count; i++) {
        device = devices[i];
        if (device->type != driver->device_type) {
            continue;
        }
        if (device->conn != driver->driver_type) {
            continue;
        }
        if (device->owned) {
            continue;
        }

        if (driver->check && driver->check(device, get_kernel_function_pointers())) {
            // This driver base is able to manage this device
            // so we can create an instance of the driver
            // through the driver's manager registration. (like register_teletype_driver)
            goto success;
        }
    }

    char* x = "No device iunfortunately.";
    for (u32 i = 0; x[i] != 0x00; i++) {
        *((u8*)0xb8000 + i * 2) = x[i];
    }

    kfree(device);
    return -1;

    success:
    if (driver->device_type == DEVICE_TYPE_TTYPE && register_teletype_driver(driver, device) >= 0) {
        char* x = "Success install.";
        for (u32 i = 0; x[i] != 0x00; i++) {
            *((u8*)0xb8000 + i * 2) = x[i];
        }
        printf("Successfully initialised teletype driver for device %s (ID %x)\n",
                device->name,
                device->id);
        kfree(device);
        return 0;
    } else {
        char* x = "Driver type not supported for auto-registration.";
        for (u32 i = 0; x[i] != 0x00; i++) {
            *((u8*)0xb8000 + i * 2) = x[i];
        }
        printf("Driver type %u not supported for auto-registration.\n",
                driver->driver_type);
        kfree(device);
        return -2;
    }
}
