//
// Created by Adithiya Venkatakrishnan on 20/12/2024.
//

#include "fungame.h"
#include "fg_internal.h"

#include <display/advanced/graphics.h>
#include <display/simple/display.h>
#include <modules/modules.h>
#include <modules/strings.h>
#include <ps2/keyboard.h>
#include <ps2/manualkeyboard.h>
#include <timer/PIT.h>

#include "fg_sprites.h"

typedef enum current_screen {
    INSTRUCTIONS,
    GAME,
    GAMEOVER,
    WIN
} current_screen; // whole lotta yap from gpt4

struct {
    u32 starting_frame;
    u32 frame;
    u32 second_frame;
    u32 fps;
    u32 fis; // frames in second
    int handler;
    current_screen screen;
    bool paused;
} state;

// the actual frame of the frame
void draw_frame() {
    draw_string(
        0, 0,
        "\xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB");
    draw_string(0, VGA_TEXT_HEIGHT - 1,
                "\xC8\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC");
    for (int i = 1; i < VGA_TEXT_HEIGHT - 1; i++) {
        draw_char(0, i, '\xBA');
        draw_char(VGA_TEXT_WIDTH - 1, i, '\xBA');
    }
    draw_string_with_color((VGA_TEXT_WIDTH / 2 - 1) - 9 / 2, 0, " fungame ",
                           VGA_TEXT_COLOR(COLOR_BLUE, COLOR_LIGHT_GREY));
}

void setfps() {
    state.fps = state.frame - state.second_frame;
    state.second_frame = state.frame - 1;
}

void setup() {
    state.handler = run_every_second(setfps);
    state.screen = GAME;
}

struct maincharacter {
    u8 x;
    u8 y;
};

void render_instructions() {
    // typewriter effect
    char* instructions = "Welcome to fungame, a game of mystery, excitement, and other\n"
    "varieties of fun. In this game, you will be simulating an ALU\n"
    "(arithmatic logic unit) that will be performing various operations\n"
    "for the CPU. The goal of the game is to get the correct output for\n"
    "every operation. If you get 3 operations wrong, the CPU will crash\n"
    "and the user will throw their computer out the window. Good luck!\n\n"
    "Press ESC to exit the game at any time. Press return to start. \nPress delete to pause.";

    int x = 6;
    int y = Y_MIDDLE - (8/2);
    for (int i = 0; i < MIN(state.frame - state.starting_frame, (u32)strlen(instructions)); i++) {
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

void render_game() {
    draw_sprite_with_color(customer, 2, 2, VGA_TEXT_COLOR(COLOR_RED, COLOR_LIGHT_GREY), VGA_TEXT_COLOR(COLOR_WHITE, COLOR_LIGHT_RED));
}

void render_paused() {
    for (int i = 0; i < 51; i++) {
        draw_char(X_MIDDLE - 25 + i, Y_MIDDLE - 5, '\xB0');
    }
    for (int i = 0; i < 51; i++) {
        draw_char(X_MIDDLE - 25 + i, Y_MIDDLE + 5, '\xB0');
    }
    for (int i = 0; i < 11; i++) {
        draw_char(X_MIDDLE - 25, Y_MIDDLE - 5 + i, '\xB0');
    }
    for (int i = 0; i < 11; i++) {
        draw_char(X_MIDDLE + 25, Y_MIDDLE - 5 + i, '\xB0');
    }

    for (int i = 0; i < 49; i++) {
        draw_char(X_MIDDLE - 24 + i, Y_MIDDLE - 4, '\xB1');
    }
    for (int i = 0; i < 49; i++) {
        draw_char(X_MIDDLE - 24 + i, Y_MIDDLE + 4, '\xB1');
    }
    for (int i = 0; i < 9; i++) {
        draw_char(X_MIDDLE - 24, Y_MIDDLE - 4 + i, '\xB1');
    }
    for (int i = 0; i < 9; i++) {
        draw_char(X_MIDDLE + 24, Y_MIDDLE - 4 + i, '\xB1');
    }

    for (int i = 0; i < 47; i++) {
        draw_char(X_MIDDLE - 23 + i, Y_MIDDLE - 3, '\xB2');
    }
    for (int i = 0; i < 47; i++) {
        draw_char(X_MIDDLE - 23 + i, Y_MIDDLE + 3, '\xB2');
    }
    for (int i = 0; i < 7; i++) {
        draw_char(X_MIDDLE - 23, Y_MIDDLE - 3 + i, '\xB2');
    }
    for (int i = 0; i < 7; i++) {
        draw_char(X_MIDDLE + 23, Y_MIDDLE - 3 + i, '\xB2');
    }

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 45; j++) {
            draw_color(X_MIDDLE - 22 + j, Y_MIDDLE - 2 + i, VGA_TEXT_COLOR(COLOR_BLUE, COLOR_WHITE));
        }
    }

    draw_string(X_MIDDLE - (11/2), Y_MIDDLE - 1, "P A U S E D");
    draw_string(X_MIDDLE - (21/2), Y_MIDDLE + 1, "Press ENTER to resume");
}

void render() {
    draw_frame();
    if (state.screen == INSTRUCTIONS) {
        render_instructions();
    } else if (state.screen == GAME) {
        render_game();
        if (state.paused) {
            render_paused();
        }
    }
}

void keysin() {
    if (state.screen == INSTRUCTIONS) {
        if (keyboard_char('\n')) {
            state.screen = GAME;
            state.starting_frame = state.frame;
        }
    } else if (state.screen == GAME) {
        if (keyboard_char('\b')) {
            state.paused = true;
        }
        if (state.paused) {
            if (keyboard_char('\n')) {
                state.paused = false;
            }
        }
        if (state.paused) return; // don't do anything if paused
        if (keyboard_char('\n')) {
            // do something
        }
    }
}

void stats() {
    draw_string(55, 0, "FPS: ");
    draw_string(60, 0, itoa(state.fps, "         "));

    draw_string(65, 0, "Frame: ");
    draw_string(72, 0, itoa(state.fis, "         "));
}

void quit() {
    stop_run_every_second(state.handler);
    clear_screen();
    printf("Thank you for playing...\n");
}

void fungame() {
    printf("Loading...");
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

        if ((current_tick - last_frame) > (TIMER_TPS / FPS)) {
            last_frame = current_tick;

            clear_and_set_screen_color(VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLUE));

            render();
            keysin();

            stats();

            if (!state.paused) {
                state.frame++;
                state.fis = state.frame - state.second_frame + 1;
            }
            swap_graphics_buffer();
        }
    }
}
