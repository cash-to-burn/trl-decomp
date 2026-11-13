#pragma once

#ifndef GAME_INSTANCE_H
#define GAME_INSTANCE_H

#include "common.h"
#include "cdc/runtime/cdcMath/Math.h"
#include "game/obtable.h"

// Forward declarations
struct Intro;
struct Instance;
struct HModel;
struct SoundInstanceData;
struct Body;
struct AnimatedGeoms;
struct AnimComponent;
struct CharacterProxy;
struct IntroData;
struct IndexedFace;
struct TerrainGroup;
struct CollideInfo;
struct Cloth;
struct Flammable;
struct ModObjState;
struct RagdollClient;
struct VMObject;
struct MultiSpline;
struct StreamUnit;

namespace cdc {
    class ISceneEntity;
}

// Doubly-linked list node
// 0x5aa4 in PDB / size: 0x8 (8 bytes)
struct NodeType {
    NodeType* prev;                         // 0x00
    NodeType* next;                         // 0x04
};
static_assert(sizeof(NodeType) == 0x8, "NodeType size mismatch");

// 0x5e35 in PDB / size: 0x100 (256 bytes)
struct BaseInstance {
    NodeType node;                          // 0x000
    Instance* next;                         // 0x008
    Instance* prev;                         // 0x00c
    cdc::Vector3 position;                  // 0x010
    cdc::Vector3 oldPos;                    // 0x020
    cdc::Euler rotation;                    // 0x030
    cdc::Euler oldRotation;                 // 0x040
    cdc::Vector3 scale;                     // 0x050
    cdc::Vector3 shadowPosition;            // 0x060
    cdc::Vector3 centerOfMass;              // 0x070
    cdc::Matrix* matrix;                    // 0x080
    cdc::Matrix* oldMatrix;                 // 0x084
    cdc::Matrix* matrixPool[2];             // 0x088
    signed char curMatrixPool;              // 0x090
    signed char rootMatrix;                 // 0x091
    unsigned char introNum;                 // 0x092
    unsigned char pad093;                   // 0x093
    Object* object;                         // 0x094
    Intro* intro;                           // 0x098
    HModel* hModelList;                     // 0x09c
    float distance_to_camera_sq;            // 0x0a0
    int iflags;                             // 0x0a4
    int iflags2;                            // 0x0a8
    int iflags3;                            // 0x0ac
    signed char currentRenderModel;         // 0x0b0
    signed char currentBaseModel;           // 0x0b1
    unsigned char currentStreamUnitID;      // 0x0b2
    unsigned char pad0b3;                   // 0x0b3
    SoundInstanceData* soundInstances;      // 0x0b4
    Instance* LinkParent;                   // 0x0b8
    int16_t ParentLinkNode;                 // 0x0bc
    int16_t linkMarkerIndex;                // 0x0be
    int vvRemoveDist;                       // 0x0c0
    int removeDist;                         // 0x0c4
    _CVECTOR* perVertexColor;               // 0x0c8
    float fadeValue;                        // 0x0cc
    signed char fadeType;                   // 0x0d0
    signed char burnType;                   // 0x0d1
    signed char reflectFlag;                // 0x0d2
    signed char instanceRed;                // 0x0d3
    signed char instanceGreen;              // 0x0d4
    signed char instanceBlue;               // 0x0d5
    signed char numGeometries;              // 0x0d6
    signed char fxMaterialMode;             // 0x0d7
    float drawgroupFadeValue;               // 0x0d8
    unsigned int noDrawGroups;              // 0x0dc
    unsigned int fxNoDrawGroups;            // 0x0e0
    Body* body;                             // 0x0e4
    AnimatedGeoms* animatedGeoms;           // 0x0e8
    int instanceID;                         // 0x0ec
    Instance* owner;                        // 0x0f0
    AnimComponent* animComponent;           // 0x0f4
    CharacterProxy* characterProxy;         // 0x0f8
    cdc::ISceneEntity* pEntity;             // 0x0fc
};
static_assert(sizeof(BaseInstance) == 0x100, "BaseInstance size mismatch");

