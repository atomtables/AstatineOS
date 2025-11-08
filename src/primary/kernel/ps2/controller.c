//
// Created by Adithiya Venkatakrishnan on 19/12/2024.
//

#include "controller.h"

#include <display/simple/display.h>
#include <exception/exception.h>
#include <modules/modules.h>

#include "keyboard.h"

static const bool DEBUG = false;

#define wait_for(_cond) while (!(_cond)) { NOP(); }

bool verify_controller_ready_write() {
    u8 status = inportb(CONTROLLER_MAIN);
    return BIT_GET(status, 1) == 0;
}

bool verify_controller_ready_read() {
    u8 status = inportb(CONTROLLER_MAIN);
    return BIT_GET(status, 0) == 1;
}

bool verify_controller_response(u8 response) {
    if (response == 0x00 || response == 0xFF) {
        return false;
    }
    return true;
}

void sendcommandb(u8 cmd) {
    wait_for(verify_controller_ready_write());
    outportb(CONTROLLER_MAIN, cmd);
}

void sendcommandw(u8 cmd, u8 subcmd) {
    wait_for(verify_controller_ready_write());
    outportb(CONTROLLER_MAIN, cmd);

    wait_for(verify_controller_ready_write());
    outportb(CONTROLLER_AUX, subcmd);
}

u8 readdatab() {
    wait_for(verify_controller_ready_read());
    return inportb(CONTROLLER_AUX);
}

u8 read_configuration_byte() {
    sendcommandb(0x20);
    u8 config = readdatab();
    return config;
}

void write_configuration_byte(u8 config) {
    sendcommandw(0x60, config);
    // Don't read response - there isn't one for this command
    if (DEBUG) display.printf("current controller byte: 0x%x\n", read_configuration_byte());
}

void ps2_controller_init() {
    sendcommandb(CONTROLCMD_DISABLE_P1);

    u8 config = read_configuration_byte();
    config |= 0x01; // Enable first PS/2 port
    config |= 0x40; // Enable first PS/2 translation
    write_configuration_byte(config);

    keyboard_irq_enabled = true;
    keyboard_translation_enabled = true;

    // sendcommandb(CONTROLCMD_ENABLE_P1);
}