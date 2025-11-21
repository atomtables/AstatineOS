//
// Created by atomchairs on 31/8/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include "default.h"
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

FILE*   outputFile;
char*   outputName;
char*   inputDirName;
DIR*    inputDir;

#define u8  uint8_t
#define u32 uint32_t
#define ptr uint32_t

typedef struct FILE_ENTRY {
    u8   flags; // 0x00 (0)
    char name[8]; // 0x01-0x09 (1-9)
    char extension[3]; // 0x0A-0x0C (10-12)
    ptr  start_addr; // 0x0D-0x10 (13-16)
    u32  size; // 0x11-0x14 (17-20)
} __attribute__((packed)) FILE_ENTRY; // size 20 bytes

u8*     output;
u32     outputSize; 

FILE_ENTRY* listed;
char**      listedPaths;
u32     listedSize;
u32     listedCount;

// no support for nested dirs rn
int main(const int argc, char** argv) {
    if (argc == 2 && argv != NULL) {
        if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
            printf("mkabp version 0.1.0\n");
            return 0;
        } else {
            printf("Usage: mkabp <input directory> [output file]\n");
            return 1;
        }
    }
    // we need the folder where all the files will be stored, so that should be the first positional argument
    if (argc > 1) inputDirName = argv[1];
    else {
        fprintf(stderr, "Please include a directory to include in the boot partition. %s", argv[1]);
        return 1;
    }
    struct stat st;
    if (stat(inputDirName, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Please include an actual directory to include in the boot partition. Not one that doesn't exist.");
        return 1;
    }
    if (argc > 2) outputName = argv[2];
    else outputName = "./abp.bin";

    // allocate the buffer
    output = (u8*)malloc(512);
    outputSize = 512;
    listed = (FILE_ENTRY*)malloc(sizeof(FILE_ENTRY) * 10);
    listedPaths = (char**)malloc(sizeof(char*) * 10);
    listedSize = 10;
    listedCount = 0;

    // now we put our vbr in the first sector.
    memcpy(output, default_bin, default_bin_len);

    // we don't know how many files we are going to have
    // or the offset of the physical address of each file on disk
    // so we need to count amount of files, and make incomplete file entries
    // for their things then do calculations once we finish iterating the first time.
    inputDir = opendir(inputDirName);
    if (inputDir == NULL) {
        perror("Please include a valid directory, opendir");
        return 1;
    }
    struct dirent* entry;
    while ((entry = readdir(inputDir)) != NULL) {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct full path for the entry
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", inputDirName, entry->d_name);

        // Check if it's a regular file or a directory
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISREG(st.st_mode)) {
                FILE* fp = fopen(full_path, "r");
                fseek(fp, 0L, SEEK_END);
                size_t size = ftell(fp);
                // now update the file entru
                FILE_ENTRY* current = &listed[listedCount];
                listedPaths[listedCount] = strdup(full_path);
                current->flags = 0;
                char* token = strtok(entry->d_name, "/");
                char* temp;
                while (temp != NULL) if ((temp = strtok(NULL, "/"))) token = temp;
                token = strtok(token, ".");
                while (token != NULL) {
                    if (!current->name[0]) {
                        strncpy(current->name, token, 8);
                    } else {
                        strncpy(current->extension, token, 3);
                    }
                    token = strtok(NULL, ".");
                }
                current->size = size;
                // now update size of listed
                listedCount++;
                if (listedCount > listedSize) {
                    listedSize += 10;
                    listed = realloc(listed, listedSize * sizeof(FILE_ENTRY));
                    listedPaths = realloc(listedPaths, listedSize * sizeof(char*));
                }
            }
        } else {
            perror("stat");
        }
    }
    closedir(inputDir);

    *(uint16_t*)(output+16) = (uint16_t)listedCount;
    printf("%d\n%d\n", listedCount, *(output+16));

    // let's copy over the file list
    u32 sectorsFileList = ((sizeof(FILE_ENTRY) * listedCount) / 512) + 1;
    outputSize += sectorsFileList*512;
    output = (u8*)realloc(output, outputSize);
    memcpy(output+512, listed, listedCount * sizeof(FILE_ENTRY));

    int i;
    char* path;
    FILE_ENTRY* fileEntry;
    for (
        i = 0,
        path = listedPaths[i],
        fileEntry = &listed[i];
        i < listedCount;
        i++,
        path = listedPaths[i],
        fileEntry = &listed[i]
    ) {
        FILE* file = fopen(path, "r");
        u8* buf = malloc(fileEntry->size);
        fread(buf, sizeof(u8), fileEntry->size, file);
        // buf should have the contents, which we can now dump into the output.
        u32 fileSectors = (fileEntry->size / 512) + 1;
        int oldOutputSize = outputSize;
        printf("old output size: %d, file sectors: %d, size: %d", oldOutputSize, fileSectors, fileEntry->size);
        outputSize += fileSectors*512;
        output = (u8*)realloc(output, outputSize);
        memcpy(output+oldOutputSize, buf, fileEntry->size); 
        // each file should fill up a sector at least
        // may not be efficient on space but there shouldn't be that many files here to begin with.
        fileEntry->start_addr = oldOutputSize;
        memcpy(output+512, listed, listedCount * sizeof(FILE_ENTRY));
    }

    *(uint32_t*)(output+12) = (uint32_t)(outputSize/512);
    
    outputFile = fopen(outputName, "w+");
    fwrite(output, sizeof(u8), outputSize, outputFile);

    return 0;
}