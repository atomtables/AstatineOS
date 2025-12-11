//
// Created by Adithiya Venkatakrishnan on 14/1/2025.
//

#include "basicbasic.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define u32 uint32_t
#define u64 uint64_t
#define u16 uint16_t

#define VGA_TEXT_COLOR(fg, bg) (((bg) << 4) | (fg))
#define COLOR_BLACK         0x0
#define COLOR_BLUE          0x1
#define COLOR_GREEN         0x2
#define COLOR_CYAN          0x3
#define COLOR_RED           0x4
#define COLOR_MAGENTA       0x5
#define COLOR_BROWN         0x6
#define COLOR_LIGHT_GREY    0x7
#define COLOR_DARK_GREY     0x8
#define COLOR_LIGHT_BLUE    0x9
#define COLOR_LIGHT_GREEN   0xA
#define COLOR_LIGHT_CYAN    0xB
#define COLOR_LIGHT_RED     0xC
#define COLOR_LIGHT_MAGENTA 0xD
#define COLOR_YELLOW        0xE
#define COLOR_WHITE         0xF

int fd;
struct teletype_packet {
    uint8_t yes;
    uint32_t x;
    uint32_t y;
    char* buffer;
    uint32_t size;
    uint8_t with_color;
    uint8_t color;
    uint8_t move_cursor;
} packet;

void set_terminal_mode(uint8_t mode) {
    asm volatile (
        "int $0x30\n"
        :
        : "a"(3), "b"(0), "c"(mode)
    );
};

int xopen(char* identifier) {
    int fd;
    asm volatile (
        "int $0x30\n"
        : "=a"(fd)
        : "a"(4), "b"(identifier), "d"(0)
    );
    return fd;
}

void output(void) {
    // draw a white frame
    for (int x = 0; x < 80; x++) {
        // printf("\033[0;%dH\033[47m \033[0m", x+1);
        // printf("\033[24;%dH\033[47m \033[0m", x+1);
    }
}

void enable_vga_cursor() {

}

void disable_vga_cursor() {}

void set_vga_cursor(u32 x, u32 y) {
    packet.y = y;
    packet.x = x;
    packet.buffer = NULL;
    packet.size = 0;
    packet.with_color = 0;
    packet.color = 0x0f;
    packet.move_cursor = 1;
    write(fd, &packet, sizeof(struct teletype_packet));
}

static struct {
    char* input;
    u32 input_length;

    bool ready_for_input_init;
    bool ready_for_next_input;
    bool ready_for_processing;

    bool end;

    // render cache
    char buffer[22][79];
    char previous_buffer[22][79];
    bool row_dirty[22];
    bool input_dirty;
    bool outline_drawn;
    char last_input[65];
    u32 last_input_length;
    bool last_running;
} state;

static struct {
    u32 variables[26];

    bool running;

    struct program {
        u32 ln;
        char* line;
    }* program;
    u32 program_length;
    u32 program_maxlen;

    u32 current_line_number;
    bool goto_performed;

    bool awaiting_input;
} basic;
static void setup() {
    fd = xopen("teletype");

    state.ready_for_input_init = true;
    state.ready_for_next_input = false;
    state.ready_for_processing = false;

    for (int i = 0; i < 22; i++) {
        memset(state.buffer[i], ' ', 78);
        state.buffer[i][78] = '\0';
        memset(state.previous_buffer[i], 0, 79);
        state.row_dirty[i] = true;
    }
    state.input_dirty = true;
    state.outline_drawn = false;
    state.last_input[0] = '\0';
    state.last_input_length = 0;
    state.last_running = false;

    basic.program = malloc(sizeof(struct program) * 50);
    basic.program_maxlen = 50;
}

static void quit() {
    // disable_double_buffering();
    // clear_screen();
    // enable_vga_cursor();

    free(basic.program);
    state.end = true;
}

static void append_buffer(const char* line) {
    memmove(state.buffer, state.buffer + 1, sizeof(state.buffer[0]) * 21);
    for (int i = 0; i < 21; i++) {
        state.row_dirty[i] = true;
    }

    memset(state.buffer[21], ' ', 78);
    state.buffer[21][78] = '\0';

    size_t len = strnlen(line, 78);
    memcpy(state.buffer[21], line, len);
    state.row_dirty[21] = true;
}

