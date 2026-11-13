#pragma once

#ifndef GAME_COLLIDE_H
#define GAME_COLLIDE_H

#include "common.h"
#include "cdc/runtime/cdcMath/Math.h"
#include "game/camera/cameracollide.h"
#include "game/camera/cameraspline.h"
#include "game/instance.h"
#include "game/Stream/stream.h"
#include <cstddef>
#include <cstdint>

struct AttackWaveRuntime;
struct BGInstance;
struct BGObject;
struct Camera;
struct Contact;
struct HBox;
struct HCapsule;
struct HSphere;
struct Intro;
struct TerrainAnimTexture;
struct TerrainRenderVertex;
struct TerrainVMORenderVertex;
struct VMObject;
struct VMOffsetTable;

namespace cdc {
// 0x00423ff0 (PDB: 0x00423060 / ??Lcdc@@YA?AVVector3@0@ABV10@0@Z)
Vector3 operator%(const Vector3& a, const Vector3& b);
} // namespace cdc

// Size: 0x06 (6 bytes, TI: 0x68db)
struct VertexInt16 {
    /* 0x000 */ int16_t x;
    /* 0x002 */ int16_t y;
    /* 0x004 */ int16_t z;

    operator cdc::Vector3(); // 0x004240b0 (PDB: 0x00423120)
};
static_assert(sizeof(VertexInt16) == 0x06, "VertexInt16 size mismatch");

// Size: 0x08 (8 bytes, TI: 0x6ef7)
struct SegmentResult {
    /* 0x000 */ IndexedFace* face;
    /* 0x004 */ float lambda;
};
static_assert(sizeof(SegmentResult) == 0x08, "SegmentResult size mismatch");

// Size: 0x0c (12 bytes, TI: 0xc14c)
struct SignalResults {
    /* 0x000 */ float lambda;
    /* 0x004 */ Signal* signal;
    /* 0x008 */ Level* level;
};
static_assert(sizeof(SignalResults) == 0x0c, "SignalResults size mismatch");

// Size: 0x20 (32 bytes, TI: 0x59ae)
struct HMarker {
    /* 0x000 */ int segment;
    /* 0x004 */ int index;
    /* 0x008 */ float px;
    /* 0x00c */ float py;
    /* 0x010 */ float pz;
    /* 0x014 */ float rx;
    /* 0x018 */ float ry;
    /* 0x01c */ float rz;
};
static_assert(sizeof(HMarker) == 0x20, "HMarker size mismatch");

// Size: 0x08 (8 bytes, TI: 0x5f61)
struct HPrim {
    /* 0x000 */ uint8_t hpFlags;
    /* 0x001 */ uint8_t withFlags;
    /* 0x002 */ uint8_t type;
    /* 0x003 */ uint8_t segment;
    /* 0x004 */ union {
        HSphere* hsphere;
        HBox* hbox;
        HMarker* hmarker;
        HCapsule* hcapsule;
    } data;
};
static_assert(sizeof(HPrim) == 0x08, "HPrim size mismatch");

// Size: 0x10 (16 bytes, TI: 0x5fad)
struct HModel {
    /* 0x000 */ HPrim* hPrimList;
    /* 0x004 */ int numHPrims;
    /* 0x008 */ HMarker* dynamicHMarkerList;
    /* 0x00c */ int numDynamicHMarkers;
};
static_assert(sizeof(HModel) == 0x10, "HModel size mismatch");

// Size: 0x50 (80 bytes, TI: 0x6ac5)
struct CollideInfo {
    /* 0x000 */ cdc::Vector3 point0f;
    /* 0x0010 */ cdc::Vector3 point1f;
    /* 0x0020 */ cdc::Vector3 offsetf;
    /* 0x0030 */ uint8_t flags;
    /* 0x0031 */ int8_t segment;
    /* 0x0032 */ int8_t type0;
    /* 0x0033 */ int8_t type1;
    /* 0x0034 */ void* prim0;
    /* 0x0038 */ void* prim1;
    /* 0x003c */ BaseInstance* inst0;
    /* 0x0040 */ BaseInstance* inst1;
    /* 0x0044 */ const Contact* contact;
    /* 0x0048 */ TerrainGroup* terrainGroup;
    /* 0x004c */ uint8_t pad04c[4];
};
static_assert(sizeof(CollideInfo) == 0x50, "CollideInfo size mismatch");

