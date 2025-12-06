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

// Maintains a AstatineDriverIndex* list of available drivers on
// the system: not every driver will have a use at
// the moment, but the actual driver is still
// registered so if such a device appears later
// the driver can be initialised for it.
// These drivers are NOT ABLE TO BE LOADED
// these just contain the values to check if the driver
// is compatible as well as a file path.
struct AstatineDriverIndex {
    // TODO
};
Dynarray* available_drivers;
// List of active drivers on the system
// AstatineDriver*
Dynarray* active_drivers;
// List of loaded drivers in memory
// AstatineDriverFile*
Dynarray* loaded_drivers;


static int initialise_driver_with_subsystem(AstatineDriverFile* driver, Device* device) {
    // depending on the driver type, we can call the relevant
    // registration function
    switch (device->type) {
        case DEVICE_TYPE_TTYPE:
            int ret = register_teletype_driver(driver, device);
            return ret;
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
    for (int i = 0; i < devices->count; i++) {
        
        Device* device = dynarray_get(devices, i);
        if (device->type != driver->device_type && device->type != DEVICE_TYPE_UNKNOWN) {
            continue;
        }
        if (device->conn != driver->driver_type && device->conn != CONNECTION_TYPE_UNKNOWN) {
            continue;
        }
        // driver is responsible for owning device
        // unowning a device can be done by the driver
        // but will be done after calling deinit just in case
        if (device->owned) {
            continue;
        }

        if (driver->check != 0 && driver->check(device, get_kernel_function_pointers())) {
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
    int errno = 0;
    // Then we should read just the ELF header
    ELF_Header header;
    if (fat_file_read(file, &header, sizeof(ELF_Header), &temp) != 0) {
        printf("Failed to read the ELF header.\n");
        errno = 2;
        goto cleanup;
    }

    struct page_keep_track {
        u32 page_addr;
        u32 size;
    } pkt;
    // ts is the array rip
    Dynarray* pages_for_each_segment = dynarray_create(sizeof(struct page_keep_track));
    Dynarray* program_headers = dynarray_create(sizeof(ELF_Program_Header));
    Dynarray* section_headers = dynarray_create(sizeof(ELF_Section_Header));
    if (!available_drivers) available_drivers = dynarray_create(sizeof(AstatineDriverFile*));
    if (!active_drivers) active_drivers = dynarray_create(sizeof(AstatineDriverFile*));
    char* string_table = 0;

    if (!load_program_headers_elf(file, program_headers)) {
        printf("Failed to load ELF program headers.\n");
        errno = 3;
        goto cleanup;
    }
    if (!load_section_headers_elf(file, section_headers)) {
        printf("Failed to load ELF section headers.\n");
        errno = 3;
        goto cleanup;
    }

    // Drivers must be an executable compiled with -pie (so their address base is zero)
    // ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), static-pie linked, not stripped
    // they must also be static because drivers cannot have dynamic dependencies (well they will later)
    if (header.type != 2) {
        printf("ELF file is not a relocatable executable.\n");
        errno = 4;
        goto cleanup;
    }

    // TODO: do a quick check that this is static-PIE
    // by checking the program headers for dynamic linking info

    // This is PIE code so we can kinda just load it into memory
    // First, allocate the pages necessary
    // Zeroth, find the size of the load section
    u32 current_delta = 0;
    u32 base_virtual_page = 0;
    bool base_delta_ready = false;
    ELF_Program_Header* ph;
    ELF_Dynamic_Entry* dyn_location = null;
    u32 dyn_count = 0;
    for (u32 i = 0; i < program_headers->count; i++) {
        ph = dynarray_get(program_headers, i);
        // looking for loadable headers
        if (ph->type == ELF_PT_LOAD) {
            u32 segment_virtual_page = ph->virtual_addr & ~0xFFF;
            u32 page_offset = ph->virtual_addr & 0xFFF;
            u32 bytes_span = ph->size_mem + page_offset;
            u32 pages_needed = (bytes_span + 0xFFF) >> 12;
            u32 segment_page = 0;

            if (!base_delta_ready) {
                segment_page = alloc_page_range(pages_needed, PAGEF_NOUSER);
                if (!segment_page) {
                    printf("Failed to allocate pages for driver segment.\n");
                    errno = 5;
                    goto cleanup;
                }
                base_virtual_page = segment_virtual_page;
                current_delta = segment_page - base_virtual_page;
                base_delta_ready = true;
            } else {
                segment_page = current_delta + segment_virtual_page;
                if (!alloc_page_range_at_addr(segment_page, pages_needed, PAGEF_NOUSER)) {
                    printf("Failed to allocate pages for driver segment at required address.\n");
                    errno = 5;
                    goto cleanup;
                }
            }

            pkt.page_addr = segment_page;
            pkt.size = pages_needed * 4096;
            dynarray_add(pages_for_each_segment, &pkt);

            u32 load_address = current_delta + ph->virtual_addr;
            fat_file_seek(file, ph->offset_in_file, FAT_SEEK_START);
            if (fat_file_read(file, (void*)load_address, ph->size_file, &temp) != 0) {
                printf("Failed to read driver segment from file.\n");
                errno = 6;
                goto cleanup;
            }

            // Zero out the rest of the heap as per the spec
            for (u32 b = ph->size_file; b < ph->size_mem; b++) {
                *((u8*)(load_address + b)) = 0;
            }
        } else if (ph->type == ELF_PT_DYNAMIC) {
            // If we see anything that requires dynamic linking then dip harder than WITF
            // To load this we have to look at the offset, subtract by delta, and
            // parse it from there. This should point to .dynamic
            dyn_location = (ELF_Dynamic_Entry*)(ph->virtual_addr + current_delta);
            dyn_count = ph->size_file / sizeof(ELF_Dynamic_Entry);
        }
    }

    if (!dyn_location) {
        printf("Driver ELF has no dynamic section; cannot proceed.\n");
        errno = 7;
        goto cleanup;
    }
    
    ELF_Dynamic_Entry de;
    struct {
        void* location;
        u32 size, entsize;
    } rela = {0}, rel = {0};
    for (u32 de_idx = 0; de_idx < dyn_count; de_idx++) {
        de = dyn_location[de_idx];
        if (de.tag == DT_NULL) {
            break;
        }
        switch (de.tag) {
        case DT_RELA:
            rela.location = (void*)(de.val + current_delta);
        break;
        case DT_RELASZ:
            rela.size = de.val;
        break;
        case DT_RELAENT:
            rela.entsize = de.val;
        break;
        case DT_REL:
            rel.location = (void*)(de.val + current_delta);
        break;
        case DT_RELSZ:
            rel.size = de.val;
        break;
        case DT_RELENT:
            rel.entsize = de.val;
        break;
        case DT_NEEDED:
        case DT_SONAME:
            // oh no we have to link??? not allowed
            // we don't support any dynamic linking in drivers yet
            printf("Driver uses dynamic linking which is unsupported.\n");
            errno = 7;
            goto cleanup;
        break;
        default:
            continue;
        }
    }

    // We don't NEED rel & rela but we need at least one of them 
    // to perform relocations (if they aren't there then it prob
    // means that this wasn't built to be relocatable)
    if (!rel.location && !rela.location) {
        printf("Driver has no relocation information; cannot proceed.\n");
        errno = 8;
        goto cleanup;
    }
    if (rel.entsize != sizeof(ELF_Relocation_Entry) &&
        rela.entsize != sizeof(ELF_Relocation_Entry_Addend)) {
        printf("Driver relocation entry size mismatch; cannot proceed.\n");
        errno = 9;
        goto cleanup;
    }
    
    // iterate over each relocation
    ELF_Relocation_Entry e;
    if (rel.location && rel.entsize != 0) {
        u32 rel_count = rel.size / rel.entsize;
        for (u32 i = 0; i < rel_count; i++) {
            e = ((ELF_Relocation_Entry*)rel.location)[i];
            u32* reloc_addr = (u32*)(e.offset + current_delta);
            u32 type = e.info & 0xFF;
            u32 sym_index = e.info >> 8;
            switch (type) {
                case R_386_NONE:
                    // do nothing
                break;
                case R_386_32:
                    *reloc_addr += current_delta;
                break;
                case R_386_PC32:
                    *reloc_addr += current_delta - (u32)reloc_addr;
                break;
                case R_386_RELATIVE:
                    *reloc_addr = current_delta + (*reloc_addr);
                break;
                default:
                    errno = 10;
                    goto cleanup;
                break;
            }
        }
    }
    ELF_Relocation_Entry_Addend ea;
    if (rela.location && rela.entsize != 0) {
        u32 rela_count = rela.size / rela.entsize;
        for (u32 i = 0; i < rela_count; i++) {
            ea = ((ELF_Relocation_Entry_Addend*)rela.location)[i];
            u32* reloc_addr = (u32*)(ea.offset + current_delta);
            u32 type = ea.info & 0xFF;
            u32 sym_index = ea.info >> 8;
            switch (type) {
                case R_386_NONE:
                    // do nothing
                break;
                case R_386_32:
                    *reloc_addr += current_delta;
                break;
                case R_386_PC32:
                    *reloc_addr += current_delta - (u32)reloc_addr;
                break;
                case R_386_RELATIVE:
                    *reloc_addr = current_delta + (*reloc_addr);;
                break;
                default:
                    errno = 11;
                    goto cleanup;
                break;
            }
        }
    }
    // Relocation is done: let's get the string table first
    ELF_Section_Header* string_header = dynarray_get(section_headers,
        header.section_string_table_section_index);
    string_table = kmalloc(string_header->size);
    fat_file_seek(file, string_header->offset, FAT_SEEK_START);
    if (fat_file_read(file, string_table, string_header->size, &temp) != 0) {
        printf("Failed to read driver string table.\n");
        errno = 12;
        goto cleanup;
    }
    
    // look for the .astatine_driver section
    for (u32 i = 0; i < section_headers->count; i++) {
        ELF_Section_Header* sh = dynarray_get(section_headers, i);
        char* sec_name = string_table + sh->name;
        if (strcmp(sec_name, ".astatine_driver") == 0) {
            // found the driver section
            AstatineDriverFile* driver = (AstatineDriverFile*)(sh->addr + current_delta);
            // verify the driver
            if (!verify_driver(driver->verification)) {
                printf("Driver verification failed.\n");
                errno = 13;
                goto cleanup;
            }
            // register the driver
            u32 detected = postregister_driver(driver);
            printf("Driver %s installed successfully; detected %u devices.\n",
                    driver->name, detected);
            if (detected != 0) {
                goto success_with_devices;
            }
            break;
        }
    }

    cleanup:
    for (int i = 0; i < pages_for_each_segment->count; i++) {
        struct page_keep_track* pkt = dynarray_get(pages_for_each_segment, i);
            u32 pages_to_free = pkt->size / 4096;
            for (u32 j = 0; j < pages_to_free; j++) {
                free_page(pkt->page_addr + (j * 4096));
            }
    }
    success_with_devices:
    if (string_table) {
        kfree(string_table);
    }
    dynarray_destroy(pages_for_each_segment);
    dynarray_destroy(program_headers);
    dynarray_destroy(section_headers);
    return errno;
}
