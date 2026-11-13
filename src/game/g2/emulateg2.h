#pragma once

#ifndef GAME_G2_EMULATEG2_H
#define GAME_G2_EMULATEG2_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/anim/anim.h"

struct BaseInstance;
struct CharacterState;
struct Instance;

void G2EmulationInit();                                                                                                                // 0x004de560 (PDB: 0x004e0090)
void G2EmulationInstanceToInstanceSwitchAnimation(Instance* instance, Instance* host, int NewAnim, int NewFrame, int Frames, int CurrentMode); // 0x004de670 (PDB: 0x004e01b0)
void G2EmulationInstanceSwitchAnimation(Instance* instance, int NewAnim, int NewFrame, int Frames, int CurrentMode);                   // 0x004de730 (PDB: 0x004e0270)
void G2EmulationInstanceToInstanceSwitchAnimationCharacter(Instance* instance, Instance* host, int NewAnim, int NewFrame, int Frames, int CurrentMode); // 0x004de7e0 (PDB: 0x004e0320)
void G2EmulationInstanceSetAnimation(BaseInstance* instance, int CurrentSection, int NewAnim, int NewFrame, int Frames);               // 0x004de8b0 (PDB: 0x004e03f0)
void G2EmulationInstanceSetAnimationLastFrame(Instance* instance, int NewAnim);                                                        // 0x004de960 (PDB: 0x004e04a0)
void G2EmulationSetAnimation(CharacterState* state, int NewAnim, int NewFrame, int Frames);                                            // 0x004de9d0 (PDB: 0x004e0510)
void G2EmulationInstanceSetMode(BaseInstance* instance, int CurrentSection, int Mode);                                                 // 0x004dea10 (PDB: 0x004e0550)
void G2EmulationInstanceSetAnimSpeed(Instance* instance, int CurrentSection, float speed);                                             // 0x004dea60 (PDB: 0x004e05a0)
void G2EmulationSetControllerAlpha(Instance* instance, int seg, int type, int alphaTableIndex);                                        // 0x004dea90 (PDB: 0x004e05d0)
int G2EmulationInstanceQueryAnimation(Instance* instance, int CurrentSection);                                                         // 0x004dead0 (PDB: 0x004e0610)
int G2EmulationInstanceQueryAnimLength(Instance* instance, int CurrentSection);                                                        // 0x004deaf0 (PDB: 0x004e0630)
int G2EmulationInstanceEventAnimPlaying(Instance* instance, int CurrentSection);                                                       // 0x004deb30 (PDB: 0x004e0670)
void G2EmulationInstanceSetEventAnimPlaying(Instance* instance, int CurrentSection);                                                   // 0x004deb60 (PDB: 0x004e06a0)
int G2EmulationInstanceQueryMode(Instance* instance, int CurrentSection);                                                              // 0x004deb90 (PDB: 0x004e06d0)
void G2EmulationInstanceSetStartAndEndSegment(BaseInstance* instance, int CurrentSection, short StartSeg, short EndSeg);               // 0x004debd0 (PDB: 0x004e0710)
void G2EmulationInstanceSetTotalSections(BaseInstance* instance, short Total);                                                         // 0x004dec00 (PDB: 0x004e0740)
void G2EmulationInstanceInitSection(Instance* instance, int CurrentSection, AnimCallbackFn callback, void* callbackData);              // 0x004dec50 (PDB: 0x004e0790)
void G2EmulationSetInterpController_Vector(Instance* instance, int segment, int type, cdc::Vector3* vec, int Frames, int Data);        // 0x004dec80 (PDB: 0x004e07c0)
void G2EmulationSetMode(CharacterState* state, int CurrentSection, int Mode);                                                          // 0x004decd0 (PDB: 0x004e0810)
int G2EmulationQueryAnimation(CharacterState* state, int CurrentSection);                                                              // 0x004ded20 (PDB: 0x004e0860)
void G2EmulationSetStartAndEndSegment(CharacterState* state, int CurrentSection, short StartSeg, short EndSeg);                         // 0x004ded50 (PDB: 0x004e0890)
void G2EmulationSetTotalSections(CharacterState* state, short Total);                                                                  // 0x004ded90 (PDB: 0x004e08d0)
int G2EmulationQueryInstanceAnimation(Instance* instance);                                                                             // 0x004deda0 (PDB: 0x004e08e0)
void G2EmulationInstancePlayAnimation(BaseInstance* instance);                                                                         // 0x004dedc0 (PDB: 0x004e0900)
void G2EmulatePlayAnimation(CharacterState* state);                                                                                    // 0x004dede0 (PDB: 0x004e0920)
int G2EmulationInstanceQueryFrame(Instance* instance, int CurrentSection);                                                             // 0x004dee00 (PDB: 0x004e0940)
int G2EmulationQueryFrame(CharacterState* state, int CurrentSection);                                                                  // 0x004dee30 (PDB: 0x004e0970)

#endif // GAME_G2_EMULATEG2_H
