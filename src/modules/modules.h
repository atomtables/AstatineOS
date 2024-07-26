#ifndef MODULES_H
#define MODULES_H

// JDH
typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;

typedef char        i8;
typedef short       i16;
typedef int         i32;
typedef long long   i64;

typedef u32     size_t;
typedef u32     uintptr_t;

typedef float   f32;
typedef double  f64;
typedef u8      bool;
typedef char*   string;

#define null    0
#define true    1
#define false   0

#define alloca(b)   __builtin_alloca(b)

string itoa(u32 number, char* str);

string itoa_signed(i32 number, char* str);

string xtoa(u32 number, char* str);

string xtoa_padded(u32 number, char* str);

u8 inportb(u16 port);

void outportb(u16 port, u8 data);

#endif
