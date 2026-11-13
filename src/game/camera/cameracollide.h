#pragma once

#ifndef GAME_CAMERA_CAMERACOLLIDE_H
#define GAME_CAMERA_CAMERACOLLIDE_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/camera/camera.h"
#include <cstddef>
#include <cstdint>

struct BaseInstance;
struct Instance;
struct Level;

// Size: 0x0a (10 bytes, TI: 0x668d)
struct IndexedFace {
    /* 0x000 */ uint16_t i0;
    /* 0x002 */ uint16_t i1;
    /* 0x004 */ uint16_t i2;
    /* 0x006 */ uint8_t adjacencyFlags;
    /* 0x007 */ uint8_t collisionFlags;
    /* 0x008 */ uint8_t clientFlags;
    /* 0x009 */ uint8_t materialType;
};
static_assert(sizeof(IndexedFace) == 0x0a, "IndexedFace size mismatch");

// Size: 0x30 (48 bytes, TI: 0x6418)
struct MTriangle {
    /* 0x000 */ cdc::Vector3 v0;
    /* 0x010 */ cdc::Vector3 v1;
    /* 0x020 */ cdc::Vector3 v2;
};
static_assert(sizeof(MTriangle) == 0x30, "MTriangle size mismatch");

// Size: 0x50 (80 bytes, TI: 0x6f46)
struct Mesh {
    /* 0x000 */ uint8_t pad000[0x20];
    /* 0x020 */ cdc::Vector3 m_position;
    /* 0x030 */ void* m_vertices;
    /* 0x034 */ IndexedFace* m_faces;
    /* 0x038 */ uint8_t pad038[8];
    /* 0x040 */ uint16_t m_vertexType;
    /* 0x042 */ uint8_t pad042[14];

    void GetTriangle(MTriangle* out, const IndexedFace* face); // 0x00424130
};
static_assert(offsetof(Mesh, m_position) == 0x20, "Mesh::m_position offset mismatch");
static_assert(offsetof(Mesh, m_faces) == 0x34, "Mesh::m_faces offset mismatch");
static_assert(sizeof(Mesh) == 0x50, "Mesh size mismatch");

// Size: 0xb0 (176 bytes, TI: 0x5e5e)
struct TerrainGroup {
    /* 0x000 */ uint8_t pad000[0x34];
    /* 0x034 */ Level* level;
    /* 0x038 */ Mesh* mesh;
    /* 0x03c */ uint8_t pad03c[0x74];
};
static_assert(offsetof(TerrainGroup, level) == 0x34, "TerrainGroup::level offset mismatch");
static_assert(offsetof(TerrainGroup, mesh) == 0x38, "TerrainGroup::mesh offset mismatch");
static_assert(sizeof(TerrainGroup) == 0xb0, "TerrainGroup size mismatch");

// Size: 0x60 (96 bytes, TI: 0x71df)
struct PCollideInfo {
    /* 0x000 */ cdc::Vector3 worldNormal;
    /* 0x010 */ int type;
    /* 0x014 */ int segment;
    /* 0x018 */ void* prim;
    /* 0x01c */ BaseInstance* inst;
    /* 0x020 */ void* geometry;
    /* 0x024 */ uint8_t pad024[12];
    /* 0x030 */ cdc::Vector3 startPoint;
    /* 0x040 */ cdc::Vector3 endPoint;
    /* 0x050 */ int collideType;
    /* 0x054 */ Instance* instance;
    /* 0x058 */ TerrainGroup* terrainGroup;
    /* 0x05c */ uint8_t pad05c[4];
};
static_assert(offsetof(PCollideInfo, startPoint) == 0x30, "PCollideInfo::startPoint offset mismatch");
static_assert(offsetof(PCollideInfo, terrainGroup) == 0x58, "PCollideInfo::terrainGroup offset mismatch");
static_assert(sizeof(PCollideInfo) == 0x60, "PCollideInfo size mismatch");

// Size: 0x90 (144 bytes, TI: 0xbb5c)
struct CameraCollisionInfo {
    /* 0x000 */ IndexedFace* tfaceList[5];
    /* 0x014 */ TerrainGroup* terrainGroups[6];
    /* 0x02c */ float lengthList[6];
    /* 0x044 */ int flags;
    /* 0x048 */ int line;
    /* 0x04c */ int numCollided;
    /* 0x050 */ float lenCenterToExtend;
    /* 0x054 */ uint8_t pad054[12];
    /* 0x060 */ cdc::Vector3 start;
    /* 0x070 */ cdc::Vector3 end;
    /* 0x080 */ int cldLines;
    /* 0x084 */ uint8_t pad084[12];
};
static_assert(offsetof(CameraCollisionInfo, start) == 0x60, "CameraCollisionInfo::start offset mismatch");
static_assert(offsetof(CameraCollisionInfo, cldLines) == 0x80, "CameraCollisionInfo::cldLines offset mismatch");
static_assert(sizeof(CameraCollisionInfo) == 0x90, "CameraCollisionInfo size mismatch");

// Globals defined in cameraCollide.cpp
extern int previousCollision;                  // 0x010fd970 (PDB: 0x01100b70)
extern cdc::Vector3 CameraLine_startPt[5];     // 0x010fd980 (PDB: 0x01100b80)
extern cdc::Vector3 CameraLine_endPt[5];       // 0x010fd9d0 (PDB: 0x01100bd0)

// Functions defined in cameraCollide.cpp
void CAMERA_update_dist_debounced(Camera* camera, float dist);                                                         // 0x00483c50 (PDB: 0x00485680)
float CAMERA_dampgetline(float target);                                                                                // 0x00483ec0 (PDB: 0x004858f0)
IndexedFace* CAMERA_SphereToSphereWithLines(Camera* camera, CameraCollisionInfo* colInfo, int secondcheck_flag, int* objectFlag); // 0x00484910 (PDB: 0x00486340)
int CAMERA_DoCameraCollision(Camera* camera, cdc::Vector3* targetCamPos, int simpleflag);                              // 0x004857b0 (PDB: 0x004871e0)

#endif // GAME_CAMERA_CAMERACOLLIDE_H
