//
// Created by Adithiya Venkatakrishnan on 3/1/2025.
//

#include "netnotes.h"

#include <display/advanced/graphics.h>
#include <display/simple/display.h>
#include <memory/memory.h>
#include <memory/malloc.h>
#include <modules/strings.h>
#include <pcspeaker/pcspeaker.h>
#include <ps2/keyboard.h>
#include <ps2/manualkeyboard.h>
#include <timer/PIT.h>

#define X_SPACE 78
#define Y_SPACE 23
#define X_MIDDLE (X_SPACE / 2)
#define Y_MIDDLE (Y_SPACE / 2)

typedef struct pos {
    u32 x;
    u32 y;
} pos;

typedef struct note {
    char* title;
    char* content;
    u32 position; // in the content (for writes)
    pos location; // on screen (for cursor) (2 to 78 x, 3 to 23 y)
    u32 id;
} note;

static struct {
    u32 barrier;
    u32 page;

    u32 selected;

    note* notes;
    u32 size_notes;
    u32 count_notes;

    note* current_note;

    bool shouldQuit;
    bool firstRun;

    bool showPrompt;
    char* pquestion;
    char* pbuffer;
    u32 pindex;
    void (*paction)(void);
} data = {.firstRun = false, .selected = 0, .page = 0};

static void init_note(note* n) {
    n->title = kmalloc(128);
    n->content = kmalloc(2048); // 78 * 19
    memset(n->content, ' ', 2048);

    n->position = 0;
    n->location.x = 2;
    n->location.y = 3;

    n->id = rand();
}

static void create_note(void) {
    data.showPrompt = false;

    data.count_notes++;
    if (data.count_notes > data.size_notes) {
        data.notes = krealloc(data.notes, sizeof(note) * data.size_notes);
        data.size_notes *= 2;
    }

    note* n = &(data.notes[data.count_notes - 1]);
    init_note(n);
    strcpy(n->title, data.pbuffer);

    data.notes[data.count_notes - 1] = *n;

    data.selected = data.count_notes - 1;
}

static void create_sample_notes() {
    char** selections[] = {
        (char*[]){"Hello World", "testing"},
        (char*[]){
            "Goodbye World", "NetNotes is this really cool application for NetworkOS.\n"
            "It's a basic note taking app that can store your needs as\n"
            "you go. It can store as many notes as you need,\n"
            "and saves on exit until you restart."
        },
        (char*[]){"I'm confused", "computer science is too hard idk what im doing."},
        (char*[]){"What's the pou32", "\n\n\n\n\n            insert sample mental breakdown here"},
    };

    for (u32 i = 0; i < 4; i++) {
        note* n = &data.notes[i];
        init_note(n);
        strcpy(n->title, selections[i][0]);
        // only the creation of sample notes needs parsing
        u32 k = 0;
        u32 nl = 0;
        for (u32 j = 0; selections[i][1][j] != 0; j++, k++) {
            // if newline, write until the end of 78 characters
            if (selections[i][1][j] == '\n') {
                do {
                    n->content[k] = ' ';
                    k++;
                }
                while ((k) % 76 != 0);
                k--;
                nl++;
                continue;
            }

            n->content[k] = selections[i][1][j];
        }
        n->position = k;
        n->location.x = (k % 76) + 2;
        n->location.y = (k / 76) + 3;

        n->content[1444] = 0;
        n->id = i;
        data.notes[i] = *n;

        data.count_notes++;
    }
}

static void setup() {
    enable_double_buffering();
    disable_vga_cursor();
    data.shouldQuit = false;

    if (!data.firstRun) {
        data.notes = kcalloc(sizeof(note) * 10);
        data.size_notes = 10;
        data.count_notes = 0;
        create_sample_notes();
    }
    data.current_note = null;
    data.firstRun = true;
    // data.page = 0;
    data.selected = 0;
    data.showPrompt = false;

    data.pbuffer = kmalloc(64);
    data.pindex = 0;
}

