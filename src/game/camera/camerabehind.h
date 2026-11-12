#ifndef GAME_CAMERA_CAMERABEHIND_H
#define GAME_CAMERA_CAMERABEHIND_H

#include "cdc/runtime/cdcMath/Math.h"

struct Camera;

// Size: 0x34 bytes (52 bytes, TI: 0xbb6d)
struct shapeValueParams {
    /* 0x00 */ float high_stop_degree;
    /* 0x04 */ float low_stop_degree;
    /* 0x08 */ float high_degree;
    /* 0x0c */ float med_degree;
    /* 0x10 */ float middle_degree;
    /* 0x14 */ float low_degree;
    /* 0x18 */ float high_stop_val;
    /* 0x1c */ float low_stop_val;
    /* 0x20 */ float high_val;
    /* 0x24 */ float med_val;
    /* 0x28 */ float zero_val;
    /* 0x2c */ float low_val;
    /* 0x30 */ float xrot;
};
static_assert(sizeof(shapeValueParams) == 0x34, "shapeValueParams size mismatch");

// Follow camera globals (Original: f:\tr7\game\camera\cameraBehind.c)
extern float hybridSplineDist;      // 0x010fd8e4 (PDB: 0x010feae4)
extern int gBehindCamOverride;      // 0x010fd8e8 (PDB: 0x010feae8)
extern int cameraCloseLimitingTilt; // 0x010fd8ec (PDB: 0x010feaec)
extern float splineLookX;           // 0x010fda7c (PDB: 0x010fec7c)
extern float splineLookZ;           // 0x010fda80 (PDB: 0x010fec80)

// Follow / behind camera routines (Original: f:\tr7\game\camera\cameraBehind.c)
void  CAMERA_CalcAutoCenterData(Camera* camera, float* outMult, float* outSpeed); // 0x00480470 (PDB: 0x00481e10)
float CameraGetShapeValue(shapeValueParams* params);                               // 0x00480dc0 (PDB: 0x00482760)
void  CAMERA_ResetFollowTransition(Camera* camera);                                // 0x00480e90 (PDB: 0x00482830)
void  CAMERA_OffsetRecentZPos(const cdc::Vector3& offset);                         // 0x00481550 (PDB: 0x00482ef0)
void  CAMERA_GenericCameraProcess(Camera* camera);                                 // 0x004824b0 (PDB: 0x00483e50)

#endif // GAME_CAMERA_CAMERABEHIND_H
