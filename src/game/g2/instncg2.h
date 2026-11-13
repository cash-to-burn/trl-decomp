#pragma once

#ifndef GAME_G2_INSTNCG2_H
#define GAME_G2_INSTNCG2_H

#include "cdc/runtime/cdcMath/Math.h"

struct AnimFragment;
struct BaseInstance;
struct Instance;
struct Object;
struct Segment;

void G2Instance_Reset();                                                                                   // 0x004702f0
AnimFragment* G2Instance_GetKeylist(BaseInstance* inst, int keylistID);                                    // 0x004dee70 (PDB: 0x004e09a0)
AnimFragment* G2Instance_GetKeylistFromObject(Object* obj, int keylistID);                                 // 0x004deea0 (PDB: 0x004e09d0)
void G2Instance_BuildClothTransforms(Instance* inst);                                                      // 0x004deed0 (PDB: 0x004e0a00)
void G2Instance_BuildVirtualTransforms(Instance* inst);                                                    // 0x004deef0 (PDB: 0x004e0a20)
void G2Instance_NextKeyframe(BaseInstance* inst);                                                          // 0x004defd0 (PDB: 0x004e0b00)
int G2Instance_GetNumSections(Instance* inst);                                                             // 0x004df020 (PDB: 0x004e0b50)
void _G2Instance_RebuildAnimatedTransforms(Instance* inst);                                                // 0x004df060 (PDB: 0x004e0b90)
void G2Instance_SetTransformsToIdentity(BaseInstance* inst);                                               // 0x004df4b0 (PDB: 0x004e0fe0)
void _G2Instance_BuildFacadeTransforms(Instance* inst, Segment* segment, cdc::Matrix* segMat, cdc::Matrix* parentMat); // 0x004df760 (PDB: 0x004e1290)
void _G2Instance_RebuildNonAnimatedTransforms(Instance* inst);                                             // 0x004dfa40 (PDB: 0x004e1570)
void G2Instance_BuildTransforms(Instance* inst);                                                           // 0x004dfc60 (PDB: 0x004e1790)
void G2Instance_RebuildTransforms(Instance* inst);                                                         // 0x004dff40 (PDB: 0x004e1a70)
void G2Instance_BuildTransformsForList(Instance* inst);                                                    // 0x004dff60 (PDB: 0x004e1a90)

#endif // GAME_G2_INSTNCG2_H
