#ifndef ELFLOADER_H
#define ELFLOADER_H

#include <modules/modules.h>
#include <modules/dynarray.h>
#include <fat32/fat32.h>
#include "elfloader_structs.h"

u32* elf_load(char* file_path, u32* out_count, u32* entrypoint, u32* stack_top_out);
void elf_free(u32* addrs, u32 count);

int is_elf(char* file_path);
bool load_program_headers_elf(File* file, struct dynarray* addrs);
bool load_section_headers_elf(File* file, struct dynarray* addrs);
int elf_load_and_run(char* file_path);

#endif