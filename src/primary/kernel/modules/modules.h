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

#define INT(_n)         asm ( "int %0" :: "i" (_n) )
#define wait_for(_cond) while (!(_cond)) { NOP(); }
#define ret_if(_cond)   if (_cond) return

#define _assert_0()         __error_illegal_macro__
#define _assert_1(_e)       do { if (!(_e)) panic("assertion failed " __LINE__ __FILE__); } while (0)
#define _assert_2(_e, _m)   do { if (!(_e)) panic("assertion failed " __LINE__ __FILE__); } while (0)

#define _assert(x, _e, _m, _f, ...) _f

#define HLT()          asm ( "hlt" )

#define assert(...) \
    _assert(,##__VA_ARGS__,\
    _assert_2(__VA_ARGS__),\
    _assert_1(__VA_ARGS__),\
    _assert_0(__VA_ARGS__))

#define MIN(a,b) \
    ({ __typeof__ (a) _a = (a); \
        __typeof__ (b) _b = (b); \
        _a < _b ? _a : _b; })

#define MAX(a,b) \
    ({ __typeof__ (a) _a = (a); \
        __typeof__ (b) _b = (b); \
        _a > _b ? _a : _b; })

#define ABS(a) \
    ({ __typeof__ (a) _a = (a); \
        _a < 0 ? -_a : _a; })

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

char* itoa(u32 number, char* str);
char* itoa_signed(i32 number, char* str);
char* xtoa(u32 number, char* str);
char* xtoa_padded(u32 number, char* str);

bool validate_number(const char* str);
u32 atoi(const char* str);

void memset(void* dst, u8 value, u32 n);
void memset_step(void* dst, u8 value, u32 n, u32 step);
void* memcpy(void* dst, const void* src, u32 n);
void* memmove(void* dst, void* src, u32 n);
int memcmp(const char* s1, const char* s2, u32 t);

u8 inportb(u16 port);
void outportb(u16 port, u8 data);
u16 inportw(u16 port);
void outportw(u16 port, u16 data);
u32 inportd(u16 port);
void outportd(u16 port, u32 data);

void outsw(u32 reg, u16 *buffer, i32 words);
void insw(u32 reg, u16* buffer, i32 words);
void insd(u32 reg, u32* buffer, i32 quads);

u32 rand();
void seed(u32 s);

char* num_to_bin(u8 num, char* str);

#endif
