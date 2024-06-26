/* This will force us to create a kernel entry function instead of jumping to kernel.c:0x00 */
void dummy_test_entrypoint() {
}

void clear_screen() {
    volatile char* video_memory = (char*) 0xb8000;
    for (int i = 0; i < 80 * 25; ++i) {
        *video_memory = ' ';
        *(video_memory + 1) = 0x07;
        video_memory += 2;
    }
    video_memory = (char*) 0xb8000;
}

// YO THIS GUY ONLINE WAS ACT LEGIT :skull:
void main() {
    clear_screen();

    const char str[] = "Welcome to Protected Mode (C-land)!";

    volatile char* video_memory = (char*) 0xb8000;

    int color = 0x07;

    while(1) {
        for (int i = 0; str[i] != '\0'; ++i) {
            *video_memory = str[i];
            *(video_memory + 1) = color;
            video_memory += 2;
        }
        video_memory = (char*) 0xb8000;
        color += 1;

        // This is a hacky way to slow down the loop
        for (int i = 0; i < 100000000; ++i) {
            __asm__("nop");
        }
    };
}