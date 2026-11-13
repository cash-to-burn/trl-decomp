#ifndef GAME_MULTIBODY_SRC_TUNNEL_H
#define GAME_MULTIBODY_SRC_TUNNEL_H

class Geometry;
struct GeometryImpl;
struct MultibodySystemImpl;

struct RegulateTunnelData {
    MultibodySystemImpl* system; // 0x00
    GeometryImpl* geom;          // 0x04
};

// 0x0051b570 (PDB: 0x0051d0a0 / ?RegulateNearGeometryCallback@@YA_NPAVGeometry@@PAX@Z)
bool RegulateNearGeometryCallback(Geometry* other, void* userData);

#endif // GAME_MULTIBODY_SRC_TUNNEL_H