// 0x5db7 in PDB / size: 0x260 (608 bytes)
struct Instance : public BaseInstance {
    cdc::Vector3 initialPos;                                // 0x100
    cdc::Matrix relativeMatrix;                             // 0x110
    cdc::Vector3 wNormal;                                   // 0x150
    cdc::Vector3 offset;                                    // 0x160
    cdc::Vector3 splitPoint;                                // 0x170
    cdc::Vector3 vel;                                       // 0x180
    cdc::Vector3 accl;                                      // 0x190
    cdc::Vector3 maxVel;                                    // 0x1a0
    cdc::Vector3 collideStartOverride;                      // 0x1b0
    void* data;                                             // 0x1c0
    IntroData* introData;                                   // 0x1c4
    Instance* parent;                                       // 0x1c8
    int birthStreamUnitID;                                  // 0x1cc
    int introUniqueID;                                      // 0x1d0
    float maxCheckDistance;                                 // 0x1d4
    unsigned int checkMask;                                 // 0x1d8
    float maxCheckeeDistance;                               // 0x1dc
    unsigned short currentTextureAnimFrame;                 // 0x1e0
    signed char collisionFlags;                             // 0x1e2
    signed char attachedSegment;                            // 0x1e3
    int attachedID;                                         // 0x1e4
    IndexedFace* face;                                      // 0x1e8
    IndexedFace* oldFace;                                   // 0x1ec
    TerrainGroup* faceTerrainGroup;                         // 0x1f0
    CollideInfo* collideInfo;                               // 0x1f4
    Instance* noCollideInstance;                            // 0x1f8
    IndexedFace* cachedFace;                                // 0x1fc
    TerrainGroup* cachedFaceTerrainGroup;                   // 0x200
    InstanceProcessFunc processFunc;                        // 0x204
    InstanceCollideFunc collideFunc;                        // 0x208
    InstanceAdditionalCollideFunc additionalCollideFunc;    // 0x20c
    InstanceQueryFunc queryFunc;                            // 0x210
    InstanceMessageFunc messageFunc;                        // 0x214
    IndexedFace* waterFace;                                 // 0x218
    PlaneConstants halvePlane;                              // 0x21c
    Instance* LinkChild;                                    // 0x228
    Instance* LinkSibling;                                  // 0x22c
    unsigned char splineFlags;                              // 0x230
    unsigned char pad231;                                   // 0x231
    unsigned char targetFrame;                              // 0x232
    unsigned char pad233;                                   // 0x233
    unsigned char clipBeg;                                  // 0x234
    unsigned char pad235;                                   // 0x235
    unsigned char clipEnd;                                  // 0x236
    unsigned char pad237;                                   // 0x237
    void* InstanceState;                                    // 0x238
    void* extraData;                                        // 0x23c
    unsigned char targetFade;                               // 0x240
    unsigned char pad241;                                   // 0x241
    unsigned char fadeFrames;                               // 0x242
    unsigned char pad243;                                   // 0x243
    float fadeEnd;                                          // 0x244
    float fadeStep;                                         // 0x248
    int clothCount;                                         // 0x24c
    Cloth* clothList;                                       // 0x250
    Flammable* flammableData;                               // 0x254
    ModObjState* modObjData;                                // 0x258
    RagdollClient* multibodyRagdoll;                        // 0x25c
};
static_assert(sizeof(Instance) == 0x260, "Instance size mismatch");

// 0x6f14 in PDB / size: 0x18 (24 bytes)
struct GenericIntroData {
    float InViewRemoveDist;                 // 0x00
    float OutOfViewRemoveDist;              // 0x04
    uint8_t UseModel;                       // 0x08
    uint8_t pad0;                           // 0x09
    uint16_t pad1;                          // 0x0a
    uint16_t Flags;                         // 0x0c
    int16_t AttachedInstance;               // 0x0e
    float SwingLength;                      // 0x10
    int DTPCameraID;                        // 0x14
};
static_assert(sizeof(GenericIntroData) == 0x18, "GenericIntroData size mismatch");

// 0x5f99 in PDB / size: 0x4 (4 bytes, followed inline by type-specific payload)
struct IntroData {
    int type;                               // 0x00

    GenericIntroData* AsGeneric() {
        return reinterpret_cast<GenericIntroData*>(this + 1);
    }
};
static_assert(sizeof(IntroData) == 0x4, "IntroData size mismatch");

// 0x5f67 in PDB / size: 0x70 (112 bytes)
struct Intro {
    cdc::Euler rotation;                    // 0x00
    cdc::Vector3 position;                  // 0x10
    cdc::Euler dummy1;                      // 0x20
    cdc::Vector3 dummy2;                    // 0x30
    cdc::Vector3 scale;                     // 0x40
    unsigned char objectID;                 // 0x50
    unsigned char pad051;                   // 0x51
    unsigned char intronum;                 // 0x52
    unsigned char pad053;                   // 0x53
    int UniqueID;                           // 0x54
    float maxRad;                           // 0x58
    unsigned int introFlags;                // 0x5c
    VMObject* attachedVMO;                  // 0x60
    IntroData* data;                        // 0x64
    MultiSpline* multiSpline;               // 0x68
    unsigned char startFrame;               // 0x6c
    unsigned char pad06d;                   // 0x6d
    unsigned char endFrame;                 // 0x6e
    unsigned char pad06f;                   // 0x6f
};
static_assert(sizeof(Intro) == 0x70, "Intro size mismatch");

// 0x82b7 in PDB / size: 0x4 (4 bytes)
struct SavedBasic {
    unsigned short savedID : 10;
    unsigned short shiftedSaveSize : 6;
    unsigned short streamUnitID;
};
static_assert(sizeof(SavedBasic) == 0x4, "SavedBasic size mismatch");

// 0x82bb in PDB / size: 0x2c (44 bytes)
struct SavedIntro : public SavedBasic {
    unsigned short objectID;                // 0x04
    unsigned char birthUnitID;              // 0x06
    unsigned char pad007;                   // 0x07
    unsigned char attachedUniqueID;         // 0x08
    unsigned char pad009;                   // 0x09
    unsigned char introUniqueID;            // 0x0a
    unsigned char pad00b;                   // 0x0b
    int flags;                              // 0x0c
    int flags2;                             // 0x10
    Vector3f position;                      // 0x14
    Vector3f rotation;                      // 0x20
};
static_assert(sizeof(SavedIntro) == 0x2c, "SavedIntro size mismatch");

