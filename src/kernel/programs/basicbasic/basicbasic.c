//
// Created by Adithiya Venkatakrishnan on 14/1/2025.
//

#include "basicbasic.h"

#include <display/advanced/graphics.h>
#include <display/simple/display.h>
#include <memory/memory.h>
#include <modules/strings.h>
#include <ps2/keyboard.h>
#include <ps2/manualkeyboard.h>
#include <timer/PIT.h>
#include <pcspeaker/pcspeaker.h>

static struct {
    char* input;
    u32 input_length;

    char buffer[78*22];
    u32 buffer_length;

    bool ready_for_input_init;
    bool ready_for_next_input;
    bool ready_for_processing;

    bool end;
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
    state.ready_for_input_init = true;
    state.ready_for_next_input = false;
    state.ready_for_processing = false;

    enable_double_buffering();

    memset(state.buffer, ' ', 78*22);

    basic.program = malloc(sizeof(struct program) * 50);
    basic.program_maxlen = 50;
}

static void quit() {
    disable_double_buffering();
    display.clear_screen();
    enable_vga_cursor();

    free(basic.program, sizeof(struct program) * basic.program_maxlen);
    state.end = true;
}

static void append_buffer(const char* line) {
    memcpy(state.buffer, state.buffer + 78, 78*21);
    memset(state.buffer + 78*21, 0, 78);
    strcpy(state.buffer + 78*21, line);
}

static void draw_outline() {
    draw_string(
        0, 0,
        "\xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB");
    draw_string(0, VGA_TEXT_HEIGHT - 1,
                "\xC8\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC");
    for (int i = 1; i < VGA_TEXT_HEIGHT - 1; i++) {
        draw_char(0, i, '\xBA');
        draw_char(VGA_TEXT_WIDTH - 1, i, '\xBA');
    }
    draw_string_with_color(34, 0, " basicbasic ",
                           VGA_TEXT_COLOR(COLOR_BLACK, COLOR_LIGHT_GREY));
}

static void render_command() {
    // history
    for (int i = 0, x = 1, y = 1; i < 78*22; i++) {
        draw_char(x, y, state.buffer[i]);
        x++;
        if (x == 79) {
            x = 1;
            y++;
        }
    }

    // command
    if (!basic.running) {
        draw_string(1, 23, "] ");
        for (int i = 0; i < state.input_length; i++) {
            draw_char(i + 3, 23, state.input[i]);
        }
        enable_vga_cursor();
        set_vga_cursor(state.input_length + 3, 23);
    }
}

