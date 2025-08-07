//
// Created by Adithiya Venkatakrishnan on 20/12/2024.
//

#include "fungame.h"
#include "fg_internal.h"

#include <display/advanced/graphics.h>
#include <display/simple/display.h>
#include <memory/memory.h>
#include <modules/modules.h>
#include <modules/strings.h>
#include <pcspeaker/pcspeaker.h>
#include <ps2/keyboard.h>
#include <ps2/manualkeyboard.h>
#include <timer/PIT.h>

#include "fg_sprites.h"

typedef struct operation {
    bool active;
    u8 num1;
    u8 num2;

    enum { ADD, SUB, MUL, DIV } op;

    u16 ans;

    u32 end_frame;
} operation;

static struct {
    // graphics state
    u32 starting_frame;
    u32 frame;
    u32 second_frame;
    u32 fps;
    u32 fis; // frames in second
    bool game_setup;

    // gameplay state
    u32 score;
    u32 lives;
    u32 level;
    operation ops[4];
    u32 op_length;
    u32 new_ops_frame; // every say about 120 frames do a check

    // difficulty state
    u32 ms_min;
    u32 ms_max;

    u32 addsub_min;
    u32 addsub_max;

    u32 muldiv_min;
    u32 muldiv_max;

    u32 operation_time;

    // input state
    char* input;
    u32 input_length;
    u32 input_index;

    // dialogue state
    bool first_customer;
    bool has_been_full;
    bool full;
    bool last_was_incorrect;

    int handler;

    enum {
        START,
        INSTRUCTIONS,
        GAME,
        GAMEOVER,
        WIN
    } screen;

    bool paused;
} state;

// the actual frame of the frame
static void draw_frame() {
    draw_string(
        0, 0,
        "\xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB");
    draw_string(0, VGA_TEXT_HEIGHT - 1,
                "\xC8\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC");
    for (int i = 1; i < VGA_TEXT_HEIGHT - 1; i++) {
        draw_char(0, i, '\xBA');
        draw_char(VGA_TEXT_WIDTH - 1, i, '\xBA');
    }
    draw_string_with_color((VGA_TEXT_WIDTH / 2 - 1) - 4, 0, " fungame ",
                           VGA_TEXT_COLOR(COLOR_BLUE, COLOR_LIGHT_GREY));
}

static void setfps() {
    state.fps = state.frame - state.second_frame;
    state.second_frame = state.frame;
}

static void generateop();

static void setup() {
    enable_double_buffering();
    disable_vga_cursor();

    state.ops[0].active = false;
    state.ops[1].active = false;
    state.ops[2].active = false;
    state.ops[3].active = false;
    state.op_length = 0;

    state.lives = 3;
    state.level = 0;
    state.input = calloc(9);
    state.input_length = 0;
    state.input_index = 0;

    state.score = 0;

    state.first_customer = false;
    state.has_been_full = false;
    state.full = false;
    state.last_was_incorrect = false;
    state.paused = false;

    state.ms_min = 1000;
    state.ms_max = 10000;
    state.addsub_min = 10;
    state.addsub_max = 50;
    state.muldiv_min = 2;
    state.muldiv_max = 20;
    state.operation_time = 30;

    state.handler = run_every_second(setfps);
    state.screen = START;
}

static void render_start() {
    u8 index = (state.frame / 10) % 7;

    u8 colors[] = {
        VGA_TEXT_COLOR(COLOR_RED, COLOR_BLUE),
        VGA_TEXT_COLOR(COLOR_CYAN, COLOR_BLUE),
        VGA_TEXT_COLOR(COLOR_GREEN, COLOR_BLUE),
        VGA_TEXT_COLOR(COLOR_YELLOW, COLOR_BLUE),
        VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLUE),
        VGA_TEXT_COLOR(COLOR_DARK_GREY, COLOR_BLUE),
        VGA_TEXT_COLOR(COLOR_BLACK, COLOR_BLUE)
    };
    draw_sprite_with_color(FUNGAME_WORD, 5, 4, colors[index], colors[(index + 1) % 7], colors[(index + 2) % 7],
                           colors[(index + 3) % 7], colors[(index + 4) % 7], colors[(index + 5) % 7],
                           colors[(index + 6) % 7]);

    draw_string(X_MIDDLE - 10, Y_MIDDLE + 5, "Press ENTER to start");
    draw_string(X_MIDDLE - 14, Y_MIDDLE + 6, "Press ESC to exit at any time");
}