// Size: 0xd0 (208 bytes, TI: 0x6e81)
struct Signal {
    /* 0x000 */ uint32_t flags; // triggered = 0x8000 (low 16 bits hold signal bitfields)
    /* 0x004 */ SplineCameraLevelData* splineCamDataPtr[3];
    /* 0x010 */ int CameraLock;
    /* 0x014 */ int CameraUnlock;
    /* 0x018 */ int16_t CameraSave;
    /* 0x01a */ int16_t CameraRestore;
    /* 0x01c */ uint16_t CameraShakeScale;
    /* 0x01e */ int16_t CameraShakeTime;
    /* 0x020 */ SplineCameraLevelData splineCamParams;
    /* 0x078 */ char streamName[20];
    /* 0x08c */ int streamPortalIndex;
    /* 0x090 */ int SetSlideAngle;
    /* 0x094 */ int ReverbType;
    /* 0x098 */ int ReverbVolume;
    /* 0x09c */ int setMusicVar;
    /* 0x0a0 */ int setMusicVarValue;
    /* 0x0a4 */ int fatalType;
    /* 0x0a8 */ AttackWaveRuntime* enableAttackWaves;
    /* 0x0ac */ AttackWaveRuntime* disableAttackWaves;
    /* 0x0b0 */ AttackWaveRuntime* killAttackWaves;
    /* 0x0b4 */ int8_t CameraStackType;
    /* 0x0b5 */ uint8_t CameraStackFlags;
    /* 0x0b6 */ uint8_t CameraStackSignalFlags[2];
    /* 0x0b8 */ int DTPCameraID[2];
    /* 0x0c0 */ Signal* CameraLinkSignal;
    /* 0x0c4 */ int FSFXInDTPID;
    /* 0x0c8 */ int FSFXExitDTPID;
    /* 0x0cc */ uint16_t FSFXInActiveDist;
    /* 0x0ce */ uint16_t FSFXExitActiveDist;
};
static_assert(sizeof(Signal) == 0xd0, "Signal size mismatch");
static_assert(offsetof(Signal, streamPortalIndex) == 0x8c, "Signal::streamPortalIndex offset mismatch");

// Size: 0xa0 (160 bytes, TI: 0x6c6d)
struct StreamUnitPortal {
    /* 0x000 */ char tolevelname[30];
    /* 0x01e */ int16_t toSignalID;
    /* 0x020 */ int16_t MSignalID;
    /* 0x022 */ int16_t streamID;
    /* 0x024 */ uint16_t* closeVertList;
    /* 0x028 */ float activeDistance;
    /* 0x02c */ StreamUnit* toStreamUnit;
    /* 0x030 */ cdc::Vector3 min;
    /* 0x040 */ cdc::Vector3 max;
    /* 0x050 */ cdc::Vector3 quad[4];
    /* 0x090 */ cdc::Vector3 normal;
};
static_assert(sizeof(StreamUnitPortal) == 0xa0, "StreamUnitPortal size mismatch");
static_assert(offsetof(StreamUnitPortal, streamID) == 0x22, "StreamUnitPortal::streamID offset mismatch");

// Size: 0x68 (104 bytes, TI: 0x5d45)
struct Terrain {
    /* 0x000 */ int16_t UnitChangeFlags;
    /* 0x002 */ int16_t spad;
    /* 0x004 */ int numIntros;
    /* 0x008 */ Intro* introList;
    /* 0x00c */ int numStreamUnitPortals;
    /* 0x010 */ StreamUnitPortal* streamUnitPortals;
    /* 0x014 */ int numTerrainGroups;
    /* 0x018 */ TerrainGroup* terrainGroups;
    /* 0x01c */ TerrainGroup* signalTerrainGroup;
    /* 0x020 */ Signal* signals;
    /* 0x024 */ TerrainAnimTexture* terrainAnimTextures;
    /* 0x028 */ int numBGInstances;
    /* 0x02c */ BGInstance* BGInstanceList;
    /* 0x030 */ int numBGObjects;
    /* 0x034 */ BGObject* BGObjectList;
    /* 0x038 */ int numVMObjects;
    /* 0x03c */ VMObject* vmobjectList;
    /* 0x040 */ VMOffsetTable* vmTableList;
    /* 0x044 */ TerrainRenderVertex* XboxPcVertexBuffer;
    /* 0x048 */ TerrainVMORenderVertex* XboxPcVmoBuffer;
    /* 0x04c */ void* XboxPcPortalAdjustVerts;
    /* 0x050 */ void* d3dvertexBuffer;
    /* 0x054 */ unsigned int numTerrainVertices;
    /* 0x058 */ void* d3dvmoVertexBuffer;
    /* 0x05c */ unsigned int numTerrainVMOVertices;
    /* 0x060 */ unsigned int cdcRenderDataID;
    /* 0x064 */ void* cdcRenderTerrainData;
};
static_assert(sizeof(Terrain) == 0x68, "Terrain size mismatch");

