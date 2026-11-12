#include "game/obtable.h"
#include "game/instance.h"
#include "game/gameloop.h"
#include "game/main.h"
#include "game/player/player.h"
#include "game/pc/pclocals.h"
#include "game/archive/ArchiveFileSystem.h"
#include "cdc/cdcFile/FileSystem.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Global object tables
ObjectList* gObjectList = nullptr;              // 0x010efbd0 (PDB: ?gObjectList@@3PAUObjectList@@A)
ObjectTracker GlobalObjects[94];                // 0x01158518 (PDB: ?GlobalObjects@@3PAUObjectTracker@@A)

// 0x00f15458 (PDB: ?objectAccess@@3PAUObjectAccess@@A)
ObjectAccess objectAccess[] = {
    { "waterfx", nullptr },
    { "particle", nullptr },
    { "bullet_hit", nullptr },
    { "bullet_hit_shotgun", nullptr },
    { "fx_grapple_bling", nullptr },
    { "generalbank", nullptr },
    { "handgun_rbweapon", nullptr },
    { "handgun_rbweapon_upa", nullptr },
    { "handgun_rbweapon_upb", nullptr },
    { "handgun_rbweapon_upc", nullptr },
    { "handgun_revolver", nullptr },
    { "shotgun_rbweapon", nullptr },
    { "autorifle_rbweapon", nullptr },
    { "rpg_rbweapon", nullptr },
    { "grenadelauncher_rbweapon", nullptr },
    { "smgmpfive_rbweapon", nullptr },
    { "excalibur_rbweapon", nullptr },
    { "mreaver_rbweapon", nullptr },
    { "g_grenade_thrower", nullptr },
    { "g_binoculars", nullptr },
    { "g_flare", nullptr },
    { "g_glowstick", nullptr },
    { "g_healthpack", nullptr },
    { "magnetic_grapple", nullptr },
    { "g_personal_light", nullptr },
    { "ea_russia_parachute", nullptr },
    { nullptr, nullptr }
};

#define PLAYER_OBJECT_FUNCS \
    reinterpret_cast<InstanceInitFunc>(RazielInit), \
    reinterpret_cast<InstanceProcessFunc>(RazielProcess), \
    reinterpret_cast<InstanceCollideFunc>(RazielCollide), \
    reinterpret_cast<InstanceQueryFunc>(RazielQuery), \
    reinterpret_cast<InstanceMessageFunc>(RazielPost), \
    reinterpret_cast<InstanceAdditionalCollideFunc>(RazielAdditionalCollide)