static void render_prompt(char* question, char* buffer) {
    for (u32 i = 0; i < 51; i++) {
        draw_char_with_color(X_MIDDLE - 25 + i, Y_MIDDLE - 5, '\xB0', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLACK));
    }
    for (u32 i = 0; i < 51; i++) {
        draw_char_with_color(X_MIDDLE - 25 + i, Y_MIDDLE + 5, '\xB0', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLACK));
    }
    for (u32 i = 0; i < 11; i++) {
        draw_char_with_color(X_MIDDLE - 25, Y_MIDDLE - 5 + i, '\xB0', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLACK));
    }
    for (u32 i = 0; i < 11; i++) {
        draw_char_with_color(X_MIDDLE + 25, Y_MIDDLE - 5 + i, '\xB0', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLACK));
    }

    for (u32 i = 0; i < 49; i++) {
        draw_char_with_color(X_MIDDLE - 24 + i, Y_MIDDLE - 4, '\xB1', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLACK));
    }
    for (u32 i = 0; i < 49; i++) {
        draw_char_with_color(X_MIDDLE - 24 + i, Y_MIDDLE + 4, '\xB1', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLACK));
    }
    for (u32 i = 0; i < 9; i++) {
        draw_char_with_color(X_MIDDLE - 24, Y_MIDDLE - 4 + i, '\xB1', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLACK));
    }
    for (u32 i = 0; i < 9; i++) {
        draw_char_with_color(X_MIDDLE + 24, Y_MIDDLE - 4 + i, '\xB1', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLACK));
    }

    for (u32 i = 0; i < 47; i++) {
        draw_char_with_color(X_MIDDLE - 23 + i, Y_MIDDLE - 3, '\xB2', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLACK));
    }
    for (u32 i = 0; i < 47; i++) {
        draw_char_with_color(X_MIDDLE - 23 + i, Y_MIDDLE + 3, '\xB2', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLACK));
    }
    for (u32 i = 0; i < 7; i++) {
        draw_char_with_color(X_MIDDLE - 23, Y_MIDDLE - 3 + i, '\xB2', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLACK));
    }
    for (u32 i = 0; i < 7; i++) {
        draw_char_with_color(X_MIDDLE + 23, Y_MIDDLE - 3 + i, '\xB2', VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLACK));
    }

    for (u32 i = 0; i < 5; i++) {
        for (u32 j = 0; j < 45; j++) {
            draw_char_with_color(X_MIDDLE - 22 + j, Y_MIDDLE - 2 + i, '\x00', VGA_TEXT_COLOR(COLOR_BLACK, COLOR_WHITE));
        }
    }

    draw_string(X_MIDDLE - (strlen(question) / 2), Y_MIDDLE - 1, question);
    draw_string(X_MIDDLE - (strlen(buffer) / 2), Y_MIDDLE + 1, buffer);
}

static void draw_outline() {
    draw_string(
        0, 0,
        "\xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB");
    draw_string(0, VGA_TEXT_HEIGHT - 1,
                "\xC8\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC");
    for (u32 i = 1; i < VGA_TEXT_HEIGHT - 1; i++) {
        draw_char(0, i, '\xBA');
        draw_char(VGA_TEXT_WIDTH - 1, i, '\xBA');
    }
    draw_string_with_color(35, 0, " netnotes ",
                           VGA_TEXT_COLOR(COLOR_BLACK, COLOR_LIGHT_GREY));
}

static void render_menu() {
    // this will be a bunch of options on the below of the screen (2 lines)
    // y = 23
    draw_string(
        0, 22,
        "\xCC\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xB9");

    if (data.page == 0) {
        draw_string_with_color(3, 23, "^N", VGA_TEXT_COLOR(COLOR_BLACK, COLOR_LIGHT_GREY));
        draw_string(6, 23, "New");

        draw_string_with_color(10, 23, "^Q", VGA_TEXT_COLOR(COLOR_BLACK, COLOR_LIGHT_GREY));
        draw_string(13, 23, "Quit");
    }
    else if (data.page == 1) {
        draw_string_with_color(3, 23, "^X", VGA_TEXT_COLOR(COLOR_BLACK, COLOR_LIGHT_GREY));
        draw_string(6, 23, "Close");

        draw_string_with_color(12, 23, "^Q", VGA_TEXT_COLOR(COLOR_BLACK, COLOR_LIGHT_GREY));
        draw_string(15, 23, "Quit");
    }
}

static void render_selection() {
    for (u32 i = 0, y = 2; i < data.count_notes; i++, y++) {
        if (i == data.selected) {
            for (u32 x = 2; x < VGA_TEXT_WIDTH - 2; x++) {
                draw_color(x, y, VGA_TEXT_COLOR(COLOR_BLACK, COLOR_LIGHT_GREY));
            }
            draw_string_with_color(3, y, data.notes[i].title, VGA_TEXT_COLOR(COLOR_BLACK, COLOR_LIGHT_GREY));
            draw_string(50, y, xtoa_padded((u32)data.notes[i].title, "          "));
        }
        else { draw_string(3, y, data.notes[i].title); draw_string(50, y, xtoa_padded((u32)data.notes[i].title, "          ")); }
    }
    disable_vga_cursor();
}

static void render_note() {
    // draw the note
    // title
    draw_string(2, 1, data.current_note->title);
    draw_string(
        0, 2,
        "\xCC\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xB9");

    // content
    for (u32 i = 0, c = data.current_note->content[i], x = 2, y = 3;
         i < 76 * 19;
         i++, c = data.current_note->content[i]) {
        draw_char(x, y, c);
        x++;
        if (x > VGA_TEXT_WIDTH - 3) {
            x = 2;
            y++;
        }
    }

    set_vga_cursor(data.current_note->location.x, data.current_note->location.y);
}