// 0x6c67 in PDB / size: 0x114 (276 bytes)
struct InstanceList {
    int numInstances;                       // 0x000
    Instance* first;                        // 0x004
    int nextInstanceID;                     // 0x008
    Instance* first_free;                   // 0x00c
    NodeType group[32];                     // 0x010 (256 bytes)
    Instance* instance;                     // 0x110
};
static_assert(sizeof(InstanceList) == 0x114, "InstanceList size mismatch");

// Globals
extern InstanceList gInstanceList;                      // 0x010c59e0 (PDB: ?gInstanceList@@3UInstanceList@@A)
extern Instance gInstancePool[207];                     // 0x010c5e50 (PDB: ?gInstancePool@@3PAUInstance@@A)
extern short gInstanceIntroUniqueIDs[207];              // 0x010c5b00 (PDB: ?gInstanceIntroUniqueIDs@@3PAFA)
extern short gInstanceCurrentStreamUnitIDs[207];        // 0x010c5ca0 (PDB: ?gInstanceCurrentStreamUnitIDs@@3PAFA)

// Subsystem functions
// 0x004545b0 (PDB: 0x00455be0 / ?INSTANCE_InitInstanceList@@YAXXZ)
void INSTANCE_InitInstanceList(void);

// 0x00454670 (PDB: 0x00455ca0 / ?INSTANCE_NewInstance@@YAPAUInstance@@XZ)
Instance* INSTANCE_NewInstance(void);

// 0x004546d0 (PDB: 0x00455d10 / ?INSTANCE_InsertInstanceGroup@@YAXPAUInstance@@@Z)
void INSTANCE_InsertInstanceGroup(Instance* instance);

// 0x00455370 (PDB: 0x004569b0 / ?INSTANCE_FindIntroUniqueID@@YAPAUIntro@@HPAH@Z)
Intro* INSTANCE_FindIntroUniqueID(int uniqueID, int* outStreamUnitID);

// 0x004553f0 (PDB: 0x00456a30 / ?INSTANCE_Find@@YAPAUInstance@@H@Z)
Instance* INSTANCE_Find(int instanceID);

// 0x00455130 (PDB: 0x00456770 / ?INSTANCE_Query@@YAIPAUInstance@@I@Z)
unsigned int INSTANCE_Query(Instance* instance, unsigned int query);

// 0x00455bb0 (PDB: 0x004571f0 / ?INSTANCE_SetModel@@YAXPAUInstance@@H@Z)
void INSTANCE_SetModel(Instance* instance, int modelNum);

// 0x00455db0 (PDB: 0x00457430 / ?INSTANCE_SetCurrentStreamUnitID@@YAXPAUInstance@@H@Z)
void INSTANCE_SetCurrentStreamUnitID(Instance* instance, int streamUnitID);

// 0x00456770 (PDB: 0x00457df0 / ?INSTANCE_PlainDeath@@YAXPAUInstance@@@Z)
void INSTANCE_PlainDeath(Instance* instance);

// 0x004567c0 (PDB: 0x00457e40 / ?INSTANCE_KillInstance@@YAXPAUInstance@@@Z)
void INSTANCE_KillInstance(Instance* instance);

// 0x00457a20 (PDB: 0x00457c60 / ?INSTANCE_BirthCommon@@YAXPAUInstance@@PAUObject@@HH@Z)
void INSTANCE_BirthCommon(Instance* instance, Object* obj, int modelNum, int introNum);

// 0x004582a0 (PDB: 0x00459920 / ?INSTANCE_IntroduceInstance@@YAPAUInstance@@PAUIntro@@F_N@Z)
Instance* INSTANCE_IntroduceInstance(Intro* intro, short streamUnitID, bool flag);

// 0x00458600 (PDB: 0x00459c80 / ?INSTANCE_BirthObjectNoParent@@YAPAUInstance@@IPAVVector3@cdc@@PAVEuler@3@PAUIntroData@@PAUObject@@HH@Z)
Instance* INSTANCE_BirthObjectNoParent(unsigned int streamUnitID, cdc::Vector3* pos, cdc::Euler* rot, IntroData* introData, Object* obj, int modelNum, int flag);

// 0x00458750 (PDB: 0x00459dd0 / ?INSTANCE_BirthObject@@YAPAUInstance@@PAU1@PAUObject@@HH@Z)
Instance* INSTANCE_BirthObject(Instance* parent, Object* obj, int modelNum, int flag);

// 0x00458840 (PDB: 0x00459ec0 / ?INSTANCE_IntroduceSavedInstance@@YAPAUInstance@@PAUSavedIntro@@PAUStreamUnit@@PAH@Z)
Instance* INSTANCE_IntroduceSavedInstance(SavedIntro* savedIntro, StreamUnit* streamUnit, int* pOut);

#endif // GAME_INSTANCE_H