// 0x00f15538 (PDB: ?objectFunc@@3PAUObjectFunc@@A)
ObjectFunc objectFunc[] = {
    { "lara", PLAYER_OBJECT_FUNCS },
    { "lara_winter", PLAYER_OBJECT_FUNCS },
    { "lara_young", PLAYER_OBJECT_FUNCS },
    { "lara_biker", PLAYER_OBJECT_FUNCS },
    { "lara_evening", PLAYER_OBJECT_FUNCS },
    { "lara_evening_alt", PLAYER_OBJECT_FUNCS },
    { "physical", reinterpret_cast<InstanceInitFunc>(0x005333a0), nullptr, nullptr, nullptr, nullptr, nullptr },
    { "particle", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr },
    { "enemy", reinterpret_cast<InstanceInitFunc>(0x00494030), reinterpret_cast<InstanceProcessFunc>(0x00492b20), reinterpret_cast<InstanceCollideFunc>(0x00492f30), reinterpret_cast<InstanceQueryFunc>(0x00492b90), reinterpret_cast<InstanceMessageFunc>(0x00492bc0), reinterpret_cast<InstanceAdditionalCollideFunc>(0x004932c0) },
    { "flashback_stream", reinterpret_cast<InstanceInitFunc>(0x005d6450), reinterpret_cast<InstanceProcessFunc>(0x005d6660), nullptr, reinterpret_cast<InstanceQueryFunc>(0x005d5d20), reinterpret_cast<InstanceMessageFunc>(0x005d69f0), nullptr },
    { "russia_stream", reinterpret_cast<InstanceInitFunc>(0x005d6450), reinterpret_cast<InstanceProcessFunc>(0x005d6660), nullptr, reinterpret_cast<InstanceQueryFunc>(0x005d5d20), reinterpret_cast<InstanceMessageFunc>(0x005d69f0), nullptr },
    { "tokyo_stream", reinterpret_cast<InstanceInitFunc>(0x005d6450), reinterpret_cast<InstanceProcessFunc>(0x005d6660), nullptr, reinterpret_cast<InstanceQueryFunc>(0x005d5d20), reinterpret_cast<InstanceMessageFunc>(0x005d69f0), nullptr },
    { "lara_classic", PLAYER_OBJECT_FUNCS },
    { "lara_classic_alt", PLAYER_OBJECT_FUNCS },
    { "lara_goth", PLAYER_OBJECT_FUNCS },
    { "lara_goth_alt", PLAYER_OBJECT_FUNCS },
    { "lara_biker_alt", PLAYER_OBJECT_FUNCS },
    { "lara_biker_nj", PLAYER_OBJECT_FUNCS },
    { "lara_suit", PLAYER_OBJECT_FUNCS },
    { "lara_suit_alt", PLAYER_OBJECT_FUNCS },
    { "lara_alta", PLAYER_OBJECT_FUNCS },
    { "lara_altb", PLAYER_OBJECT_FUNCS },
    { "lara_altc", PLAYER_OBJECT_FUNCS },
    { "lara_alt", PLAYER_OBJECT_FUNCS },
    { "lara_winter_alt", PLAYER_OBJECT_FUNCS },
    { "lara_winter_alta", PLAYER_OBJECT_FUNCS },
    { "lara_winter_nj", PLAYER_OBJECT_FUNCS },
    { "lara_winter_alt_nj", PLAYER_OBJECT_FUNCS },
    { "lara_winter_alta_nj", PLAYER_OBJECT_FUNCS },
    { "lara_evening_red", PLAYER_OBJECT_FUNCS },
    { "lara_evening_alta", PLAYER_OBJECT_FUNCS },
    { "lara_special_forces", PLAYER_OBJECT_FUNCS },
    { "lara_special_forces_alt", PLAYER_OBJECT_FUNCS },
    { "lara_catsuit", PLAYER_OBJECT_FUNCS },
    { "lara_catsuit_snow", PLAYER_OBJECT_FUNCS },
    { "lara_bikini", PLAYER_OBJECT_FUNCS },
    { "lara_bikini_alt", PLAYER_OBJECT_FUNCS },
    { "lara_sport", PLAYER_OBJECT_FUNCS },
    { "lara_sport_alt", PLAYER_OBJECT_FUNCS },
    { "amanda_player", PLAYER_OBJECT_FUNCS },
    { "amanda_player_alt", PLAYER_OBJECT_FUNCS },
    { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }
};

// 0x00461c60 (PDB: 0x00463620 / ?OBTABLE_InstanceInit@@YAXPAUInstance@@@Z)
void OBTABLE_InstanceInit(Instance* instance)
{
    if (instance == nullptr || instance->object == nullptr) {
        return;
    }

    Object* obj = instance->object;
    int funcId = obj->functionTableID;

    if ((instance->iflags & 0x20000) == 0) {
        // Spatial hierarchy update
        typedef void (*SpatialFunc)(Instance*);
        reinterpret_cast<SpatialFunc>(0x004df4b0)(instance);
    }

    if (funcId < 0) {
        // Generic instance init
        typedef void (*GenericInitFunc)(Instance*, GameTracker*);
        reinterpret_cast<GenericInitFunc>(0x00453110)(instance, &g_gameTracker);
    } else {
        InstanceInitFunc initFunc = objectFunc[funcId].initFunc;
        if (initFunc != nullptr) {
            initFunc(instance, &g_gameTracker);
        }
    }

    instance->iflags2 |= 0x200000;

    if ((instance->iflags & 0x100000) == 0 && (obj->oflags2 & 0x200000) == 0) {
        typedef void (*PostInitFunc)(Instance*);
        reinterpret_cast<PostInitFunc>(0x00465350)(instance);
    }

    if (instance->intro != nullptr && (instance->intro->introFlags & 0x20000000) != 0) {
        instance->iflags2 &= ~0x20004; // 0xfffdfffb
    }
}

// 0x00461d00 (PDB: 0x004636c0 / ?OBTABLE_GetInstanceCollideFunc@@YAXPAUInstance@@@Z)
void OBTABLE_GetInstanceCollideFunc(Instance* instance)
{
    if (instance == nullptr || instance->object == nullptr) return;
    int funcId = instance->object->functionTableID;
    if (funcId >= 0) {
        instance->collideFunc = objectFunc[funcId].collideFunc;
    } else {
        instance->collideFunc = reinterpret_cast<InstanceCollideFunc>(0x004dee60);
    }
}

