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
#include <modules/dynarray.h>

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

// Maintains a AstatineDriverFile* list of available drivers on
// the system: not every driver will have a use at
// the moment, but the actual driver is still
// registered so if such a device appears later
// the driver can be initialised for it.
// These drivers are NOT ABLE TO BE LOADED
// these just contain the values to check if the driver
// is compatible as well as a file path.
Dynarray* available_drivers;
Dynarray* active_drivers;
AstatineDriverFile** loaded_drivers;
u32 loaded_driver_count = 0;
static u32 loaded_driver_size = 0;


static int initialise_driver_with_subsystem(AstatineDriverFile* driver, Device* device) {
    // depending on the driver type, we can call the relevant
    // registration function
    switch (driver->device_type) {
        case DEVICE_TYPE_TTYPE:
            return register_teletype_driver(driver, device);
        default:
            printf("Driver type %u not supported for auto-registration.\n",
                    driver->driver_type);
            return -1;
    }
}
u32 postregister_driver(AstatineDriverFile* driver) {
    // for any given driver we will need to probe the system
    // in the case that it needs to detect some hardware that
    // can otherwise not be detected
    u32 detected_devices = 0;
    Device probe_device;
    if (driver->probe && driver->probe(&probe_device, get_kernel_function_pointers())) {
        printf("Driver %s probed and found its device.\n", driver->name);
        // We don't register here because either way the register_driver function
        // does that. It's somewhat inefficient but it means that 
        // =======
        // wait i just had a fucking brainfart of course the fucking registration
        // function can just take a device and driver if we're searching for the
        // fucking device anyway 
        // and when we search for the driver for a device WE STILL HAVE BOTH OF
        // THE FUCKING DEVICES TS CAN NOT BE REAL
        Device* device = register_device(device);
        initialise_driver_with_subsystem(driver, device);
        detected_devices++;
    }
    // During post-registration, we use the base to probe
    // all devices on the system that match the device type.
    for (int i = 0; i < device_count; i++) {
        Device* device = devices[i];
        if (device->type != driver->device_type || device->type != DEVICE_TYPE_UNKNOWN) {
            continue;
        }
        if (device->conn != driver->driver_type || device->conn != CONNECTION_TYPE_UNKNOWN) {
            continue;
        }
        // driver is responsible for owning device
        // unowning a device can be done by the driver
        // but will be done after calling deinit just in case
        if (device->owned) {
            continue;
        }

        if (driver->check && driver->check(device, get_kernel_function_pointers())) {
            // This driver base is able to manage this device
            // so we can create an instance of the driver
            // through the driver's manager registration. (like register_teletype_driver)
            if (initialise_driver_with_subsystem(driver, device) == 0) {
                detected_devices++;
            }
        }
    }

    return detected_devices;
}