static void render_instructions() {
    // typewriter effect
    char* instructions = "Welcome to fungame, a game of mystery, excitement, and other\n"
        "varieties of fun. In this game, you will be simulating an ALU\n"
        "(arithmatic logic unit) that will be performing various operations\n"
        "for the CPU. The goal of the game is to get the correct output for\n"
        "every operation. If you get 3 operations wrong, the CPU will crash\n"
        "and the user will throw their computer out the window. Good luck!\n\n"
        "Press ESC to exit the game at any time. Press return to start. \nPress tab to pause.";

    int x = 6;
    int y = Y_MIDDLE - (8 / 2);
    for (u32 i = 0; i < MIN(state.frame - state.starting_frame, (u32)strlen(instructions)); i++) {
        if (instructions[i] == '\n') goto newline;
        draw_char(x, y, instructions[i]);
        x++;
        if ((x == X_SPACE - 5) && (i != 0)) {
        newline:
            x = 6;
            y++;
        }
    }
}

static char* currentop = "No operations outgoing...";

static void up_the_difficulty() {
    // at level 1, they are being introduced, so 1-8 seconds is good with a default max.
    // level 2 is faster, level 3 includes multiplication, level 4 probably start increasing speed and numbers.
    state.level++;
    switch (state.level) {
    case 1:
        state.ms_min = 1000;
        state.ms_max = 8000;
        break;
    case 2:
        state.ms_min = 1000;
        state.ms_max = 10000;
        state.muldiv_max = 20;
        break;
    case 3:
        state.muldiv_min = 5;
        state.muldiv_max = 20;
        state.addsub_min = 20;
        state.operation_time = 25;
        break;
    case 4:
        state.ms_min = 1000;
        state.ms_max = 8000;
        state.addsub_min = 25;
        state.addsub_max = 100;
        state.muldiv_min = 10;
        state.muldiv_max = 25;
        state.operation_time = 22;
        break;
    case 5:
        state.ms_min = 1000;
        state.ms_max = 6000;
        state.operation_time = 20;
        break;
    case 6:
        state.addsub_min = 40;
        state.addsub_max = 125;
        state.muldiv_min = 15;
        state.ms_min = 900;
        state.ms_max = 6000;
        state.operation_time = 18;
        break;
    default: break;
    }

    currentop = "Upping the ante...";
    state.starting_frame = state.frame;
}

static void RNG();

static void setup_game() {
    // wait_and_do(20000, has_been_quiet);
}

