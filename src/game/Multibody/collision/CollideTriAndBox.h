#ifndef GAME_MULTIBODY_COLLISION_COLLIDETRIANDBOX_H
#define GAME_MULTIBODY_COLLISION_COLLIDETRIANDBOX_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/Multibody/collision/CollideUtils.h"
#include "game/camera/cameracollide.h"

// PDB: TriAndBox (size: 0xb0, TI: 0x98da)
struct alignas(16) TriAndBox {
    enum AxisType {
        NoAxis   = 0,
        TriFace  = 1,
        BoxFaceX = 2,
        BoxFaceY = 3,
        BoxFaceZ = 4,
        EdgeX1   = 5,
        EdgeX2   = 6,
        EdgeX3   = 7,
        EdgeY1   = 8,
        EdgeY2   = 9,
        EdgeY3   = 10,
        EdgeZ1   = 11,
        EdgeZ2   = 12,
        EdgeZ3   = 13,
    };

    enum BoxEdge {
        BOX_EDGE_NX_NZ = 0,
        BOX_EDGE_NX_PZ = 1,
        BOX_EDGE_PX_NZ = 2,
        BOX_EDGE_PX_PZ = 3,
        BOX_EDGE_NX_NY = 4,
        BOX_EDGE_NX_PY = 5,
        BOX_EDGE_PX_NY = 6,
        BOX_EDGE_PX_PY = 7,
        BOX_EDGE_NY_NZ = 8,
        BOX_EDGE_NY_PZ = 9,
        BOX_EDGE_PY_NZ = 10,
        BOX_EDGE_PY_PZ = 11,
    };

    enum TriEdge {
        TRI_EDGE0 = 0x08,
        TRI_EDGE1 = 0x10,
        TRI_EDGE2 = 0x20,
    };

    /* 0x000 */ cdc::Vector3 v1;
    /* 0x010 */ cdc::Vector3 v2;
    /* 0x020 */ cdc::Vector3 v3;
    /* 0x030 */ cdc::Vector3 d1;
    /* 0x040 */ cdc::Vector3 d2;
    /* 0x050 */ cdc::Vector3 d3;
    /* 0x060 */ cdc::Vector3 triNormal;
    /* 0x070 */ cdc::Vector3 normal;
    /* 0x080 */ cdc::Vector3 edgeNormal;
    /* 0x090 */ cdc::Vector3 h;
    /* 0x0a0 */ const OBox* box;
    /* 0x0a4 */ float separation;
    /* 0x0a8 */ AxisType axis;
    /* 0x0ac */ unsigned char adjacencyFlags;

    // 0x004feea0 (PDB: 0x00500ba0)
    int ComputeBoxFaceContact(CPoint* cp, int maxContacts, AxisType faceAxis);
    // 0x004ff620 (PDB: 0x00501320)
    void BuildBoxPolygon(ClipPoint* poly);
    // 0x004ffad0 (PDB: 0x005017d0)
    int ComputeTriFaceContact(CPoint* cp, int maxContacts);
    // 0x00500000 (PDB: 0x00501d00)
    int Collide(CPoint* cp, int maxContacts, const OBox* obox, const MTriangle* tri, unsigned char adj);
};
static_assert(offsetof(TriAndBox, box) == 0xa0, "TriAndBox::box offset mismatch");
static_assert(offsetof(TriAndBox, adjacencyFlags) == 0xac, "TriAndBox::adjacencyFlags offset mismatch");
static_assert(sizeof(TriAndBox) == 0xb0, "TriAndBox size mismatch");

// 0x00500980 (PDB: 0x00502680 / ?CollideTriAndBox@@YAHPAUCPoint@@HABUOBox@@ABUMTriangle@@E@Z)
int CollideTriAndBox(
    CPoint* cp,
    int maxContacts,
    const OBox& box,
    const MTriangle& tri,
    unsigned char adj);

#endif // GAME_MULTIBODY_COLLISION_COLLIDETRIANDBOX_H