// 0x00461d30 (PDB: 0x004636f0 / ?OBTABLE_GetInstanceAdditionalCollideFunc@@YAXPAUInstance@@@Z)
void OBTABLE_GetInstanceAdditionalCollideFunc(Instance* instance)
{
    if (instance == nullptr || instance->object == nullptr) return;
    int funcId = instance->object->functionTableID;
    if (funcId >= 0) {
        instance->additionalCollideFunc = objectFunc[funcId].additionalCollideFunc;
    } else {
        instance->additionalCollideFunc = nullptr;
    }
}

// 0x00461d60 (PDB: 0x00463720 / ?OBTABLE_GetInstanceProcessFunc@@YAXPAUInstance@@@Z)
void OBTABLE_GetInstanceProcessFunc(Instance* instance)
{
    if (instance == nullptr || instance->object == nullptr) return;
    int funcId = instance->object->functionTableID;
    if (funcId >= 0) {
        instance->processFunc = objectFunc[funcId].processFunc;
    } else {
        instance->processFunc = reinterpret_cast<InstanceProcessFunc>(0x004531e0);
    }
}

// 0x00461d90 (PDB: 0x00463750 / ?OBTABLE_GetInstanceQueryFunc@@YAXPAUInstance@@@Z)
void OBTABLE_GetInstanceQueryFunc(Instance* instance)
{
    if (instance == nullptr || instance->object == nullptr) return;
    int funcId = instance->object->functionTableID;
    if (funcId >= 0) {
        instance->queryFunc = objectFunc[funcId].queryFunc;
    } else {
        instance->queryFunc = reinterpret_cast<InstanceQueryFunc>(0x00453200);
    }
}

// 0x00461dc0 (PDB: 0x00463780 / ?OBTABLE_GetInstanceMessageFunc@@YAXPAUInstance@@@Z)
void OBTABLE_GetInstanceMessageFunc(Instance* instance)
{
    if (instance == nullptr || instance->object == nullptr) return;
    int funcId = instance->object->functionTableID;
    if (funcId >= 0) {
        instance->messageFunc = objectFunc[funcId].messageFunc;
    } else {
        instance->messageFunc = reinterpret_cast<InstanceMessageFunc>(0x004534d0);
    }
}

// 0x00461df0 (PDB: 0x004637b0 / ?OBTABLE_InitObjectWithID@@YAXPAUObject@@@Z)
void OBTABLE_InitObjectWithID(Object* object)
{
    if (object == nullptr || gObjectList == nullptr) return;

    // Set object name from gObjectList (1-based ID)
    if (object->uniqueID > 0 && object->uniqueID <= gObjectList->numObjects) {
        object->name = gObjectList->object[object->uniqueID - 1].name;
    }

    const char* scriptName = nullptr;
    if (object->oflags & 0x20000000) {
        scriptName = "physical";
    } else if (object->oflags2 & 0x00100000) {
        scriptName = "enemy";
    } else {
        scriptName = object->name;
    }

    int funcId = -1;
    if (scriptName != nullptr) {
        for (int i = 0; objectFunc[i].scriptName != nullptr; i++) {
            if (_stricmp(objectFunc[i].scriptName, scriptName) == 0) {
                funcId = i;
                break;
            }
        }
    }
    object->functionTableID = funcId;
}

// 0x00461eb0 (PDB: 0x00463870 / ?OBTABLE_SetObjectAccess@@YAXPAUObject@@@Z)
void OBTABLE_SetObjectAccess(Object* object)
{
    if (object == nullptr || object->name == nullptr) return;
    for (ObjectAccess* access = objectAccess; access->objectName != nullptr; access++) {
        if (_stricmp(access->objectName, object->name) == 0) {
            access->object = object;
            break;
        }
    }
}

// 0x00461ef0 (PDB: 0x004638b0 / ?OBTABLE_ClearObjectReferences@@YAXXZ)
void OBTABLE_ClearObjectReferences(void)
{
    for (ObjectAccess* access = objectAccess; access->objectName != nullptr; access++) {
        access->object = nullptr;
    }
}

// 0x00461f10 (PDB: 0x004638d0 / ?OBTABLE_RemoveObjectEntry@@YAXPAUObject@@@Z)
void OBTABLE_RemoveObjectEntry(Object* object)
{
    for (ObjectAccess* access = objectAccess; access->objectName != nullptr; access++) {
        if (access->object == object) {
            access->object = nullptr;
            break;
        }
    }
}

