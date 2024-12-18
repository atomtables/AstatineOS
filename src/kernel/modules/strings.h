//
// Created by Adithiya Venkatakrishnan on 16/12/2024.
//

#ifndef STRINGS_H
#define STRINGS_H

#include <modules/modules.h>

typedef struct StrtokA {
    char** ret;
    int count;
} StrtokA;

int     strlen      (const char* str);

void*   strcpy      (void* dst, const char* src);

int     strcmp      (char* s1, char* s2);
int     strncmp     (char* s1, char* s2, u32 t);

char*   strdup      (const char* str);

char*   strtok      (char* str, const char* sep);
char*   strtok_r    (char* s, const char* delim, char** last);
StrtokA strtok_a(char* s, const char* delim);

#endif //STRINGS_H
