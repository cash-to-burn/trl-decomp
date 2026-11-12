#ifndef GAME_PLAYERCNTRL_H
#define GAME_PLAYERCNTRL_H

// 0x00f159f4 (PDB: 0x00f15d2c / ?gPlayerControl_Enabled@@3_NA)
extern bool gPlayerControl_Enabled;

// 0x00463b40 (PDB: 0x00465500 / ?PLAYERCNTRL_Enable@@YAXXZ)
void PLAYERCNTRL_Enable(void);

// 0x00463b50 (PDB: 0x00465510 / ?PLAYERCNTRL_Disable@@YAXXZ)
void PLAYERCNTRL_Disable(void);

#endif // GAME_PLAYERCNTRL_H
