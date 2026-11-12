#include "game/Stream/stream.h"
#include "game/main.h"
#include "game/pc/pclocals.h"
#include "game/archive/ArchiveFileSystem.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
static inline void Sleep(uint32_t ms) { usleep(ms * 1000); }
#endif

// Global stream unit pool at 0x01158238 (PDB VA: 0x010b1668)
StreamUnit StreamUnitList[MAX_STREAM_UNITS];
int NumStreamUnits = MAX_STREAM_UNITS;
StreamObjectList *g_streamObjList = nullptr; // 0x01159254

// 0x005d3c40 (PDB: 0x005d6700)
void STREAM_Init(void)
{
    // If already initialized, nothing to do
    if (g_streamObjList != nullptr) {
        return;
    }

    // 1. Initialize all 8 stream unit entries
    for (int i = 0; i < MAX_STREAM_UNITS; i++) {
        StreamUnit *unit = &StreamUnitList[i];
        unit->StreamUnitID = 0;
        unit->used = 0;
        unit->unitHidden = 0;
        unit->unitFlags = 0;
        unit->level = nullptr;
        unit->baseAreaName[0] = '\0';
        unit->resolveObj = nullptr;
        unit->loadData = nullptr;
        unit->fogNear = 0.0f;
        unit->fogFar = 0.0f;
        unit->farPlane = 0.0f;
        unit->fogR = 0;
        unit->fogG = 0;
        unit->fogB = 0;
        unit->fogOverrideFlag = 0;
        unit->FrameCount = 0;
        unit->cliprect[0] = 0;
        unit->cliprect[1] = 0;
        unit->cliprect[2] = 0;
        unit->cliprect[3] = 0;
        unit->draw = 0;
        unit->drawtime = 0;
        unit->EventMain = nullptr;
        unit->CallTrigger = nullptr;
        unit->IsTriggerActive = nullptr;
        unit->pCellGroup = nullptr;
    }

    // 2. Format path to global object list: \TR7\%s\objlist.dat
    char objListPath[260];
    snprintf(objListPath, sizeof(objListPath), "\\TR7\\%s\\objlist.dat", g_buildDir);

    // 3. Check archive filesystem for objlist.dat
    if (g_pArchiveFS != nullptr && g_pArchiveFS->FileExists(objListPath)) {
        void *pData = FSHelper_ReadFile(objListPath, 0, g_pArchiveFS);
        if (pData != nullptr) {
            g_streamObjList = (StreamObjectList *)pData;
            int numObj = *(int *)pData;
            char *pNames = (char *)pData + 4 + (numObj * 20);
            StreamObjectEntry *entries = (StreamObjectEntry *)((char *)pData + 4);

            for (int i = 0; i < numObj; i++) {
                if (entries[i].id != 0) {
                    entries[i].name = pNames + 2;
                    int16_t nameLen = *(int16_t *)pNames;
                    pNames += 2 + (nameLen * 2);
                }
            }
            return;
        }
    }

    // Default allocation fallback: empty list
    g_streamObjList = (StreamObjectList *)malloc(sizeof(int));
    if (g_streamObjList != nullptr) {
        g_streamObjList->numObjects = 0;
    }
}

// 0x005d3bc0 (PDB: 0x005d6680)
void STREAM_AbortAreaLoad(void)
{
    for (int i = 0; i < MAX_STREAM_UNITS; i++) {
        StreamUnit *unit = &StreamUnitList[i];
        if (unit->used == 1) { // Currently loading
            unit->used = 0;
            unit->resolveObj = nullptr;
            if (unit->loadData != nullptr) {
                free(unit->loadData);
                unit->loadData = nullptr;
            }
        }
    }
}

// 0x005d4400 (PDB: 0x005d6ec0)
StreamUnit *STREAM_GetStreamUnitWithID(int streamUnitID)
{
    for (int i = 0; i < MAX_STREAM_UNITS; i++) {
        StreamUnit *unit = &StreamUnitList[i];
        if (unit->used == 2 && unit->StreamUnitID == streamUnitID) {
            return unit;
        }
    }
    return nullptr;
}

// 0x005d4440 (PDB: 0x005d6f00)
StreamUnit *STREAM_GetStreamUnitWithName(const char *name)
{
    if (name == nullptr) {
        return nullptr;
    }

    for (int i = 0; i < MAX_STREAM_UNITS; i++) {
        StreamUnit *unit = &StreamUnitList[i];
        if (unit->used != 0 && unit->used != 4) {
            if (_stricmp(name, unit->baseAreaName) == 0) {
                return unit;
            }
        }
    }
    return nullptr;
}

// 0x005d43c0 (PDB: 0x005d6e80)
Level *STREAM_GetLevelWithID(int streamUnitID)
{
    for (int i = 0; i < MAX_STREAM_UNITS; i++) {
        StreamUnit *unit = &StreamUnitList[i];
        if (unit->used == 2 && unit->StreamUnitID == streamUnitID) {
            return unit->level;
        }
    }
    return nullptr;
}