#define VGA_TEXT_WIDTH 80
#define VGA_TEXT_HEIGHT 25

static void draw_char(u32 x, u32 y, char ch) {
    packet.y = y;
    packet.x = x;
    packet.buffer = &ch;
    packet.size = 1;
    packet.with_color = 0;
    packet.color = 0x0f;
    packet.move_cursor = 0;
    write(fd, &packet, sizeof(struct teletype_packet));
}

static void draw_char_with_color(u32 x, u32 y, char ch, uint8_t color) {
    packet.y = y;
    packet.x = x;
    packet.buffer = &ch;
    packet.size = 1;
    packet.with_color = 1;
    packet.color = color;
    packet.move_cursor = 0;
    write(fd, &packet, sizeof(struct teletype_packet));
}

static void draw_string(u32 x, u32 y, const char* str) {
    packet.y = y;
    packet.x = x;
    packet.buffer = (char*)str;
    packet.size = strlen(str);
    packet.with_color = 0;
    packet.color = 0x0f;
    packet.move_cursor = 0;
    write(fd, &packet, sizeof(struct teletype_packet));
}

static void draw_string_with_color(u32 x, u32 y, const char* str, uint8_t color) {
    packet.y = y;
    packet.x = x;
    packet.buffer = (char*)str;
    packet.size = strlen(str);
    packet.with_color = 1;
    packet.color = color;
    packet.move_cursor = 0;
    write(fd, &packet, sizeof(struct teletype_packet));
}

static void draw_outline() {
    if (state.outline_drawn) return;
    draw_string(
        0, 0,
        "\xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB");
    draw_string(0, VGA_TEXT_HEIGHT - 1,
                "\xC8\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC");
    for (u32 i = 1; i < VGA_TEXT_HEIGHT - 1; i++) {
        draw_char_with_color(0, i, '\xBA', VGA_TEXT_COLOR(COLOR_LIGHT_GREY, COLOR_BLACK));
        draw_char_with_color(VGA_TEXT_WIDTH - 1, i, '\xBA', VGA_TEXT_COLOR(COLOR_LIGHT_GREY, COLOR_BLACK));
    }
    draw_string_with_color(34, 0, " basicbasic ",
                           VGA_TEXT_COLOR(COLOR_BLACK, COLOR_LIGHT_GREY));
    state.outline_drawn = true;
}

static void render_history() {
    for (int row = 0; row < 22; row++) {
        if (!state.row_dirty[row] && memcmp(state.buffer[row], state.previous_buffer[row], 79) == 0) {
            continue;
        }
        draw_string(1, 1 + row, state.buffer[row]);
        memcpy(state.previous_buffer[row], state.buffer[row], 79);
        state.row_dirty[row] = false;
    }
}

static void render_command() {
    if (!state.input) {
        // No live input buffer yet; draw the prompt " ] " and clear the rest once.
        char line[79];
        memset(line, ' ', sizeof(line));
        line[78] = '\0';
        line[0] = ']';
        line[1] = ' ';
        draw_string(1, 23, line);
        state.input_dirty = false;
        state.last_input_length = 0;
        state.last_input[0] = '\0';
        state.last_running = basic.running;
        if (!basic.running) {
            enable_vga_cursor();
            set_vga_cursor(3, 23);
        }
        return;
    }

    bool running_changed = (basic.running != state.last_running);
    bool input_changed = state.input_dirty || running_changed || (state.input_length != state.last_input_length) || (strncmp(state.input, state.last_input, state.input_length) != 0);

    if (!basic.running && input_changed) {
        char line[79];
        memset(line, ' ', sizeof(line));
        line[78] = '\0';
        line[0] = ']';
        line[1] = ' ';
        memcpy(line + 2, state.input, state.input_length);
        draw_string(1, 23, line);
        strncpy(state.last_input, state.input, sizeof(state.last_input) - 1);
        state.last_input[sizeof(state.last_input) - 1] = '\0';
        state.last_input_length = state.input_length;
        state.input_dirty = false;
    } else if (basic.running && input_changed) {
        // Clear the command line when running
        char line[79];
        memset(line, ' ', sizeof(line));
        line[78] = '\0';
        draw_string(1, 23, line);
        state.last_input[0] = '\0';
        state.last_input_length = 0;
        state.input_dirty = false;
    }

    state.last_running = basic.running;

    if (!basic.running) {
        enable_vga_cursor();
        set_vga_cursor(state.input_length + 3, 23);
    }
}

