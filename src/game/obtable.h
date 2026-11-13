#pragma once

#ifndef GAME_OBTABLE_H
#define GAME_OBTABLE_H

#include "common.h"
#include "cdc/runtime/cdcMath/Math.h"

// Forward declarations
struct Instance;
struct GameTracker;
struct ResolveObject;
struct Model;
struct AnimListEntry;
struct AnimFxHeader;
struct AnimScriptObject;
struct AnimBlend;
struct SFXData;
struct ObjectEffectFXA;
struct _GenericFXObject;
struct ObjectDTPData;
struct VramLink;
struct ClothSetup;

// Size: 0x8 (8 bytes, TI: 0x5aae)
struct ObjectBase {
    AnimBlend* animBlend;                   // 0x00
    void* animPattern;                      // 0x04
};
static_assert(sizeof(ObjectBase) == 0x8, "ObjectBase size mismatch");

// Callback function prototypes for objects/instances
typedef void (*InstanceInitFunc)(Instance* instance, GameTracker* tracker);
typedef void (*InstanceProcessFunc)(Instance* instance, GameTracker* tracker);
typedef void (*InstanceCollideFunc)(Instance* instance, GameTracker* tracker);
typedef unsigned int (*InstanceQueryFunc)(Instance* instance, unsigned int query);
typedef void (*InstanceMessageFunc)(Instance* instance, unsigned int message, unsigned int data);
typedef void (*InstanceAdditionalCollideFunc)(Instance* instance, GameTracker* tracker);

// 0x5cd6 in PDB / size: 0x80 (128 bytes)
struct Object {
    int oflags;                             // 0x00
    int oflags2;                            // 0x04
    int uniqueID;                           // 0x08
    unsigned int guiID;                     // 0x0c
    int functionTableID;                    // 0x10
    void* obsoleteSoundBank;                // 0x14
    int16_t numModels;                      // 0x18
    int16_t numAnims;                       // 0x1a
    int16_t numAnimPatterns;                // 0x1c
    int16_t pad01e;                         // 0x1e
    Model** modelList;                      // 0x20
    AnimListEntry* animList;                // 0x24
    AnimFxHeader** animFXList;              // 0x28
    AnimScriptObject** animPatternList;     // 0x2c
    int introDist;                          // 0x30
    int vvIntroDist;                        // 0x34
    int removeDist;                         // 0x38
    int vvRemoveDist;                       // 0x3c
    ObjectBase* baseData;                   // 0x40
    void* data;                             // 0x44
    char* name;                             // 0x48
    SFXData* soundData;                     // 0x4c
    int16_t sectionA;                       // 0x50
    int16_t sectionB;                       // 0x52
    int16_t sectionC;                       // 0x54
    unsigned char numberOfEffects;          // 0x56
    unsigned char pad057;                   // 0x57
    ObjectEffectFXA* effectList;            // 0x58
    _GenericFXObject* effectData;           // 0x5c
    ObjectDTPData* objectDTPData;           // 0x60
    VramLink* textureLoadList;              // 0x64
    void* childObjectList;                  // 0x68
    int lod1Dist;                           // 0x6c
    int lod2Dist;                           // 0x70
    signed char lod1Model;                  // 0x74
    signed char lod2Model;                  // 0x75
    signed char shadowModel;                // 0x76
    signed char lightingOverride;           // 0x77
    float maxCheckeeDistance;               // 0x78
    ClothSetup** rdSetupList;               // 0x7c
};
static_assert(sizeof(Object) == 0x80, "Object size mismatch");

// 0x6eab in PDB / size: 0x24 (36 bytes)
struct ObjectTracker {
    ResolveObject* resolveObj;              // 0x00
    char* objectName;                       // 0x04
    Object* object;                         // 0x08
    short objectID;                         // 0x0c
    short objectStatus;                     // 0x0e
    unsigned char numInUse;                 // 0x10
    unsigned char pad011;                   // 0x11
    signed char numObjectsUsing;            // 0x12
    signed char objectsUsing[13];           // 0x13
    void* vramBlock;                        // 0x20
};
static_assert(sizeof(ObjectTracker) == 0x24, "ObjectTracker size mismatch");

// 0x6c6f in PDB / size: 0x8 (8 bytes)
struct ObjectEntry {
    char* name;                             // 0x00
    int trackerIndex;                       // 0x04
};
static_assert(sizeof(ObjectEntry) == 0x8, "ObjectEntry size mismatch");

// 0x6c99 in PDB / size: 0x4 + N * 0x8
struct ObjectList {
    int numObjects;                         // 0x00
    ObjectEntry object[1];                  // 0x04
};

