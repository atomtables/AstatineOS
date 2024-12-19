#ifndef MODULES_H
#define MODULES_H

// JDH
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;

typedef float f32;
typedef double f64;
typedef u8 bool;

#define null   0
#define true   1
#define false  0

#define alloca(b)       __builtin_alloca(b)
#define asm             __asm__ volatile
#define PACKED          __attribute__((packed))

#define CLI()           asm ( "cli" )
#define STI()           asm ( "sti" )

#define BREAKPOINT()    asm ( "int3" )
#define FATALERROR()    asm ( "ud2" )

#define NOP()           asm ( "nop" )

#define _assert_0()         __error_illegal_macro__
#define _assert_1(_e)       do { if (!(_e)) panic(NULL); } while (0)
#define _assert_2(_e, _m)   do { if (!(_e)) panic((_m)); } while (0)

#define _assert(x, _e, _m, _f, ...) _f

#define assert(...) \
    _assert(,##__VA_ARGS__,\
    _assert_2(__VA_ARGS__),\
    _assert_1(__VA_ARGS__),\
    _assert_0(__VA_ARGS__))

#define __MIN_IMPL(_x, _y, _xn, _yn) \
    __extension__({\
        __typeof__(_x) _xn = (_x);\
        __typeof__(_y) _yn = (_y);\
        (_xn < _yn ? _xn : _yn);\
    })
#define MIN(_x, _y) \
    __MIN_IMPL(_x, _y, CONCAT(__x, __COUNTER__), CONCAT(__y, __COUNTER__))

#define __MAX_IMPL(_x, _y, _xn, _yn) \
    __extension__({\
        __typeof__(_x) _xn = (_x);\
        __typeof__(_y) _yn = (_y);\
        (_xn > _yn ? _xn : _yn);\
    })
#define MAX(_x, _y) \
    __MAX_IMPL(_x, _y, CONCAT(__x, __COUNTER__), CONCAT(__y, __COUNTER__))

#define CLAMP(_x, _mi, _ma) \
    (MAX(_mi, MIN(_x, _ma)))

// returns the highest set bit of x
// i.e. if x == 0xF, HIBIT(x) == 3 (4th index)
// WARNING: currently only works for up to 32-bit types
#define HIBIT(_x) (31 - __builtin_clz((_x)))

// returns the lowest set bit of x
#define LOBIT(_x)\
__extension__({ __typeof__(_x) __x = (_x); HIBIT(__x & -__x); })

// returns _v with _n-th bit = _x
#define BIT_SET(_v, _n, _x) \
    __extension__({ \
        __typeof__(_v) __v = (_v); \
        (__v ^ ((-(_x) ^ __v) & (1 << (_n)))); \
    })

#define BIT_GET(_v, _n) \
    __extension__({ \
        __typeof__(_v) __v = (_v); \
        ((__v >> (_n)) & 1); \
    })


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

#define max(a, b) \
    ({ __typeof__ (a) _a = (a); \
        __typeof__ (b) _b = (b); \
        _a > _b ? _a : _b; })

#define min(a, b) \
    __extension__({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a < _b ? _a : _b; })

char* itoa(u32 number, char* str);
char* itoa_signed(i32 number, char* str);
char* xtoa(u32 number, char* str);
char* xtoa_padded(u32 number, char* str);

void memset(void* dst, u8 value, u32 n);
void memset_step(void* dst, u8 value, u32 n, u32 step);
void* memcpy(void* dst, void* src, u32 n);
void* memmove(void* dst, void* src, u32 n);

u8 inportb(u16 port);
void outportb(u16 port, u8 data);

u16 inportw(u16 port);
void outportw(u16 port, u16 data);

u32 rand();
void seed(u32 s);

#endif
