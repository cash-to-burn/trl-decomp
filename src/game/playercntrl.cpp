#include "game/playercntrl.h"

// 0x00f159f4 (PDB: 0x00f15d2c / ?gPlayerControl_Enabled@@3_NA)
bool gPlayerControl_Enabled = true;

// Forward declaration of internal input reset
static void INPUT_Reset(void) {
    // 0x004e0380: Resets input controllers
}

// 0x00463b40 (PDB: 0x00465500 / ?PLAYERCNTRL_Enable@@YAXXZ)
void PLAYERCNTRL_Enable(void) {
    INPUT_Reset();
    gPlayerControl_Enabled = true;
}

// 0x00463b50 (PDB: 0x00465510 / ?PLAYERCNTRL_Disable@@YAXXZ)
void PLAYERCNTRL_Disable(void) {
    gPlayerControl_Enabled = false;
    INPUT_Reset();
}
