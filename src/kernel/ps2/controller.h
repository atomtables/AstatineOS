//
// Created by Adithiya Venkatakrishnan on 19/12/2024.
//

#ifndef CONTROLLER_H
#define CONTROLLER_H

#define CONTROLLER_MAIN 0x64
#define CONTROLLER_AUX  0x60

#define CONTROLCMD_DISABLE_P1 0xAD
#define CONTROLCMD_ENABLE_P1  0xAE
#define CONTROLCMD_DISABLE_P2 0xA7
#define CONTROLCMD_ENABLE_P2  0xA8

void ps2_controller_init();

#endif //CONTROLLER_H
