#ifndef MODULES_H
#define MODULES_H

// JDH
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef char      i8;
typedef short     i16;
typedef int       i32;
typedef long long i64;

typedef float  f32;
typedef double f64;
typedef u8     bool;
typedef char*  string;

#define null   0
#define true   1
#define false  0

#define alloca(b)    __builtin_alloca(b)
#define asm          __asm__ volatile
#define PACKED       __attribute__((packed))
#define CLI()        asm ("cli")
#define STI()        asm ("sti")
#define BREAKPOINT() asm ( "int3" );
#define FATALERROR() asm ( "ud2" );

#define DIV_BY_ZERO() \
    asm ( \
        "movl $1, %eax;" \
        "movl $0, %ebx;" \
        "div %ebx;"      \
    );

#define get_registers() \
    u32* registers[8];\
    __asm__ volatile ("movl %%eax, %0": "=r"(registers[0])); \
    __asm__ volatile ("movl %%ebx, %0": "=r"(registers[1])); \
    __asm__ volatile ("movl %%ecx, %0": "=r"(registers[2])); \
    __asm__ volatile ("movl %%edx, %0": "=r"(registers[3])); \
    __asm__ volatile ("movl %%esi, %0": "=r"(registers[4])); \
    __asm__ volatile ("movl %%edi, %0": "=r"(registers[5])); \
    __asm__ volatile ("movl %%ebp, %0": "=r"(registers[6])); \
    __asm__ volatile ("movl %%esp, %0": "=r"(registers[7]));


string  itoa        (u32 number, string str);
string  itoa_signed (i32 number, string str);
string  xtoa        (u32 number, string str);
string  xtoa_padded (u32 number, string str);

void    memset      (void* dst,  u8 value,  u32 n);
void    memset_step (void* dst,  u8 value,  u32 n, u32 step);
void*   memcpy      (void* dst,  void* src, u32 n);
void*   memmove     (void* dst,  void* src, u32 n);

u8      inportb     (u16 port);
void    outportb    (u16 port,   u8 data);

#endif
