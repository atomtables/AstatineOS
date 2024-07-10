#ifndef modules_h
#define modules_h

// JDH
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;
typedef u32 size_t;
typedef u32 uintptr_t;
typedef float f32;
typedef double f64;
typedef u8 (bool);
#define true (1)
#define false (0)
#define null (0)

#define string char*

const u8 char_to_int_conversions[10][2] = {
    {0, '0'},
    {1, '1'},
    {2, '2'},
    {3, '3'},
    {4, '4'},
    {5, '5'},
    {6, '6'},
    {7, '7'},
    {8, '8'},
    {9, '9'},
};

static inline u8 find_char_for_int(u8 item) {
    for (int i = 0; i < 10; i++) {
        if (char_to_int_conversions[i][0] == item) {
            return char_to_int_conversions[i][1];
        }
    }
    return null;
}

const u8 char_to_hex_conversions[16][2] = {
    {0, '0'},
    {1, '1'},
    {2, '2'},
    {3, '3'},
    {4, '4'},
    {5, '5'},
    {6, '6'},
    {7, '7'},
    {8, '8'},
    {9, '9'},
    {10, 'A'},
    {11, 'B'},
    {12, 'C'},
    {13, 'D'},
    {14, 'E'},
    {15, 'F'},
};

static inline u8 find_char_for_hex(u8 item) {
    for (int i = 0; i < 10; i++) {
        if (char_to_hex_conversions[i][0] == item) {
            return char_to_hex_conversions[i][1];
        }
    }
    return null;
}

static inline u8 inportb(u16 port) {
    u8 r;
    __asm__ ("inb %1, %0" : "=a" (r) : "dN" (port));
    return r;
}

static inline void outportb(u16 port, u8 data) {
    __asm__ ("outb %1, %0" : : "dN" (port), "a" (data));
}

#endif