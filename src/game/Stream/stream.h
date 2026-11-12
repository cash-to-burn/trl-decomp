#pragma once

#ifndef GAME_STREAM_STREAM_H
#define GAME_STREAM_STREAM_H

#include "common.h"
#include "cdc/runtime/cdcMath/Math.h"
#include <cstddef>
#include <cstdint>

namespace cdc {
struct AreaDBase;
}

namespace dtp {
struct UnitData;
struct ADMD;
}

// Forward declarations
struct Level;
struct ResolveObject;
struct CellGroup;
struct Terrain;
struct MarkUp;
struct CameraKey;
struct Intro;
struct Signal;
struct SplineCameraLevelData;
struct SFXMkr;
struct VramLink;
struct PlanData;
struct AttackWaveList;
struct AttackWaveGroupList;
struct CombatDoorsList;
struct TerrainLight;
struct SpecialMarkerList;
struct MoveData;

// 0x5e0d in PDB / size: 0x130 (304 bytes)
struct Level {
    /* 0x000 */ Terrain *terrain;
    /* 0x004 */ float waterZLevel;
    /* 0x008 */ uint8_t backColorR;
    /* 0x009 */ uint8_t backColorG;
    /* 0x00a */ uint8_t backColorB;
    /* 0x00b */ uint8_t cpad1;
    /* 0x00c */ uint8_t spectralColorR;
    /* 0x00d */ uint8_t spectralColorG;
    /* 0x00e */ uint8_t spectralColorB;
    /* 0x00f */ uint8_t spectralFXAlways;
    /* 0x010 */ uint8_t waterColorR;
    /* 0x011 */ uint8_t waterColorG;
    /* 0x012 */ uint8_t waterColorB;
    /* 0x013 */ int8_t waterBlend;
    /* 0x014 */ float farPlane;
    /* 0x018 */ float fogFar;
    /* 0x01c */ float fogNear;
    /* 0x020 */ float spectralFarPlane;
    /* 0x024 */ float spectralFogFar;
    /* 0x028 */ float spectralFogNear;
    /* 0x02c */ float waterFarPlane;
    /* 0x030 */ float waterFogFar;
    /* 0x034 */ float waterFogNear;
    /* 0x038 */ int UnderwaterFXAlpha;
    /* 0x03c */ float UnderwaterFXMovement;
    /* 0x040 */ float UnderwaterFXSpeed;
    /* 0x044 */ int SpectralFXAlpha;
    /* 0x048 */ float SpectralFXIncrease;
    /* 0x04c */ float SpectralFXCenter;
    /* 0x050 */ int WaterFXAlpha;
    /* 0x054 */ float WaterFXMovement;
    /* 0x058 */ float WaterFXSpeed;
    /* 0x05c */ int WaterFSFX;
    /* 0x060 */ int numMarkUps;
    /* 0x064 */ MarkUp *markupList;
    /* 0x068 */ int numCameras;
    /* 0x06c */ CameraKey *cameraList;
    /* 0x070 */ int flags;
    /* 0x074 */ int numIntros;
    /* 0x078 */ Intro *introList;
    /* 0x07c */ void *objectNameList;
    /* 0x080 */ char *worldName;
    /* 0x084 */ Signal *startGoingIntoWaterSignal;
    /* 0x088 */ Signal *startGoingOutOfWaterSignal;
    /* 0x08c */ int unitFlags;
    /* 0x090 */ Signal *SignalListStart;
    /* 0x094 */ void *SignalIDList;
    /* 0x098 */ SplineCameraLevelData *splineCameraData;
    /* 0x09c */ void *relocModule;
    /* 0x0a0 */ int NumberOfSFXMarkers;
    /* 0x0a4 */ SFXMkr *SFXMarkerList;
    /* 0x0a8 */ unsigned int versionNumber;
    /* 0x0ac */ unsigned int guiID;
    /* 0x0b0 */ char *dynamicMusicName;
    /* 0x0b4 */ int streamUnitID;
    /* 0x0b8 */ VramLink *textureLoadList;
    /* 0x0bc */ PlanData *planData;
    /* 0x0c0 */ unsigned int mapRegion;
    /* 0x0c4 */ void *WeatherHeightmapData;
    /* 0x0c8 */ AttackWaveList *attackWaveList;
    /* 0x0cc */ AttackWaveGroupList *attackWaveGroupList;
    /* 0x0d0 */ CombatDoorsList *combatDoorsList;
    /* 0x0d4 */ int numTerrainLights;
    /* 0x0d8 */ TerrainLight *terrainLights;
    /* 0x0dc */ void *terrainLightGrids;
    /* 0x0e0 */ SpecialMarkerList *vmarkerList;
    /* 0x0e4 */ SpecialMarkerList *pmarkerList;
    /* 0x0e8 */ char *playerName;
    /* 0x0ec */ unsigned int levelCount;
    /* 0x0f0 */ MoveData *moveData;
    /* 0x0f4 */ void *pCdcPlannerData;
    /* 0x0f8 */ cdc::AreaDBase *pAreaDBase;
    /* 0x0fc */ dtp::UnitData *pUnitData;
    /* 0x100 */ dtp::ADMD *pADMDData;
    /* 0x110 */ cdc::Vector3 sceneCenterOffset;
    /* 0x120 */ uint8_t playerObjectID;
    /* 0x121 */ uint8_t pad121[15];
};
static_assert(offsetof(Level, splineCameraData) == 0x098, "Level::splineCameraData offset mismatch");
static_assert(sizeof(Level) == 0x130, "Level size mismatch");

