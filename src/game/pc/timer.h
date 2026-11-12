#pragma once

#ifndef GAME_PC_TIMER_H
#define GAME_PC_TIMER_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// 0x00402570 (original PDB: 0x00402af0)
void TIMER_Init(void);

// 0x00402520
uint64_t WIN_TimeUS(void);

// 0x00402590
uint64_t TIMER_GetTimeUS(void);

// 0x00402ba0
uint32_t TIMER_GetTimeMS(void);

// 0x00402610
uint32_t TIMER_TimeDiff(uint32_t start, uint32_t end);

#ifdef __cplusplus
}
#endif

#endif // GAME_PC_TIMER_H
