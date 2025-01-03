//
// Created by Adithiya Venkatakrishnan on 3/1/2025.
//

#include "netnotes.h"

#include <display/advanced/graphics.h>
#include <display/simple/display.h>
#include <ps2/keyboard.h>
#include <ps2/manualkeyboard.h>
#include <timer/PIT.h>

static struct {

} data;

static void setup() {
    enable_double_buffering();
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
    draw_string_with_color(35, 0, " netnotes ",
                           VGA_TEXT_COLOR(COLOR_BLACK, COLOR_LIGHT_GREY));
}

static void draw_menu() {

}

static void render() {
    draw_outline();
}

static void quit() {
    disable_double_buffering();
    clear_screen();
    printf("endall\n");
}

// having a consistent frame rate isn't nearly as important
void netnotes() {
    wait_for_key_release('\n');
    setup();
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

            clear_and_set_screen_color(VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLACK));

            // music();
            // logic();
            render();
            // keysin();
            //
            // stats();
            //
            // if (!state.paused) {
            //     state.frame++;
            //     state.fis = state.frame - state.second_frame + 1;
            // }
            // else { nosound(); }
            swap_graphics_buffer();
        }
    }
}
