#include "modules.h"
#include "display.h"

/* the screen is 80x24 in text mode */
#define VGA_TEXT_WIDTH 80
#define VGA_TEXT_HEIGHT 24

// YO THIS GUY ONLINE WAS ACT LEGIT :skull:
int main() {
    clear_screen();
    __append_string__("Welcome to NetworkOS...");

    for(;;);
}