static void render() {
    draw_outline();
    render_command();
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

static int eval(char* exp) {
    exp = remove_spaces(exp);
    StrtokA cmd = strtok_a(exp, "+-*/");

    if (cmd.count == 1) {
        return atoi(exp);
    }

    int num1;
    if (cmd.ret[0][0] >= 'a' && cmd.ret[0][0] <= 'z') {
        num1 = basic.variables[cmd.ret[0][0] - 'a'];
    } else {
        num1 = atoi(cmd.ret[0]);
    }
    int num2;
    if (cmd.ret[1][0] >= 'a' && cmd.ret[1][0] <= 'z') {
        num2 = basic.variables[cmd.ret[1][0] - 'a'];
    } else {
        num2 = atoi(cmd.ret[1]);
    }
    char op = *(exp + strlen(cmd.ret[0]));

    if (op == '+') {
        return num1 + num2;
    }
    if (op == '-') {
        return num1 - num2;
    }
    if (op == '*') {
        return num1 * num2;
    }
    if (op == '/') {
        return num1 / num2;
    }
    return -1;
}

static bool commit_instruction(StrtokA cmd, char* line) {
    line = remove_spaces(strdup(line));
    if (!strcmp(cmd.ret[0], "run")) {
        if (basic.running) {
            beep();
            append_buffer("?RUNNING ERROR");
        }
        basic.running = true;
        free(line, strlen(line) + 1);
        return true;
    }
    if (!strcmp(cmd.ret[0], "end")) {
        basic.running = false;
        free(line, strlen(line) + 1);
        return true;
    }
    if (!strcmp(cmd.ret[0], "goto")) {
        u32 ln = atoi(cmd.ret[1]);
        for (int i = 0; i < basic.program_length; i++) {
            if (basic.program[i].ln == ln) {
                basic.current_line_number = i;
                if (basic.running) basic.goto_performed = true;
                basic.running = true;
                free(line, strlen(line) + 1);
                return true;
            }
        }
        beep();
        append_buffer("?LINE ERROR");
        free(line, strlen(line) + 1);
        return false;
    }
    if (!strcmp(cmd.ret[0], "print")) {
        char* str = strlwr(line + 5);
        if (str[0] == '\"') {
            str++;
            str[strlen(str) - 1] = 0;
            append_buffer(str);
            free(line, strlen(line) + 1);
            return true;
        }
        if (str[0] >= 'a' && str[0] <= 'z' && str[1] == 0) {
            char* buf = itoa(basic.variables[str[0] - 'a'], malloc(11));
            append_buffer(buf);
            free(buf, 11);
            free(line, strlen(line) + 1);
            return true;
        }
        beep();
        append_buffer("?SYNTAX ERROR");
        free(line, strlen(line) + 1);
        return false;
    }
    if (!strcmp(cmd.ret[0], "cls")) {
        memset(state.buffer, ' ', 78*22);
        free(line, strlen(line) + 1);
        return true;
    }
    if (line[0] >= 'a' && line[0] <= 'z' && (line[1] == '=') ) {
        basic.variables[line[0] - 'a'] = eval(line + 2);
        free(line, strlen(line) + 1);
        return true;
    }
    if (!strcmp(cmd.ret[0], "list")) {
        for (int i = 0; i < basic.program_length; i++) {
            char* num = itoa(basic.program[i].ln, malloc(11));
            int bytes = strlen(basic.program[i].line) + 12;
            char* buf = malloc(bytes);
            int j = 0;
            for (j = 0; num[j] != 0; j++) {
                buf[j] = num[j];
            }
            buf[j++] = ' ';
            for (int k = j; buf[j - k] != 0; j++) {
                buf[j] = basic.program[i].line[j - k];
            }
            free(buf, bytes);
            append_buffer(buf);
        }
        append_buffer(itoa(basic.current_line_number, "          "));
        free(line, strlen(line) + 1);
        return true;
    }
    if (!strcmp(cmd.ret[0], "quit")) {
        quit();
        free(line, strlen(line) + 1);
        return true;
    }
    beep();
    append_buffer("?SYNTAX ERROR");
    free(line, strlen(line) + 1);
    return false;
}

static void continue_program() {
    char* line = strlwr(basic.program[basic.current_line_number].line);
    StrtokA cmd = strtok_a(line, " ");

    if (!commit_instruction(cmd, line)) {
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

static void interpret_basic() {
    StrtokA cmd = strtok_a(state.input, " ");

    cmd.ret[0] = strlwr(cmd.ret[0]);

    if (state.input[0] >= '0' && state.input[0] <= '9') {
        u32 ln = atoi(cmd.ret[0]);
        for (int i = 0; i < basic.program_length; i++) {
            if (basic.program[i].ln < ln) {
                continue;
            }
            if (basic.program[i].ln == ln) {
                free(basic.program[i].line, strlen(basic.program[i].line) + 1);
                basic.program[i].line = strdup(state.input + strlen(cmd.ret[0]) + 1);
                goto complete;
            }
            if (basic.program[i].ln > ln){
                // i am not implementing a pushing thing too much work
                beep();
                append_buffer("?LAZINESS ERROR");
                goto complete;
            }
        }
        // we have to append now
        if (basic.program_length == basic.program_maxlen) {
            basic.program = realloc(basic.program, sizeof(struct program) * basic.program_maxlen, sizeof(struct program) * basic.program_maxlen * 2);
            basic.program_maxlen *= 2;
        }
        basic.program[basic.program_length].ln = ln;
        basic.program[basic.program_length].line = strdup(state.input + strlen(cmd.ret[0]) + 1);
        basic.program_length++;
    } else {
        commit_instruction(cmd, state.input);
    }

    complete:
    free(cmd.ret, cmd.size);
}

static void input_routine() {
    static bool tick = false;

    if (state.ready_for_input_init) {
        state.input = calloc(64);
        state.input_length = 0;
        state.ready_for_input_init = false;
        state.ready_for_next_input = true;
        tick = simple_state.tick;
    }

    if (state.ready_for_next_input) {
        if (simple_state.tick != tick) {
            tick = simple_state.tick;

            if (simple_state.current_char == KEY_LF) {
                // push the input to the buffer
                char* buffer = calloc(79);
                append_buffer(strcat(strcat(buffer, "] "), state.input));
                free(buffer, 79);

                state.ready_for_processing = true;
                state.ready_for_next_input = false;
            } else if (simple_state.current_char == KEY_BS) {
                if (state.input_length > 0) {
                    state.input_length--;
                    state.input[state.input_length] = 0;
                }
            } else {
                state.input[state.input_length] = simple_state.current_char;
                state.input_length++;
            }
        }
    }

    if (state.ready_for_processing) {
        interpret_basic();
        free(state.input, 64);
        state.ready_for_input_init = true;
        state.ready_for_processing = false;
    }
}

static void keysin() {
    if (!basic.running) input_routine();
    if (keyboard_char(KEY_ESC)) basic.running = false;
}

void basicbasic() {
    wait_for_key_release('\n');
    setup();
    u64 last_frame = 0;

    while(1) {
        u64 current_tick = timer_get();

        draw_outline();

        clear_and_set_screen_color(0x0f);
        render();

        if ((current_tick - last_frame) > (15)) {
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

            swap_graphics_buffer();
        }
    }
}