static void render_game() {
    // Show current score
    draw_string(13, 0, "Score: ");
    draw_string(20, 0, itoa(state.score, "         "));
    // Show current lives
    draw_string(2, 0, "Lives: ");
    draw_string(9, 0, itoa(state.lives, "         "));

    for (int i = 5, j = 0; i < 76; i += 18, j++) {
        if (state.ops[j].active) {
            char* num1 = itoa(state.ops[j].num1, malloc(11));
            char* num2 = itoa(state.ops[j].num2, malloc(11));

            switch (state.ops[j].op) {
            case ADD:
                draw_sprite_with_color(customer, i, 2, VGA_TEXT_COLOR(COLOR_GREEN, COLOR_LIGHT_BLUE));
                draw_string(i + 3, 4, "1000010001");
                draw_string(i + 4 + strlen(num1), 10, "+");
                break;
            case SUB:
                draw_sprite_with_color(customer, i, 2, VGA_TEXT_COLOR(COLOR_RED, COLOR_LIGHT_MAGENTA));
                draw_string(i + 3, 4, "1000011011");
                draw_string(i + 4 + strlen(num1), 10, "-");
                break;
            case MUL:
                draw_sprite_with_color(customer, i, 2, VGA_TEXT_COLOR(COLOR_BROWN, COLOR_YELLOW));
                draw_string(i + 3, 4, "1011110110");
                draw_string(i + 4 + strlen(num1), 10, "*");
                break;
            case DIV:
                draw_sprite_with_color(customer, i, 2, VGA_TEXT_COLOR(COLOR_DARK_GREY, COLOR_LIGHT_GREY));
                draw_string(i + 3, 4, "1011110111");
                draw_string(i + 4 + strlen(num1), 10, "/");
                break;
            }

            draw_string(i + 3, 5, "00");
            draw_string(i + 5, 5, num_to_bin(state.ops[j].num1, "000000000"));
            draw_string(i + 3, 6, "00");
            draw_string(i + 5, 6, num_to_bin(state.ops[j].num2, "000000000"));

            draw_string(i + 3, 10, num1);
            draw_string(i + 6 + strlen(num1), 10, num2);

            // draw_string(i + 3, 12, itoa(state.ops[j].ans, "0000000000"));

            char* seconds = itoa((state.ops[j].end_frame - state.frame) / 60, malloc(11));
            draw_string(i + 3, 18, seconds);
            draw_string(i + 3 + strlen(seconds) + 1, 18, "seconds");
            draw_string(i + 3 + strlen(seconds), 19, " left");
            free(seconds, 11);

            free(num1, 11);
            free(num2, 11);
        }
    }

    int x = 2;
    int y = 23;
    for (u32 i = 0; i < MIN(state.frame - state.starting_frame, (u32)strlen(currentop)); i++) {
        draw_char(x, y, currentop[i]);
        x++;
    }

    // input
    draw_string(60, 23, "Input: ");
    draw_string(67, 23, state.input);
    enable_vga_cursor();
    set_vga_cursor(67 + state.input_length, 23);
}

static char* timed_out() {
    char* phrases[] = {
        "Out of time! An 8086 could have done better...",
        "Out of time! Should have taken Kumon classes...",
        "Out of time! 5 year olds in China are faster...",
        "Out of time! The user is getting impatient...",
        "Out of time! And you had 30 seconds...",
        "Out of time! Tick, tick, tick, tick, tick...",
        "Out of time! Can't catch a break huh..."
    };
    return phrases[rand() % 7];
}

static void game_logic() {
    // if (state.frame - state.new_ops_frame >= 600) {
    //     state.new_ops_frame = state.frame;
    //     RNG();
    // }

    static bool initial = false;
    static int dialog_frame = 0;

    if (!initial) {
        initial = true;
        dialog_frame = state.frame / 60;
    }

    for (int i = 0; i < 4; i++) {
        if (state.ops[i].active && state.frame >= state.ops[i].end_frame) {
            state.lives--;
            currentop = timed_out();
            state.ops[i].active = false;
            state.op_length--;

            // cascade the operations
            for (int j = i; j < 3; j++) {
                state.ops[j] = state.ops[j + 1];
            }
            state.ops[3].active = false;
        }
    }

    if (state.lives <= 0) {
        wait_for_key_release('\n');
        state.screen = GAMEOVER;
    }

    // generate some new dialogue
    if (state.op_length == 1 && !state.first_customer) {
        currentop = "First operation incoming...";
        state.first_customer = true;
        state.starting_frame = state.frame;
    } else if (state.op_length == 4 && !state.full && !state.has_been_full) {
        currentop = "The house is full! The APU is overloaded!!!";
        state.has_been_full = true;
        state.full = true;
        state.starting_frame = state.frame;
    } else if ((state.frame / 60) - dialog_frame >= 10) {
        dialog_frame = state.frame / 60;
        // etc. dialogue
        if (state.frame - state.starting_frame >= 600 && !state.has_been_full) {
            static int current = 0;
            char* phrases[] = {
                "Things have been quiet for some time...",
                "Wow! You've managed to keep things under control...",
                "You're too good of a player...",
                "Okay, time to actually play the game...",
                "If you're so good, try having extra operations!!!"
            };
            currentop = phrases[current];
            current++;
            // after the "introductory" part (or level zero)
            if (current == 5) {
                RNG(); RNG(); RNG(); RNG(); state.has_been_full = true;
            }
            state.starting_frame = state.frame;
        }
    } else if ((state.frame / 60) - dialog_frame >= 10 && state.has_been_full && !state.full) {
        static int current = 0;
        char* phrases[] = {
            "You got it back under control...",
            "Nice job, you're pretty good...",
            "Looks like you actually took KUMON...",
            "Are you the GOAT or what?...",
            "I really ran out of dialogues to tell you..."
        };
        currentop = phrases[current];
        current++;
        if (current == 5) current = 0;
        state.starting_frame = state.frame;
    }
}

