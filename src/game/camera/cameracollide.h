#pragma once

#ifndef GAME_CAMERA_CAMERACOLLIDE_H
#define GAME_CAMERA_CAMERACOLLIDE_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/camera/camera.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

struct BaseInstance;
struct Instance;
struct Level;

// Size: 0x0a (10 bytes, TI: 0x668d)
struct SegmentResult;

struct IndexedFace {
    /* 0x000 */ uint16_t i0;
    /* 0x002 */ uint16_t i1;
    /* 0x004 */ uint16_t i2;
    /* 0x006 */ uint8_t adjacencyFlags;
    /* 0x007 */ uint8_t collisionFlags;
    /* 0x008 */ uint8_t clientFlags;
    /* 0x009 */ uint8_t materialType;

    uint16_t GetSignalIndex() const
    {
        uint16_t idx = 0;
        std::memcpy(&idx, &clientFlags, sizeof(idx));
        return idx;
    }
};
static_assert(sizeof(IndexedFace) == 0x0a, "IndexedFace size mismatch");

// Size: 0x30 (48 bytes, TI: 0x6418)
struct MTriangle {
    /* 0x000 */ cdc::Vector3 v0;
    /* 0x010 */ cdc::Vector3 v1;
    /* 0x020 */ cdc::Vector3 v2;

    cdc::Vector3 GetNormal(); // 0x00424040 (PDB: 0x004230b0)
};
static_assert(sizeof(MTriangle) == 0x30, "MTriangle size mismatch");

// Size: 0x20 (32 bytes, TI: 0x6e15)
struct BBox {
    /* 0x000 */ cdc::Vector3 bMin;
    /* 0x010 */ cdc::Vector3 bMax;
};
static_assert(sizeof(BBox) == 0x20, "BBox size mismatch");

// Size: 0x0c (12 bytes, TI: 0x6f0e)
struct KDNode {
    /* 0x000 */ float m_negOffset;
    /* 0x004 */ float m_posOffset;
    /* 0x008 */ uint16_t m_index;
    /* 0x00a */ uint8_t m_axis;
    /* 0x00b */ uint8_t m_numFaces;

    uint8_t GetAxis() const { return m_axis; }
    bool IsLeaf() const { return m_numFaces != 0; }
    uint8_t GetNumFaces() const { return m_numFaces; }
    uint16_t GetFaceIndex() const { return m_index; }
    float GetNegOffset() const { return m_negOffset; }
    float GetPosOffset() const { return m_posOffset; }
    const KDNode* GetNegChild() const { return this + 1; }
    const KDNode* GetPosChild() const { return this + m_index; }
};
static_assert(sizeof(KDNode) == 0x0c, "KDNode size mismatch");

// Size: 0x50 (80 bytes, TI: 0x6f46)
struct Mesh {
    /* 0x000 */ BBox m_box;
    /* 0x020 */ cdc::Vector3 m_position;
    /* 0x030 */ void* m_vertices;
    /* 0x034 */ IndexedFace* m_faces;
    /* 0x038 */ KDNode* m_root;
    /* 0x03c */ void* m_clientData;
    /* 0x040 */ uint16_t m_vertexType;
    /* 0x042 */ uint16_t m_numNodes;
    /* 0x044 */ uint16_t m_numFaces;
    /* 0x046 */ uint16_t m_numVertices;
    /* 0x048 */ uint16_t m_height;
    /* 0x04a */ uint16_t m_numDegenerateFaces;
    /* 0x04c */ uint16_t m_numNonManifoldEdges;
    /* 0x04e */ uint16_t pad04e;

    void GetTriangle(MTriangle* out, const IndexedFace* face); // 0x00424130 (PDB: 0x004231a0)
    uint16_t Query(uint16_t* faceIndices, uint16_t maxFaces, const BBox& box, uint8_t flags); // 0x00502f10 (PDB: 0x00504bb0)
    int CollideSegment(SegmentResult* results, int maxResults, const cdc::Vector3& start, const cdc::Vector3& end, unsigned char flags); // 0x005030a0 (PDB: 0x00504d40)
    IndexedFace* CollideSegment(cdc::Vector3* hitPoint, cdc::Vector3* hitNormal, const cdc::Vector3& start, const cdc::Vector3& end, unsigned char flags); // 0x00503350 (PDB: 0x00504ff0)
};
static_assert(offsetof(Mesh, m_position) == 0x20, "Mesh::m_position offset mismatch");
static_assert(offsetof(Mesh, m_faces) == 0x34, "Mesh::m_faces offset mismatch");
static_assert(offsetof(Mesh, m_root) == 0x38, "Mesh::m_root offset mismatch");
static_assert(offsetof(Mesh, m_vertexType) == 0x40, "Mesh::m_vertexType offset mismatch");
static_assert(sizeof(Mesh) == 0x50, "Mesh size mismatch");

// Size: 0xb0 (176 bytes, TI: 0x5e5e)
struct TerrainGroup {
    /* 0x000 */ cdc::Vector3 globalOffset;
    /* 0x010 */ cdc::Vector3 localOffset;
    /* 0x020 */ int flags;
    /* 0x024 */ uint8_t pad024[0x10];
    /* 0x034 */ Level* level;
    /* 0x038 */ Mesh* mesh;
    /* 0x03c */ uint8_t pad03c[0x74];
};
static_assert(offsetof(TerrainGroup, flags) == 0x20, "TerrainGroup::flags offset mismatch");
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
