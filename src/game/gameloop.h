#pragma once

#ifndef GAME_GAMELOOP_H
#define GAME_GAMELOOP_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// 0x0045cc10 - Main game loop and engine state machine
int GAMELOOP_Execute(int mode);

#ifdef __cplusplus
}
#endif

#endif // GAME_GAMELOOP_H