// 0x00461f40 (PDB: 0x00463900 / ?OBTABLE_FindObject@@YAPAUObject@@H@Z)
Object* OBTABLE_FindObject(int objectID)
{
    for (int i = 0; i < 94; i++) {
        if (GlobalObjects[i].objectStatus == 2 && GlobalObjects[i].objectID == objectID) {
            return GlobalObjects[i].object;
        }
    }
    return nullptr;
}

// 0x00461f70 (PDB: 0x00463930 / ?OBTABLE_InitAnimPointers@@YAXPAUObjectTracker@@@Z)
void OBTABLE_InitAnimPointers(ObjectTracker* tracker)
{
    if (tracker == nullptr || tracker->object == nullptr) return;
    Object* obj = tracker->object;
    if (obj->animList == nullptr) {
        obj->oflags2 |= 0x80;
    }
}

// 0x00461f90 (PDB: 0x00463950 / ?OBTABLE_Init@@YAXXZ)
void OBTABLE_Init(void)
{
    char filePath[260];
    snprintf(filePath, sizeof(filePath), "\\TR7\\%s\\objectlist.txt", g_buildDir);

    char* fileBuffer = static_cast<char*>(FSHelper_ReadFile(filePath, 0, g_pArchiveFS ? g_pArchiveFS : g_pFileSystem));
    if (!fileBuffer) {
        return;
    }

    char* p = fileBuffer;
    int numLines = 0;
    if (sscanf(p, "%d", &numLines) != 1) {
        return;
    }

    // Skip header line
    while (*p && *p != '\n') p++;
    while (*p == '\r' || *p == '\n') p++;

    char* entryStart = p;
    int maxObjectID = 0;

    // Pass 1: find maxObjectID
    for (int i = 0; i < numLines && *p; i++) {
        int id = 0;
        if (sscanf(p, "%d,", &id) == 1) {
            if (id > maxObjectID) {
                maxObjectID = id;
            }
        }
        while (*p && *p != '\n') p++;
        while (*p == '\r' || *p == '\n') p++;
    }

    if (gObjectList) {
        free(gObjectList);
        gObjectList = nullptr;
    }

    // Allocate ObjectList with maxObjectID entries + string storage
    size_t stringStorageSize = (p - entryStart) + 4;
    size_t allocSize = sizeof(int) + maxObjectID * sizeof(ObjectEntry) + stringStorageSize;
    gObjectList = static_cast<ObjectList*>(malloc(allocSize));
    if (!gObjectList) {
        return;
    }

    gObjectList->numObjects = maxObjectID;
    for (int i = 0; i < maxObjectID; i++) {
        gObjectList->object[i].name = nullptr;
        gObjectList->object[i].trackerIndex = -1;
    }

    char* strDest = reinterpret_cast<char*>(&gObjectList->object[maxObjectID]);
    p = entryStart;

    // Pass 2: populate names
    for (int i = 0; i < numLines && *p; i++) {
        int id = 0;
        if (sscanf(p, "%d,%s", &id, strDest) == 2) {
            char* comma = strchr(strDest, ',');
            if (comma) {
                *comma = '\0';
            }
            if (id > 0 && id <= maxObjectID) {
                gObjectList->object[id - 1].name = strDest;
            }
            strDest += strlen(strDest) + 1;
        }
        while (*p && *p != '\n') p++;
        while (*p == '\r' || *p == '\n') p++;
    }

    // Fix up any active objects in GlobalObjects[94]
    for (int i = 0; i < 94; i++) {
        if (GlobalObjects[i].objectStatus != 0 && GlobalObjects[i].object != nullptr) {
            Object* obj = GlobalObjects[i].object;
            if (obj->uniqueID > 0 && obj->uniqueID <= gObjectList->numObjects) {
                obj->name = gObjectList->object[obj->uniqueID - 1].name;
                GlobalObjects[i].objectName = obj->name;
            }
        }
    }
}

// 0x00462220 (PDB: 0x00463be0 / ?OBTABLE_GetObjectID@@YAHPAD@Z)
int OBTABLE_GetObjectID(char* name)
{
    if (gObjectList == nullptr || name == nullptr) {
        return 0;
    }
    for (int i = 0; i < gObjectList->numObjects; i++) {
        if (gObjectList->object[i].name != nullptr) {
            if (_stricmp(name, gObjectList->object[i].name) == 0) {
                return i + 1;
            }
        }
    }
    return 0;
}