// 0x005d48f0 (PDB: 0x005d73b0)
void STREAM_MarkUnitNeeded(int streamID)
{
    for (int i = 0; i < MAX_STREAM_UNITS; i++) {
        StreamUnit *unit = &StreamUnitList[i];
        if (unit->used != 0 && unit->used != 4 && unit->StreamUnitID == streamID) {
            unit->FrameCount = g_gameTracker.frameCount;
            return;
        }
    }
}

// 0x005d4930 (PDB: 0x005d73f0)
void STREAM_DumpUnit(StreamUnit *unit, int param2)
{
    (void)param2;
    if (unit == nullptr) {
        return;
    }

    // 1. If unit is still loading, cancel the load
    if (unit->used == 1) {
        unit->used = 0;
        unit->unitHidden = 0;
        unit->unitFlags = 0;
        unit->resolveObj = nullptr;
        if (unit->loadData != nullptr) {
            free(unit->loadData);
            unit->loadData = nullptr;
        }
        return;
    }

    // 2. Mark unit as dumping / dumped
    unit->used = 4;
    unit->unitHidden = 1;
    unit->resolveObj = nullptr;
}

// 0x005d4ac0 (PDB: 0x005d7580)
void STREAM_DumpAllUnitsNotNeeded(void)
{
    for (int i = 0; i < MAX_STREAM_UNITS; i++) {
        StreamUnit *unit = &StreamUnitList[i];
        if (unit->used != 0 && unit->used != 4) {
            if (unit->FrameCount != g_gameTracker.frameCount) {
                // If bit 0 of unitFlags is not set (not persistent)
                if ((unit->unitFlags & 1) == 0) {
                    STREAM_DumpUnit(unit, 1);
                }
            }
        }
    }
}

// 0x005d4b00 (PDB: 0x005d75c0)
void STREAM_DumpAllLevels(int keepStreamUnitID, int param2)
{
    for (int i = 0; i < MAX_STREAM_UNITS; i++) {
        StreamUnit *unit = &StreamUnitList[i];
        if (unit->used != 0 && unit->used != 4 && unit->StreamUnitID != keepStreamUnitID) {
            STREAM_DumpUnit(unit, param2);
        }
    }
    g_gameTracker.mainDrawUnit = nullptr;
}

// 0x005d4d50 (PDB: 0x005d7810)
int STREAM_PollLoadQueue(void)
{
    if (g_pArchiveFS == nullptr) {
        return 0;
    }

    // Polls pending file reads in the archive system
    int res1 = g_pArchiveFS->Poll();
    g_pArchiveFS->ProcessRequests();
    if (res1 == 1) {
        return 1;
    }

    int res2 = g_pArchiveFS->Poll();
    if (res2 == 1) {
        return 1;
    }

    return 0;
}

// 0x005d5720 (PDB: 0x005d81e0)
StreamUnit *STREAM_LoadLevel(const char *baseAreaName, void *streamPortal, int streamID)
{
    (void)streamPortal;
    if (baseAreaName == nullptr) {
        return nullptr;
    }

    // 1. Check if unit is already present
    for (int i = 0; i < MAX_STREAM_UNITS; i++) {
        StreamUnit *unit = &StreamUnitList[i];
        if (unit->used != 0 && unit->used != 4) {
            if (_stricmp(unit->baseAreaName, baseAreaName) == 0) {
                unit->FrameCount = g_gameTracker.frameCount;
                return unit;
            }
        }
    }

    // 2. Find an empty or dumped unit slot
    StreamUnit *targetUnit = nullptr;
    for (int i = 0; i < MAX_STREAM_UNITS; i++) {
        StreamUnit *unit = &StreamUnitList[i];
        if (unit->used == 0 || unit->used == 4) {
            targetUnit = unit;
            break;
        }
    }

    if (targetUnit == nullptr) {
        return nullptr;
    }

    // 3. Populate new unit slot
    targetUnit->StreamUnitID = streamID;
    targetUnit->used = 2; // Active
    targetUnit->unitHidden = 0;
    targetUnit->unitFlags = 0;
    strncpy(targetUnit->baseAreaName, baseAreaName, sizeof(targetUnit->baseAreaName) - 1);
    targetUnit->baseAreaName[sizeof(targetUnit->baseAreaName) - 1] = '\0';
    targetUnit->FrameCount = g_gameTracker.frameCount;

    // 4. Update GameTracker current area
    strncpy(g_gameTracker.baseAreaName, baseAreaName, sizeof(g_gameTracker.baseAreaName) - 1);
    g_gameTracker.StreamUnitID = streamID;

    return targetUnit;
}

// 0x005d5940 (PDB: 0x005d8400)
void STREAM_WaitForUnit(StreamUnit *unit)
{
    if (unit == nullptr) {
        return;
    }

    while (unit->used == 1) {
        STREAM_PollLoadQueue();
        Sleep(5);
    }
}
