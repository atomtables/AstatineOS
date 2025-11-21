#include "keyboard.h"
#include "controller.h"

void main() {
    ps2_controller_init();
    printf("Target complete: ps2 controller\n");

    keyboard_init();
    printf("Target complete: keyboard\n");
}