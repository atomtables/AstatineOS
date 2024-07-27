//
// Created by Adithiya Venkatakrishnan on 18/07/2024.
//

#include <memory/memory.h>
#include <modules/modules.h>

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

u8 find_char_for_hex(const u8 item) {
    for (int i = 0; i < 16; i++) {
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

string itoa_signed(i32 number, const string str) {
    // took this code from geeks4geeks cuz i didnt feel like writing it myself
    int i = 0;
    // Save the sign of the number
    const int sign = number;

    // If the number is negative, make it positive
    if (number < 0) {
        number = -number;
    }

    // Extract digits from the number and add them to the
    // string
    do {
        // Convert integer digit to character
        str[i++] = number % 10 + '0';
    } while ((number /= 10) > 0);

    // If the number was negative, add a minus sign to the
    // string
    if (sign < 0) {
        str[i++] = '-';
    }

    // Null-terminate the string
    str[i] = '\0';

    // Reverse the string to get the correct order
    for (int j = 0, k = i - 1; j < k; j++, k--) {
        const char temp = str[j];
        str[j] = str[k];
        str[k] = temp;
    }
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
    str[8] = 0;
    return str;
}

void memset(void* dst, u8 value, u32 n) {
    u8 *d = dst;

    while (n-- > 0) {
        *d++ = value;
    }
}

void* memcpy(void* dst, const void* src, u32 n) {
    u8 *d = dst;
    const u8 *s = src;

    while (n-- > 0) {
        *d++ = *s++;
    }

    return d;
}

void* memmove(void* dst, const void* src, u32 n) {
    // OK since we know that memcpy copies forwards
    if (dst < src) {
        return memcpy(dst, src, n);
    }

    u8 *d = dst;
    const u8 *s = src;

    for (size_t i = n; i > 0; i--) {
        d[i - 1] = s[i - 1];
    }

    return dst;
}

u8 inportb(u16 port) {
    u8 r;
    asm ("inb %1, %0" : "=a" (r) : "dN" (port));
    return r;
}

void outportb(u16 port, u8 data) {
    asm ("outb %1, %0" : : "dN" (port), "a" (data));
}