static u8 current_key_press() {
    // return any char that is currently being pressed, otherwise return 0
    for (u8 i = 0; i < 128; i++) { if (keyboard_char(i)) { return i; } }
    return 0;
}

static u8 c;

static char* incorrect_phrase() {
    char* phrases[] = {
        "Incorrect! Javascript floating point error...",
        "Incorrect! You're not a real programmer...",
        "Incorrect! 9 + 10 = 21...",
        "Incorrect! Should've majored in English...",
        "Incorrect! The answer is probably 42...",
        "Incorrect! You don't have many lives left...",
        "Incorrect! First place, first place...",
        "Incorrect! Should have taken KUMON classes...",
        "Incorrect! Sheldon Cooper laughs at you..."
    };
    return phrases[rand() % 9];
}

static void keysin_game_process() {
    state.input[state.input_length] = 0;

    // check if the input is correct
    if (!validate_number(state.input)) {
        currentop = "Invalid Input!";
        return;
    }

    u32 input = atoi(state.input);
    if (state.ops[0].active) {
        if (input == state.ops[0].ans) {
            state.score += 10;
            // currentop = "Correct!";
            if (state.full) {
                state.full = false;
                // wait_and_do(5000, recovery);
            }
            if (state.last_was_incorrect) {
                state.starting_frame = state.frame;
                currentop = "Back on track...";
                state.last_was_incorrect = false;
            }
            if (state.score % 100 == 0) up_the_difficulty();
        }
        else {
            if (state.full) {
                state.full = false;
                // wait_and_do(5000, recovery);
            }
            
            state.lives--;
            currentop = incorrect_phrase();
            state.last_was_incorrect = true;
            state.starting_frame = state.frame;
        }

        state.op_length = 0;
        state.input_length = 0;
        memset(state.input, 0, strlen(state.input));

        state.ops[0].active = false;
        state.ops[0] = state.ops[1];
        state.ops[1] = state.ops[2];
        state.ops[2] = state.ops[3];
        state.ops[3].active = false;
    }
}

static void keysin_game() {
    if (keyboard_char(c)) return; // wait for key release

    // all key presses are input
    if (keyboard_char('\b')) {
        c = '\b';
        if (state.input_length > 0) {
            state.input_length--;
            state.input[state.input_length] = 0;
        }
    }
    else if (keyboard_char('\n')) {
        c = '\n';
        keysin_game_process();
    } else {
        c = current_key_press();
        if (c != 0) {
            if (state.input_length < 8) {
                state.input[state.input_length++] = c;
                state.input[state.input_length] = 0;
            }
        }
    }
}

// gives a random time to generate the next operation
static void RNG() {
    wait_and_do(((rand() % state.ms_max) + state.ms_min), generateop);
}

