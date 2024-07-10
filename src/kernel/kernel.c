#include "../modules.h"

/* the screen is 80x24 in text mode */
#define VGA_TEXT_WIDTH 80
#define VGA_TEXT_HEIGHT 24

struct {
	int x;
	int y;
} display_data;

void reset_displaydata() {
    display_data.x = 0; display_data.y = 0;
}

void write_char(int x, int y, char character, char color) {
	if (x >= VGA_TEXT_WIDTH) {
		return;
	}
	
    volatile char* vga = (char*) 0xb8000;
    vga += ((y * VGA_TEXT_WIDTH * 2) + (x * 2));
	display_data.x = x;
	display_data.y = y;
    *vga = character;
	*(vga+1) = color;
}

void clear_screen() {
    for (int i = 0; i <= VGA_TEXT_WIDTH; i++)
        for (int j = 0; j <= VGA_TEXT_HEIGHT; j++)
            write_char(i, j, '\0', 0x0f);
    display_data.x = 0;
    display_data.y = 0;
}

void append_char(char character, char color) {
    write_char(display_data.x, display_data.y, character, color);
	display_data.x++;
	if (display_data.x >= VGA_TEXT_WIDTH) {
		display_data.y++;
		display_data.x = 0;
	}
}

void write_number_to_text_memory(u32 number) {
    u8 digits[10] = {0};
    int count = 9;

    while (number) {
        int digit = number % 10;
        number = number / 10;
        digits[count] = digit;
        count--;
    }

    for (int i = 0; i < 10; i++) {
        u8 value = find_char_for_int(digits[i]);
//        *vga = value;
//        vga += 2;
        append_char(value, 0x0f);
    }
}

void write_hex_to_text_memory(u32 number) {
    u8 digits[8] = {0};
    int count = 9;

    while (number) {
        int digit = number % 16;
        number = number / 16;
        digits[count] = digit;
        count--;
    }

    for (int i = 0; i < 10; i++) {
        u8 value = find_char_for_hex(digits[i]);
//        *vga = value;
//        vga += 2;
        append_char(value, 0x0f);
    }
}

void append_string(string str) {
    for (int i = 0; str[i] != '\0'; i++) {
        append_char(str[i], 0x0f);
    }
}

void append_newline() {
    display_data.y++;
	if (display_data.y > VGA_TEXT_HEIGHT) {
		display_data.y = 0;
	}
	display_data.x = 0;
}

u16 get_cursor_position() {
    u16 pos = 0;
    outportb(0x3D4, 0x0F);
    pos |= inportb(0x3D5);
    outportb(0x3D4, 0x0E);
    pos |= ((u16) inportb(0x3D5)) << 8;
    return pos;
}

void update_cursor(int x, int y) {
	u16 pos = y * VGA_TEXT_WIDTH + x;

	outportb(0x3D4, 0x0F);
	outportb(0x3D5, (u8) (pos & 0xFF));
	outportb(0x3D4, 0x0E);
	outportb(0x3D5, (u8) ((pos >> 8) & 0xFF));
}

// YO THIS GUY ONLINE WAS ACT LEGIT :skull:
int main() {
    clear_screen();

    const string hi = "Welcome back to protected mode.";

    int count = 0;


    u16 curpos;

    while (true) {
        curpos = get_cursor_position();

        append_string("Raw value: ");
        write_number_to_text_memory(curpos);

        append_newline();

        append_string("x value: ");
        write_number_to_text_memory(curpos % VGA_TEXT_WIDTH);

        append_newline();

        append_string("y value: ");
        write_number_to_text_memory(curpos / VGA_TEXT_WIDTH);

        update_cursor(count, 0);
        append_newline();

        for (int i = 0; i < 1000000000; i++) {
            __asm__ ("nop");
        }
        count ++;

        reset_displaydata();
    }


    while(1);
}