// 0x894a in PDB / size: 0x8 (8 bytes)
struct ObjectAccess {
    const char* objectName;                 // 0x00
    Object* object;                         // 0x04
};
static_assert(sizeof(ObjectAccess) == 0x8, "ObjectAccess size mismatch");

// 0xbd33 in PDB / size: 0x1c (28 bytes)
struct ObjectFunc {
    const char* scriptName;                                 // 0x00
    InstanceInitFunc initFunc;                              // 0x04
    InstanceProcessFunc processFunc;                        // 0x08
    InstanceCollideFunc collideFunc;                        // 0x0c
    InstanceQueryFunc queryFunc;                            // 0x10
    InstanceMessageFunc messageFunc;                        // 0x14
    InstanceAdditionalCollideFunc additionalCollideFunc;    // 0x18
};
static_assert(sizeof(ObjectFunc) == 0x1c, "ObjectFunc size mismatch");

// Global object tables
extern ObjectList* gObjectList;                 // 0x010efbd0 (PDB: ?gObjectList@@3PAUObjectList@@A)
extern ObjectTracker GlobalObjects[94];         // 0x01158518 (PDB: ?GlobalObjects@@3PAUObjectTracker@@A)
extern ObjectAccess objectAccess[];             // 0x00f15458 (PDB: ?objectAccess@@3PAUObjectAccess@@A)
extern ObjectFunc objectFunc[];                 // 0x00f15538 (PDB: ?objectFunc@@3PAUObjectFunc@@A)

// Subsystem functions
// 0x00461c60 (PDB: 0x00463620 / ?OBTABLE_InstanceInit@@YAXPAUInstance@@@Z)
void OBTABLE_InstanceInit(Instance* instance);

// 0x00461d00 (PDB: 0x004636c0 / ?OBTABLE_GetInstanceCollideFunc@@YAXPAUInstance@@@Z)
void OBTABLE_GetInstanceCollideFunc(Instance* instance);

// 0x00461d30 (PDB: 0x004636f0 / ?OBTABLE_GetInstanceAdditionalCollideFunc@@YAXPAUInstance@@@Z)
void OBTABLE_GetInstanceAdditionalCollideFunc(Instance* instance);

// 0x00461d60 (PDB: 0x00463720 / ?OBTABLE_GetInstanceProcessFunc@@YAXPAUInstance@@@Z)
void OBTABLE_GetInstanceProcessFunc(Instance* instance);

// 0x00461d90 (PDB: 0x00463750 / ?OBTABLE_GetInstanceQueryFunc@@YAXPAUInstance@@@Z)
void OBTABLE_GetInstanceQueryFunc(Instance* instance);

// 0x00461dc0 (PDB: 0x00463780 / ?OBTABLE_GetInstanceMessageFunc@@YAXPAUInstance@@@Z)
void OBTABLE_GetInstanceMessageFunc(Instance* instance);

// 0x00461df0 (PDB: 0x004637b0 / ?OBTABLE_InitObjectWithID@@YAXPAUObject@@@Z)
void OBTABLE_InitObjectWithID(Object* object);

// 0x00461eb0 (PDB: 0x00463870 / ?OBTABLE_SetObjectAccess@@YAXPAUObject@@@Z)
void OBTABLE_SetObjectAccess(Object* object);

// 0x00461ef0 (PDB: 0x004638b0 / ?OBTABLE_ClearObjectReferences@@YAXXZ)
void OBTABLE_ClearObjectReferences(void);

// 0x00461f10 (PDB: 0x004638d0 / ?OBTABLE_RemoveObjectEntry@@YAXPAUObject@@@Z)
void OBTABLE_RemoveObjectEntry(Object* object);

// 0x00461f40 (PDB: 0x00463900 / ?OBTABLE_FindObject@@YAPAUObject@@H@Z)
Object* OBTABLE_FindObject(int objectID);

// 0x00461f70 (PDB: 0x00463930 / ?OBTABLE_InitAnimPointers@@YAXPAUObjectTracker@@@Z)
void OBTABLE_InitAnimPointers(ObjectTracker* tracker);

// 0x00461f90 (PDB: 0x00463950 / ?OBTABLE_Init@@YAXXZ)
void OBTABLE_Init(void);

// 0x00462220 (PDB: 0x00463be0 / ?OBTABLE_GetObjectID@@YAHPAD@Z)
int OBTABLE_GetObjectID(char* name);

#endif // GAME_OBTABLE_H