static void generateop() {
    if (state.paused) {
        wait_and_do(5000, generateop);
        return;
    }

    operation op = {true, 0, 0, 0, 0, .end_frame=state.frame + (state.operation_time*60)};
    u8 oper1; // inefficient but I wrote this before understanding enums.
    if (state.level >= 3) oper1 = rand() % 4;
    else oper1 = rand() % 2;

    if (oper1 == 0) {
        op.op = ADD;

        // numbers don't matter if one's bigger than the other
        op.num1 = (rand() % state.addsub_max) + state.addsub_min;
        op.num2 = (rand() % state.addsub_max) + state.addsub_min;

        op.ans = op.num1 + op.num2;
    }
    else if (oper1 == 1) {
        op.op = SUB;

        // make sure the bigger number is the first number
        op.num1 = (rand() % state.addsub_max) + state.addsub_min;
        op.num2 = (rand() % state.addsub_max) + state.addsub_min;

        if (op.num1 < op.num2) {
            u8 temp = op.num1;
            op.num1 = op.num2;
            op.num2 = temp;
        }

        op.ans = op.num1 - op.num2;
    }
    else if (oper1 == 2) {
        // only after level 3

        // a lower max just because
        op.num1 = (rand() % state.muldiv_max) + state.muldiv_min;
        op.num2 = (rand() % state.muldiv_max) + state.muldiv_min;

        op.op = MUL;
        op.ans = op.num1 * op.num2;
    }
    else if (oper1 == 3) {
        // only after level 3

        // a lower max for the dividend
        // regular max for the divisor
        op.num2 = (rand() % state.muldiv_max) + state.muldiv_min;
        while (op.num2 == 0) op.num2 = (rand() % state.muldiv_max) + state.muldiv_min;
        op.num1 = ((rand() % state.muldiv_max) + state.muldiv_min) * op.num2;


        op.op = DIV;
        op.ans = op.num1 / op.num2;
    }

    for (int i = 0; i < 4; i++) {
        if (!state.ops[i].active) {
            state.ops[i] = op;
            state.op_length++;
            break;
        }
    }

    RNG();
}

static void render_paused() {
    for (int i = 0; i < 51; i++) {
        draw_char_with_color(X_MIDDLE - 25 + i, Y_MIDDLE - 5, '\xB0', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLUE));
    }
    for (int i = 0; i < 51; i++) {
        draw_char_with_color(X_MIDDLE - 25 + i, Y_MIDDLE + 5, '\xB0', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLUE));
    }
    for (int i = 0; i < 11; i++) {
        draw_char_with_color(X_MIDDLE - 25, Y_MIDDLE - 5 + i, '\xB0', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLUE));
    }
    for (int i = 0; i < 11; i++) {
        draw_char_with_color(X_MIDDLE + 25, Y_MIDDLE - 5 + i, '\xB0', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLUE));
    }

    for (int i = 0; i < 49; i++) {
        draw_char_with_color(X_MIDDLE - 24 + i, Y_MIDDLE - 4, '\xB1', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLUE));
    }
    for (int i = 0; i < 49; i++) {
        draw_char_with_color(X_MIDDLE - 24 + i, Y_MIDDLE + 4, '\xB1', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLUE));
    }
    for (int i = 0; i < 9; i++) {
        draw_char_with_color(X_MIDDLE - 24, Y_MIDDLE - 4 + i, '\xB1', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLUE));
    }
    for (int i = 0; i < 9; i++) {
        draw_char_with_color(X_MIDDLE + 24, Y_MIDDLE - 4 + i, '\xB1', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLUE));
    }

    for (int i = 0; i < 47; i++) {
        draw_char_with_color(X_MIDDLE - 23 + i, Y_MIDDLE - 3, '\xB2', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLUE));
    }
    for (int i = 0; i < 47; i++) {
        draw_char_with_color(X_MIDDLE - 23 + i, Y_MIDDLE + 3, '\xB2', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLUE));
    }
    for (int i = 0; i < 7; i++) {
        draw_char_with_color(X_MIDDLE - 23, Y_MIDDLE - 3 + i, '\xB2', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLUE));
    }
    for (int i = 0; i < 7; i++) {
        draw_char_with_color(X_MIDDLE + 23, Y_MIDDLE - 3 + i, '\xB2', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLUE));
    }

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 45; j++) {
            draw_char_with_color(X_MIDDLE - 22 + j, Y_MIDDLE - 2 + i, '\x00', VGA_TEXT_COLOR(COLOR_BLUE, COLOR_WHITE));
        }
    }

    draw_string(X_MIDDLE - (11 / 2), Y_MIDDLE - 1, "P A U S E D");
    draw_string(X_MIDDLE - (21 / 2), Y_MIDDLE + 1, "Press ENTER to resume");
}