static void render() {
    draw_outline();
    render_history();
    render_command();
}

// Simple splitter around strtok; returns token count up to max_tokens.
static int split_tokens(char* src, const char* delims, char** out, int max_tokens) {
    int count = 0;
    for (char* tok = strtok(src, delims); tok && count < max_tokens; tok = strtok(NULL, delims)) {
        out[count++] = tok;
    }
    return count;
}

char* remove_spaces(char* s) {
    char* r = s;
    char* d = s;
    do {
        while (*d == ' ') {
            ++d;
        }
    } while ((*s++ = *d++));
    return r;
}

static u32 eval(char* exp) {
    exp = remove_spaces(exp);
    char* op = strpbrk(exp, "+-*/");
    if (!op) return (u32)atoi(exp);

    char opch = *op;
    *op = '\0';
    char* rhs = op + 1;

    u32 num1 = (exp[0] >= 'a' && exp[0] <= 'z') ? basic.variables[exp[0] - 'a'] : (u32)atoi(exp);
    u32 num2 = (rhs[0] >= 'a' && rhs[0] <= 'z') ? basic.variables[rhs[0] - 'a'] : (u32)atoi(rhs);

    if (opch == '+') return num1 + num2;
    if (opch == '-') return num1 - num2;
    if (opch == '*') return num1 * num2;
    if (opch == '/') return num1 / num2;
    return (u32)-1;
}

static bool commit_instruction(char** tokens, int token_count, char* line) {
    line = remove_spaces(strdup(line));
    if (token_count < 1) {
        free(line);
        return false;
    }
    if (strcmp(tokens[0], "run") == 0) {
        if (basic.running) {
            // beep();
            append_buffer("?RUNNING ERROR");
        }
        if (basic.program_length == 0 || basic.current_line_number >= basic.program_length) {
            basic.running = false;
            basic.current_line_number = 0;
            append_buffer("?NO PROGRAM");
        } else {
            basic.running = true;
        }        
        free(line);
        return true;
    }
    if (strcmp(tokens[0], "end") == 0) {
        basic.running = false;
        free(line);
        return true;
    }
    if (strcmp(tokens[0], "goto") == 0) {
        if (token_count < 2) {
            append_buffer("?SYNTAX ERROR");
            free(line);
            return false;
        }
        u32 ln = (u32)atoi(tokens[1]);
        for (u32 i = 0; i < basic.program_length; i++) {
            if (basic.program[i].ln == ln) {
                basic.current_line_number = i;
                if (basic.running) basic.goto_performed = true;
                basic.running = true;
                free(line);
                return true;
            }
        }
        // beep();
        append_buffer("?LINE ERROR");
        free(line);
        return false;
    }
    if (strcmp(tokens[0], "print") == 0) {
        char* str = tokens[1];
        if (str[0] == '\"') {
            str++;
            str[strlen(str) - 1] = 0;
            append_buffer(str);
            free(line);
            return true;
        }
        if (str[0] >= 'a' && str[0] <= 'z' && str[1] == 0) {
            char* buf = itoa(basic.variables[str[0] - 'a'], malloc(11), 10);
            append_buffer(buf);
            free(buf);
            free(line);
            return true;
        }
        // beep();
        append_buffer("?SYNTAX ERROR");
        free(line);
        return false;
    }
    if (strcmp(tokens[0], "cls") == 0) {
        for (int i = 0; i < 22; i++) {
            memset(state.buffer[i], ' ', 78);
            state.buffer[i][78] = '\0';
            state.row_dirty[i] = true;
        }
        free(line);
        return true;
    }
    if (line[0] >= 'a' && line[0] <= 'z' && (line[1] == '=') ) {
        basic.variables[line[0] - 'a'] = eval(line + 2);
        free(line);
        return true;
    }
    if (strcmp(tokens[0], "list") == 0) {
        for (u32 i = 0; i < basic.program_length; i++) {
            char* num = itoa(basic.program[i].ln, malloc(11), 10);
            u32 bytes = strlen(basic.program[i].line) + 12;
            char* buf = malloc(bytes);
            u32 j = 0;
            for (j = 0; num[j] != 0; j++) {
                buf[j] = num[j];
            }
            buf[j++] = ' ';
            for (u32 k = j; buf[j - k] != 0; j++) {
                buf[j] = basic.program[i].line[j - k];
            }
            free(buf);
            append_buffer(buf);
        }
        append_buffer(itoa(basic.current_line_number, "          ", 10));
        free(line);
        return true;
    }
    if (strcmp(tokens[0], "quit") == 0) {
        quit();
        free(line);
        return true;
    }
    // beep();
    append_buffer("?SYNTAX ERROR");
    free(line);
    return false;
}

