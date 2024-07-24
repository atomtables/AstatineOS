//
// Created by Adithiya Venkatakrishnan on 18/07/2024.

#include "modules.h"

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

u8 find_char_for_int(u8 item) {
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

u8 find_char_for_hex(u8 item) {
    for (int i = 0; i < 10; i++) {
        if (char_to_hex_conversions[i][0] == item) {
            return char_to_hex_conversions[i][1];
        }
    }
    return null;
}

string itoa(u32 number, const string str) {
    u8 digits[10] = {0};
    int count = 9;

    while (number) {
        const int digit = number % 10;
        number = number / 10;
        digits[count] = digit;
        count--;
    }

    bool zero = false;
    int j = 0;
    for (int i = 0; i < 10; i++) {
        if (!zero) {
            if (digits[i] != 0) {
                zero = true;
                j = 0;
            }
            else continue;
        }
        str[j] = (i8)find_char_for_int(digits[i]);
        j++;
    }
    str[j] = 0;
    return str;
}

string xtoa(u32 number, const string str) {
    u8 digits[8] = {0};
    int count = 7;

    while (number) {
        const int digit = number % 16;
        number = number / 16;
        digits[count] = digit;
        count--;
    }

    bool zero = false;
    int j = 0;
    for (int i = 0; i < 8; i++) {
        if (!zero) {
            if (digits[i] != 0) {
                zero = true;
                j = 0;
            }
            else continue;
        }
        str[j] = (i8)find_char_for_hex(digits[i]);
        j++;
    }
    str[j] = 0;
    return str;
}

string xtoa_padded(u32 number, const string str) {
    u8 digits[8] = {0};
    int count = 7;

    while (number) {
        const int digit = number % 16;
        number = number / 16;
        digits[count] = digit;
        count--;
    }

    for (int i = 0; i < 8; i++) {
        str[i] = (i8)find_char_for_hex(digits[i]);
    }
    return str;
}

u8 inportb(u16 port) {
    u8 r;
    __asm__ ("inb %1, %0" : "=a" (r) : "dN" (port));
    return r;
}

void outportb(u16 port, u8 data) {
    __asm__ ("outb %1, %0" : : "dN" (port), "a" (data));
}