static void render_gameover() {
    draw_string(X_MIDDLE - 7, Y_MIDDLE - 2, "G A M E O V E R");
    draw_string(X_MIDDLE - 11, Y_MIDDLE + 0, "Press ENTER to restart");
    draw_string(X_MIDDLE - 16, Y_MIDDLE + 1, "Press TAB to return to main menu");
    draw_string(X_MIDDLE - 8, Y_MIDDLE + 2, "Press ESC to quit");
}

static u32 current = 0;

static void render() {
    draw_frame();
    if (state.screen == START) { render_start(); }
    else if (state.screen == INSTRUCTIONS) { render_instructions(); }
    else if (state.screen == GAME) {
        if (!state.game_setup) {
            state.game_setup = true;
            setup_game();
        }
        if (!state.paused) render_game();
        if (state.paused) { render_paused(); }
    } else if (state.screen == GAMEOVER) {
        render_gameover();
    }
}

static void quit();

static void keysin() {
    if (state.screen == START) {
        if (keyboard_char('\n')) {
            nosound();
            wait_for_key_release('\n');
            state.screen = INSTRUCTIONS;
            state.starting_frame = state.frame;
            current = 0;
        }
    }
    else if (state.screen == INSTRUCTIONS) {
        if (keyboard_char('\n')) {
            wait_for_key_release('\n');
            state.screen = GAME;
            state.starting_frame = state.frame;
            current = 0;
            wait_and_do(2500, generateop);
        }
    }
    else if (state.screen == GAME) {
        if (keyboard_char('\t')) { state.paused = true; }
        if (state.paused) {
            if (keyboard_char('\n')) { wait_for_key_release('\n'); state.paused = false; }
            return;
        }

        if (!state.paused) keysin_game();
    } else if (state.screen == GAMEOVER) {
        if (keyboard_char('\n')) {
            state.screen = GAME;
            state.starting_frame = state.frame;
            current = 0;
            wait_and_do(2500, generateop);

            free(state.input, 11);
            setup();
        } else if (keyboard_char('\t')) {
            state.screen = START;
            state.starting_frame = state.frame;
            current = 0;
            wait_and_do(2500, generateop);

            free(state.input, 11);
            setup();
        } else if (keyboard_char(KEY_ESC)) {
            quit();
        }
    }
}

static void logic() {
    if (state.screen == GAME) {
        game_logic();
    }
}

static void stats() {
    draw_string(55, 0, "FPS: ");
    draw_string(60, 0, itoa(state.fps, "         "));

    draw_string(65, 0, "Frame: ");
    draw_string(72, 0, itoa(state.fis, "         "));
}

static void quit() {
    stop_run_every_second(state.handler);
    nosound();
    display.clear_screen();
    disable_double_buffering();
    free(state.input, 9);
    enable_vga_cursor();
    display.printf("Thank you for playing...\n");
}

static void music() {
    u32 current_music_tick = state.frame % 15;

    if (current_music_tick == 0) {
        if (state.screen == START) {
            play_sound(intro_music[current]);
            current = (current + 1) % 32;
        }
        else if (state.screen == INSTRUCTIONS) {
            play_sound(intructions_music[current]);
            current = (current + 1) % 64;
        }
    }
}

void fungame() {
    display.printf("Loading...");
    setup();
    sleep(500);
    u64 last_frame = 0;

    while (true) { // runloop
        // this takes all precedent.
        if (keyboard_char(KEY_ESC) == 1) {
            // revert back to simple driver
            quit();
            return;
        }

        u64 current_tick = timer_get();

        if ((current_tick - last_frame) > (15)) { // 1000/60 ~ 15
            last_frame = current_tick;

            clear_and_set_screen_color(VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLUE));

            music();
            logic();
            render();
            keysin();

            stats();

            if (!state.paused) {
                state.frame++;
                state.fis = state.frame - state.second_frame + 1;
            }
            else { nosound(); }
            swap_graphics_buffer();
        }
    }
}