static void continue_program() {
    char* line = strlwr(basic.program[basic.current_line_number].line);
    char* tokens[8] = {0};
    int count = split_tokens(line, " ", tokens, 8);

    if (!commit_instruction(tokens, count, line)) {
        basic.running = false;
        return;
    }
    if (basic.goto_performed) {
        if (basic.running) {
            basic.goto_performed = false;
            return;
        }
        basic.goto_performed = false;
    }

    basic.current_line_number++;

    if (basic.current_line_number == basic.program_length) {
        basic.running = false;
        basic.goto_performed = false;
        basic.current_line_number = 0;
    }
}

static void u32erpret_basic() {
    char* tokens[8] = {0};
    int count = split_tokens(state.input, " ", tokens, 8);
    if (count == 0) return;

    tokens[0] = strlwr(tokens[0]);

    if (state.input[0] >= '0' && state.input[0] <= '9') {
        u32 ln = (u32)atoi(tokens[0]);
        for (u32 i = 0; i < basic.program_length; i++) {
            if (basic.program[i].ln < ln) {
                continue;
            }
            if (basic.program[i].ln == ln) {
                free(basic.program[i].line);
                basic.program[i].line = strdup(state.input + strlen(tokens[0]) + 1);
                goto complete;
            }
            if (basic.program[i].ln > ln){
                // i am not implementing a pushing thing too much work
                // beep();
                append_buffer("?LAZINESS ERROR");
                goto complete;
            }
        }
        // we have to append now
        if (basic.program_length == basic.program_maxlen) {
            basic.program = realloc(basic.program, sizeof(struct program) * basic.program_maxlen);
            basic.program_maxlen *= 2;
        }
        basic.program[basic.program_length].ln = ln;
        basic.program[basic.program_length].line = strdup(state.input + strlen(tokens[0]) + 1);
        basic.program_length++;
    } else {
        commit_instruction(tokens, count, state.input);
    }

    complete:
    return;
}

char ch;

static void input_routine() {
    static bool tick = false;

    if (state.ready_for_input_init) {
        state.input = calloc(1, 64);
        state.input_length = 0;
        state.ready_for_input_init = false;
        state.ready_for_next_input = true;
        state.input_dirty = true;
        // tick = simple_state.tick;
    }

    if (state.ready_for_next_input) {
        read(fd, &ch, 1);
        if (ch == 0x0A/*\n*/) {
            // push the input to the buffer
            char* buffer = calloc(1, 79);
            append_buffer(strcat(strcat(buffer, "] "), state.input));
            free(buffer);

            state.ready_for_processing = true;
            state.ready_for_next_input = false;
            state.input_dirty = true;
        } else if (ch == 0x08) {
            if (state.input_length > 0) {
                state.input_length--;
                state.input[state.input_length] = 0;
                state.input_dirty = true;
            }
        } else {
            state.input[state.input_length] = ch;
            state.input_length++;
            state.input_dirty = true;
        }
    }

    if (state.ready_for_processing) {
        u32erpret_basic();
        free(state.input);
        state.input = NULL;
        state.ready_for_input_init = true;
        state.ready_for_processing = false;
    }
}

static void keysin() {
    if (!basic.running) input_routine();
    // if (keyboard_char(KEY_ESC)) basic.running = false;
}

void main() {
    printf("Press any key to start...\n");
    scanf("%c", &ch);
    setup();
    u64 last_frame = 0;

    while(1) {
        static u64 current_tick = 0;

        draw_outline();

        // clear_and_set_screen_color(0x0f);

        if ((++current_tick - last_frame) > (10000)) {
            render();
            disable_vga_cursor();
            // 1000/60 ~ 15
            last_frame = current_tick;

            if (basic.running) {
                continue_program();
            }
            keysin();

            if (state.end) {
                state.end = false;
                return;
            };
        }
    }
}