// Collision subsystem routines (Original: game/collide.c / collide.cpp)
void COLLIDE_SegmentCollisionOn(Instance* instance, int segment);                                                                // 0x00424280 (PDB: 0x004232f0)
void COLLIDE_SegmentCollisionOff(Instance* instance, int segment);                                                               // 0x004242f0 (PDB: 0x00423360)
void COLLIDE_ResetCollisionFlag(Instance* instance, int hpMask, int withMask);                                                   // 0x00424370 (PDB: 0x004233e0)
void COLLIDE_SetTerrainGroupFlag(CollideInfo* collideInfo, int flag);                                                            // 0x004243b0 (PDB: 0x00423420)
int COLLIDE_GetNextDynamicMarker(int* outIndex, BaseInstance* instance);                                                         // 0x004243d0 (PDB: 0x00423440)
int COLLIDE_GetNextDynamicMarkers(int* outStartIndex, int* outIDs, int count, BaseInstance* instance);                           // 0x00424520 (PDB: 0x00423590)
int COLLIDE_InsertDynamicMarker(BaseInstance* instance, HMarker* marker);                                                        // 0x00424680 (PDB: 0x004236f0)
int COLLIDE_InsertDynamicWorldPosMarker(BaseInstance* instance, const cdc::Vector3& pos);                                        // 0x00424710 (PDB: 0x00423780)
int COLLIDE_InsertDynamicWorldPosMarkers(int* outIDs, int count, BaseInstance* instance, const cdc::Vector3& pos);               // 0x004247a0 (PDB: 0x00423810)
bool COLLIDE_SetDynamicWorldPosMarker(BaseInstance* instance, int markerID, const cdc::Vector3& pos);                            // 0x004249c0 (PDB: 0x00423a30)
void COLLIDE_RelocateDynamicWorldPosMarkers(BaseInstance* instance, const cdc::Vector3& delta);                                  // 0x00424a30 (PDB: 0x00423aa0)
int COLLIDE_DeleteDynamicMarker(BaseInstance* instance, int markerID);                                                           // 0x00424ab0 (PDB: 0x00423b20)
void COLLIDE_DeleteDynamicMarkerList(BaseInstance* instance);                                                                    // 0x00424bb0 (PDB: 0x00423c20)
int COLLIDE_Signals(SignalResults* results, const cdc::Vector3& start, const cdc::Vector3& end);                                 // 0x00424c20 (PDB: 0x00423c90)
int COLLIDE_FindCollisionFaceNormal(CollideInfo* collideInfo, cdc::Vector3* normal);                                             // 0x00424e30 (PDB: 0x00423ea0)
StreamUnit* COLLIDE_CameraWithStreamSignals(Camera* camera);                                                                     // 0x00424ec0 (PDB: 0x00423f30)
void COLLIDE_InstanceListWithSignals(void);                                                                                      // 0x00425060 (PDB: 0x004240d0)
void COLLIDE_UpdateAllTransformsf(BaseInstance* instance, const cdc::Vector3& offset);                                           // 0x004252b0 (PDB: 0x00424320)
void COLLIDE_MoveAllTransformsf(BaseInstance* instance, const cdc::Vector3& offset);                                             // 0x00425390 (PDB: 0x00424400)
int COLLIDE_GetHMarkerTransformFromID(BaseInstance** outInst, BaseInstance* instance, int markerID, cdc::Matrix* mat, bool useOldMatrix); // 0x00425450 (PDB: 0x004244c0)
int COLLIDE_GetHMarkerTransformFromID(BaseInstance* instance, int markerID, cdc::Matrix* mat, bool useOldMatrix);                // 0x00425720 (PDB: 0x00424790)

#endif // GAME_COLLIDE_H
