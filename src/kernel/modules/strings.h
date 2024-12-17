//
// Created by Adithiya Venkatakrishnan on 16/12/2024.
//

#ifndef STRINGS_H
#define STRINGS_H

#include <modules/modules.h>

int     strlen      (const char* str);

void*   strcpy      (void* dst, const char* src);

int     strcmp      (u8* s1, u8* s2);
int     strncmp     (u8* s1, u8* s2, u32 t);

char*   strdup      (const char* str);

char*   strtok      (char* str, const char* sep);
char*   strtok_r    (char* s, const char* delim, char** last);
char**  strtok_a    (char* s, const char* delim);

#endif //STRINGS_H