static void render() {
    draw_outline();

    render_menu();
    if (data.page == 0) render_selection();
    if (data.showPrompt) render_prompt(data.pquestion, data.pbuffer);
    if (data.page == 1) render_note();
}

void debug(char* c) { for (u32 i = 0; c[i] != 0; i++) { outportb(0xE9, c[i]); } }

static void keysin_note(u8 c) {
    if (c == 0) { return; }
    if (c == KEY_BS) {
        if (data.current_note->location.x > 2) {
            data.current_note->location.x--;
            data.current_note->content[--data.current_note->position] = ' ';
        }
        else if (data.current_note->location.y > 3) {
            bool ignore = false;
            for (u32 i = 0; i < 76; i++) {
                if (data.current_note->content[data.current_note->position - i] != ' ') {
                    data.current_note->location.x = 76 - i + 3;
                    data.current_note->location.y--;
                    data.current_note->position -= i - 1;
                    ignore = true;
                    break;
                }
            }
            if (!ignore) {
                data.current_note->location.x = 2;
                data.current_note->location.y--;
                data.current_note->position -= 76;
            }
            data.current_note->content[data.current_note->position] = ' ';
        } // else { beep(); }
        return;
    }
    if (c == '\n') {
        // if there is content from the current position to the end of the line then return
        for (u32 i = data.current_note->position % 76; i < 76; i++) {
            if (data.current_note->content[data.current_note->position + i] != ' ') {
                beep();
                return;
            }
        }

        data.current_note->location.x = 2;
        data.current_note->location.y++;

        // set position to the next multiple of 76
        data.current_note->position += 76 - (data.current_note->position % 76);

        return;
    }
    if (c == '\t') {
        beep();
        return;
    }
    u32 x = data.current_note->location.x, y = data.current_note->location.y;
    data.current_note->content[data.current_note->position] = c;

    draw_char(x, y, c);
    x++;
    data.current_note->position++;
    if (x > 76) {
        x = 2;
        y++;
    }

    data.current_note->location.x = x;
    data.current_note->location.y = y;
}

static void quit();

static void loadnote() { enable_vga_cursor(); }

static void keysin_prompt() {
    static bool tick = false;
    if (simple_state.tick != tick) {
        tick = simple_state.tick;
        if (simple_state.current_char == '\n') {
            data.showPrompt = false;
            data.pindex = 0;
            data.paction();
        }
        data.pbuffer[data.pindex] = simple_state.current_char;
        data.pindex++;
        if (data.pindex > 63) { data.paction(); }
    }
}

static void keysin() {
    static u8 c;
    ret_if(keyboard_key(c));
    if (data.page == 0) {
        if (keyboard_key(KEY_UP)) {
            c = KEY_UP;
            data.selected--;
            if (data.selected < 0) data.selected = 0;
            return;
        }
        if (keyboard_key(KEY_DOWN)) {
            c = KEY_DOWN;
            data.selected++;
            if (data.selected > data.count_notes - 1) data.selected = data.count_notes - 1;
            return;
        }
        if (data.showPrompt) keysin_prompt();
        if (keyboard_char('\n')) {
            c = '\n';

            data.page = 1;
            data.current_note = &data.notes[data.selected];

            loadnote();

            return;
        }
        if (keyboard.ctrl) {
            if (keyboard_char('n')) {
                c = 'n';

                if (!data.showPrompt) {
                    data.pindex = 0;
                    data.pquestion = "Enter a title for the note:";
                    memset(data.pbuffer, 0, 64);
                    data.showPrompt = true;
                    data.paction = create_note;
                }

                return;
            }
            if (keyboard_char('q')) {
                c = 'q';
                quit();
                return;
            }
        }
    }
    else if (data.page == 1) {
        if (keyboard.ctrl) {
            if (keyboard_char('x')) {
                c = 'x';
                data.page = 0;
                return;
            }
            c = null;
            return;
        }

        static bool tick = false;
        if (simple_state.tick != tick) {
            tick = simple_state.tick;
            keysin_note(simple_state.current_char);
        }
    }
    c = null;
}

static void quit() {
    disable_double_buffering();
    clear_screen();
    enable_vga_cursor();
    printf("endall\n");

    kfree(data.pbuffer);

    data.shouldQuit = true;
}

// having a consistent frame rate isn't nearly as important
void netnotes() {
    wait_for_key_release('\n');
    setup();
    u64 last_frame = 0;

    while (true) { // runloop
        // this takes all precedent.

        u64 current_tick = timer_get();

        clear_and_set_screen_color(VGA_TEXT_COLOR(COLOR_WHITE, COLOR_BLACK));
        render(); // i had a dream today that logic and rendering should be separated

        if ((current_tick - last_frame) > (15)) {
            // 1000/60 ~ 15
            last_frame = current_tick;


            // music();
            // logic();

            keysin();
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

        if (data.shouldQuit) { break; }
    }

    return;
}
