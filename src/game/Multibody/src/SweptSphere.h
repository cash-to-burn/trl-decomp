#ifndef GAME_MULTIBODY_SRC_SWEPTSPHERE_H
#define GAME_MULTIBODY_SRC_SWEPTSPHERE_H

struct GeometryImpl;
struct MultibodySystemImpl;

// 0x005190b0 (PDB: 0x0051abe0 / ?CollidePhantomAndMesh@@YAXPAVMultibodySystemImpl@@PAVGeometryImpl@@@Z)
void CollidePhantomAndMesh(MultibodySystemImpl* system, GeometryImpl* geom);

// 0x00519760 (PDB: 0x0051b290 / ?CollidePhantomAndGeometries@@YAXPAVMultibodySystemImpl@@PAVGeometryImpl@@@Z)
void CollidePhantomAndGeometries(MultibodySystemImpl* system, GeometryImpl* geom);

// 0x00519b30 (PDB: 0x0051b660 / ?CollidePhantoms@@YAXPAVMultibodySystemImpl@@@Z)
void CollidePhantoms(MultibodySystemImpl* system);

#endif // GAME_MULTIBODY_SRC_SWEPTSPHERE_H