// 0x001d5700 in PDB / 0x005c bytes (92 bytes)
// Extracted directly from tr7.pdb LF_STRUCTURE "StreamUnit" (type 0x6d1f)
struct StreamUnit {
    int StreamUnitID;              // 0x00: unique stream unit identifier
    uint8_t used;                  // 0x04: 0 = unused, 1 = loading, 2 = active/loaded, 4 = dumped/dumping
    uint8_t unitHidden;            // 0x05: unit visibility state
    int16_t unitFlags;             // 0x06: flags (bit 0 = persistent / don't dump)
    Level *level;                  // 0x08: pointer to parsed level structure
    char baseAreaName[20];         // 0x0c: name of the stream unit / DRM file without extension
    ResolveObject *resolveObj;     // 0x20: handle to DRM resolve object
    void *loadData;                // 0x24: raw file buffer or unit data header
    float fogNear;                 // 0x28: distance to start of fog
    float fogFar;                  // 0x2c: distance to end of fog
    float farPlane;                // 0x30: camera far clipping plane
    uint8_t fogR;                  // 0x34: fog color red channel
    uint8_t fogG;                  // 0x35: fog color green channel
    uint8_t fogB;                  // 0x36: fog color blue channel
    uint8_t fogOverrideFlag;       // 0x37: fog color override flag
    uint32_t FrameCount;           // 0x38: frame counter when this unit was last marked needed
    int16_t cliprect[4];           // 0x3c: scissor / clipping rectangle (left, top, right, bottom)
    int draw;                      // 0x44: draw state
    int drawtime;                  // 0x48: draw timestamp
    void (*EventMain)(void);       // 0x4c: unit event dispatch hook
    void (*CallTrigger)(void);     // 0x50: trigger callback hook
    bool (*IsTriggerActive)(void); // 0x54: trigger activity query hook
    CellGroup *pCellGroup;         // 0x58: portal and spatial occlusion cell group
};

struct StreamObjectEntry {
    int id;
    int offset;
    int size;
    int flags;
    const char *name;
};

struct StreamObjectList {
    int numObjects;
    StreamObjectEntry objects[1];
};

#define MAX_STREAM_UNITS 8

// Global unit array at 0x01158238 (PDB seg 3 off 0x1b1668)
extern StreamUnit StreamUnitList[MAX_STREAM_UNITS];
extern int NumStreamUnits;
extern StreamObjectList *g_streamObjList; // 0x01159254

#ifdef __cplusplus
extern "C" {
#endif

// 0x005d3c40 (PDB: 0x005d6700 / ?STREAM_Init@@YAXXZ)
// Initializes the StreamUnitList pool and loads \TR7\%s\objlist.dat
void STREAM_Init(void);

// 0x005d3bc0 (PDB: 0x005d6680 / ?STREAM_AbortAreaLoad@@YAXXZ)
// Aborts any pending stream unit load
void STREAM_AbortAreaLoad(void);

// 0x005d4400 (PDB: 0x005d6ec0 / ?STREAM_GetStreamUnitWithID@@YAPAUStreamUnit@@H@Z)
// Retrieves active StreamUnit matching streamUnitID
StreamUnit *STREAM_GetStreamUnitWithID(int streamUnitID);

// 0x005d4440 (PDB: 0x005d6f00 / ?STREAM_GetStreamUnitWithName@@YAPAUStreamUnit@@PBD@Z)
// Retrieves active StreamUnit matching baseAreaName (case-insensitive)
StreamUnit *STREAM_GetStreamUnitWithName(const char *name);

// 0x005d43c0 (PDB: 0x005d6e80 / ?STREAM_GetLevelWithID@@YAPAULevel@@H@Z)
// Retrieves Level pointer for active unit with ID
Level *STREAM_GetLevelWithID(int streamUnitID);

// 0x005d48f0 (PDB: 0x005d73b0 / ?STREAM_MarkUnitNeeded@@YAXH@Z)
// Updates unit's FrameCount to current game frame
void STREAM_MarkUnitNeeded(int streamID);

// 0x005d4930 (PDB: 0x005d73f0 / ?STREAM_DumpUnit@@YAXPAUStreamUnit@@H@Z)
// Unlinks and releases a loaded stream unit
void STREAM_DumpUnit(StreamUnit *unit, int param2);

// 0x005d4ac0 (PDB: 0x005d7580 / ?STREAM_DumpAllUnitsNotNeeded@@YAXXZ)
// Scans and dumps all active units whose FrameCount does not match current frame
void STREAM_DumpAllUnitsNotNeeded(void);

// 0x005d4b00 (PDB: 0x005d75c0 / ?STREAM_DumpAllLevels@@YAXHH@Z)
// Dumps all stream units except keepStreamUnitID
void STREAM_DumpAllLevels(int keepStreamUnitID, int param2);

// 0x005d4d50 (PDB: 0x005d7810 / ?STREAM_PollLoadQueue@@YAHXZ)
// Polls asynchronous file system requests; returns 1 if transfer completed
int STREAM_PollLoadQueue(void);

// 0x005d5720 (PDB: 0x005d81e0 / ?STREAM_LoadLevel@@YAPAUStreamUnit@@PBDPAUStreamPortal@@H@Z)
// Queues asynchronous loading of baseAreaName DRM file
StreamUnit *STREAM_LoadLevel(const char *baseAreaName, void *streamPortal, int streamID);

// 0x005d5940 (PDB: 0x005d8400 / ?STREAM_WaitForUnit@@YAXPAUStreamUnit@@@Z)
// Synchronously blocks until DRM resolve object is fully loaded
void STREAM_WaitForUnit(StreamUnit *unit);

#ifdef __cplusplus
}
#endif

#endif // GAME_STREAM_STREAM_H