static int temp;
int attempt_install_driver(File* file) {
    // Then we should read just the ELF header
    ELF_Header header;
    if (fat_file_read(file, &header, sizeof(ELF_Header), &temp) != 0) {
        printf("Failed to read the ELF header.\n");
        return 2;
    }

    // For position-independent drivers, we choose where to load them.
    // First, find the extent of the ELF (lowest and highest virtual addresses)
    u32 elf_low = 0xFFFFFFFF;
    u32 elf_high = 0;
    ELF_Program_Header ph_temp;
    for (u32 i = 0; i < header.program_entry_count; i++) {
        fat_file_seek(file, header.program_header_offset + i * sizeof(ELF_Program_Header), FAT_SEEK_START);
        if (fat_file_read(file, &ph_temp, sizeof(ELF_Program_Header), &temp) != 0) continue;
        if (ph_temp.type != ELF_PT_LOAD) continue;
        if (ph_temp.virtual_addr < elf_low) elf_low = ph_temp.virtual_addr;
        if (ph_temp.virtual_addr + ph_temp.size_mem > elf_high) 
            elf_high = ph_temp.virtual_addr + ph_temp.size_mem;
    }
    
    // get the size of our elf so we can use it for alignment
    u32 elf_size = (elf_high - elf_low + 0xFFF) & ~0xFFF;
    
    Dynarray* addrs = dynarray_create(sizeof(u32));
    // Determine if this is a position-independent driver or fixed-address
    // If elf_low is 0, it's position-independent and we choose the address
    // If elf_low is non-zero, the driver expects to load at that address
    i32 load_offset;
    if (elf_low == 0) {
        // Position-independent: we choose where to load
        u32 load_base = alloc_page(PAGEF_NOUSER);
        dynarray_add(addrs, &load_base);
        load_offset = (i32)load_base - (i32)elf_low;
    } else {
        // Fixed address: load at the address specified in the ELF
        load_offset = 0;
    }

    // Now we should go through the list of 
    // program headers
    // seek to the program header offset
    ELF_Program_Header current_header;
    ELF_Section_Header section_headers[header.section_entry_size * header.section_entry_count];
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
        // at the address WE choose (not what the ELF says) when 
        // load_offset != 0. If code is NOT pic then load_offset == 0.
        u32 actual_addr = current_header.virtual_addr + load_offset;
        bool first = true;
        for (u32 addr = actual_addr;
             addr < actual_addr + current_header.size_mem;
             addr += 0x1000) {
            // If this is not the first page or this is NOT relocatable,
            // we can allocate it normally.
            // This is because we allocate the first page up there.
            if (!first || load_offset == 0) {
                if (!alloc_page_at_addr(addr, PAGEF_NOUSER)) {
                    printf("Failed to allocate page for ELF segment at address %x.\n", addr);
                    goto cleanup;
                }
                dynarray_add(addrs, &addr);
                first = false;
            }
        }
        
        // Now we should read the segment data from the file at our chosen address
        fat_file_seek(file, current_header.offset_in_file, FAT_SEEK_START);
        if (fat_file_read(file, (void*)actual_addr, current_header.size_file, &temp) != 0) {
            printf("Failed to read segment data.\n");
            goto cleanup;
        }

        // zero out the rest of the memory (elf spec)
        memset((void*)(actual_addr + current_header.size_file),
                0,
                current_header.size_mem - current_header.size_file
        );
    }
    
    // We load the section headers now for relocations and .astatine_driver
    fat_file_seek(file, header.section_header_offset, FAT_SEEK_START);
    if (fat_file_read(file, section_headers, header.section_entry_size * header.section_entry_count, &temp) != 0) {
        printf("Failed to read section headers.\n");
        goto cleanup;
    }
    
    // Adjust section addresses by load_offset so relocations work correctly
    for (u32 i = 0; i < header.section_entry_count; i++) {
        if (section_headers[i].addr != 0) {
            section_headers[i].addr += load_offset;
        }
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
            Elf32_Rela relocs[section_headers[i].size];
            fat_file_seek(file, section_headers[i].offset, FAT_SEEK_START);
            if (fat_file_read(file, relocs, section_headers[i].size, &temp) != 0) {
                printf("Failed to read RELA section.\n");
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
            AstatineDriverFile* driver_base = (AstatineDriverFile*)section_headers[i].addr;
            if (!verify_driver(driver_base->verification)) {
                printf("Driver verification failed for %s v%s by %s\n",
                        driver_base->name,
                        driver_base->version,
                        driver_base->author);
                goto cleanup;
            }

            // After post-registration, we should know if this driver
            // is currently being used by devices.
            // We can store that information in the size section of the
            // driver base for later use.
            u32 devices_inited = postregister_driver(driver_base);
            if (devices_inited != 0) {
                loaded_driver_count++;
                if (loaded_driver_size == 0) {
                    loaded_driver_size = 4;
                    loaded_drivers = kmalloc(sizeof(AstatineDriver*) * loaded_driver_size);
                } else if (loaded_driver_count > loaded_driver_size) {
                    loaded_drivers = krealloc(loaded_drivers,
                                                sizeof(AstatineDriver*) * loaded_driver_size * 2);
                    loaded_driver_size *= 2;
                }
                loaded_drivers[loaded_driver_count - 1] = kmalloc(sizeof(AstatineDriverFile));
                memcpy(loaded_drivers[loaded_driver_count - 1], driver_base, sizeof(AstatineDriverFile));
            } else {
                // we can unload the driver since it's not being used
                for (u32 p = 0; p < addrs->count; p++) {
                    u32 addr = *(u32*)dynarray_get(addrs, p);
                    free_page(addr);
                }
            }

            // we store this in the available_drivers list
            if (!available_drivers) available_drivers = dynarray_create(sizeof(AstatineDriverFile));
            driver_base->sig[0] = 0x0; // mark as loaded
            char* path = kmalloc(strlen(driver_base->name) + 1);
            strcpy(path, driver_base->name);
            ((void**)driver_base->name)[0] = path;
            dynarray_add(available_drivers, driver_base);

            printf("Successfully registered driver: %s v%s by %s\n",
                    driver_base->name,
                    driver_base->version,
                    driver_base->author);
            goto safe;
        }
    }
    sleep(1000);
    cleanup:
    for (u32 p = 0; p < addrs->count; p++) {
        u32 addr = *(u32*)dynarray_get(addrs, p);
        free_page(addr);
    }
    safe:
    dynarray_destroy(addrs);
    if (symtab_cache) {
        for (u32 i = 0; i < header.section_entry_count; i++) {
            if (symtab_cache[i] != null) {
                kfree(symtab_cache[i]);
            }
        }
        kfree(symtab_cache);
    }
    return 0;
}
