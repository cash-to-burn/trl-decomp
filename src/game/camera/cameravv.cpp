#include "game/camera/cameravv.h"
#include "game/camera/camera.h"
#include "game/main.h"
#include "game/pc/setup.h"
#include <cmath>
#include <cstring>

namespace {

void IdentityMatrix(cdc::Matrix* m)
{
    std::memset(m, 0, sizeof(cdc::Matrix));
    m->col0.x = 1.0f;
    m->col1.y = 1.0f;
    m->col2.z = 1.0f;
    m->col3.w = 1.0f;
}

void SetVec(cdc::Vector* v, float x, float y, float z, float w = 0.0f)
{
    v->x = x;
    v->y = y;
    v->z = z;
    v->w = w;
}

void MulVec3Matrix(cdc::Vector* out, const cdc::Matrix* m, const cdc::Vector* v)
{
    out->x = m->col0.x * v->x + m->col1.x * v->y + m->col2.x * v->z;
    out->y = m->col0.y * v->x + m->col1.y * v->y + m->col2.y * v->z;
    out->z = m->col0.z * v->x + m->col1.z * v->y + m->col2.z * v->z;
    out->w = 1.0f;
}

void BuildRotationMatrix(cdc::Matrix* m, const cdc::Euler* rot)
{
    float sx = std::sin(rot->x);
    float cx = std::cos(rot->x);
    float sy = std::sin(rot->y);
    float cy = std::cos(rot->y);
    float sz = std::sin(rot->z);
    float cz = std::cos(rot->z);

    IdentityMatrix(m);
    m->col0.x = cy * cz;
    m->col0.y = cy * sz;
    m->col0.z = -sy;

    m->col1.x = sx * sy * cz - cx * sz;
    m->col1.y = sx * sy * sz + cx * cz;
    m->col1.z = sx * cy;

    m->col2.x = cx * sy * cz + sx * sz;
    m->col2.y = cx * sy * sz - sx * cz;
    m->col2.z = cx * cy;
}

void TransposeMatrix3x3(cdc::Matrix* dst, const cdc::Matrix* src)
{
    IdentityMatrix(dst);
    dst->col0.x = src->col0.x; dst->col0.y = src->col1.x; dst->col0.z = src->col2.x;
    dst->col1.x = src->col0.y; dst->col1.y = src->col1.y; dst->col1.z = src->col2.y;
    dst->col2.x = src->col0.z; dst->col2.y = src->col1.z; dst->col2.z = src->col2.z;
}

} // namespace

// 0x0048d650 (PDB: 0x0048f160)
void CAMERA_CalculateViewVolumeNormals(CameraCore* camera)
{
    float left = camera->leftX;
    float right = camera->rightX;
    float top = camera->topY;
    float bottom = camera->bottomY;
    float proj = camera->projDistance;

    // 5 view frustum plane normals: near, left, right, top, bottom
    SetVec(&camera->viewVolumeNormal[0], 0.0f, 0.0f, 1.0f, 0.0f);

    float lenL = std::sqrt(proj * proj + left * left);
    SetVec(&camera->viewVolumeNormal[1], proj / lenL, 0.0f, -left / lenL, 0.0f);

    float lenR = std::sqrt(proj * proj + right * right);
    SetVec(&camera->viewVolumeNormal[2], -proj / lenR, 0.0f, right / lenR, 0.0f);

    float lenT = std::sqrt(proj * proj + top * top);
    SetVec(&camera->viewVolumeNormal[3], 0.0f, -proj / lenT, top / lenT, 0.0f);

    float lenB = std::sqrt(proj * proj + bottom * bottom);
    SetVec(&camera->viewVolumeNormal[4], 0.0f, proj / lenB, -bottom / lenB, 0.0f);
}

// 0x0048d970 (PDB: 0x0048f480)
void CAMERA_AspectAdjustMatrixf(cdc::Matrix* matrix)
{
    float xScale = 512.0f * (1.0f / 512.0f) * theCamera.core.screenXRatio;
    float yScale = 448.0f * (1.0f / 384.0f) * theCamera.core.screenYRatio;

    matrix->col0.x *= xScale;
    matrix->col1.x *= xScale;
    matrix->col2.x *= xScale;

    matrix->col0.y *= yScale;
    matrix->col1.y *= yScale;
    matrix->col2.y *= yScale;
}

// 0x0048da40 (PDB: 0x0048f550)
void CAMERA_SetNewProjDistance(CameraCore* camera, float projDist, float speed)
{
    camera->newProjDistance = projDist;
    camera->projDistanceSpd = speed;
}

// 0x0048da60 (PDB: 0x0048f570)
int CAMERA_SphereIntersectsViewVolume(cdc::Vector3* sphere, CameraCore* camera)
{
    float radius = sphere->w;
    cdc::Vector3 offset;
    offset.x = sphere->x - camera->position.x;
    offset.y = sphere->y - camera->position.y;
    offset.z = sphere->z - camera->position.z;
    offset.w = 1.0f;

    cdc::Vector3 res0;
    MulVec3Matrix(&res0, &camera->vvNormalWorVecMatf[0], &offset);

    if (res0.z < -radius) {
        return 0;
    }
    if (res0.x < -radius || res0.y < -radius) {
        return 0;
    }
    if (res0.z > camera->farPlane + radius) {
        return 0;
    }

    cdc::Vector3 res1;
    MulVec3Matrix(&res1, &camera->vvNormalWorVecMatf[1], &offset);
    if (res1.x < -radius || res1.y < -radius) {
        return 0;
    }

    return 1;
}

// 0x0048db70 (PDB: 0x0048f680)
void CAMERA_CalcVVClipInfo(CameraCore* camera)
{
    IdentityMatrix(&camera->vvNormalWorVecMatf[0]);
    IdentityMatrix(&camera->vvNormalWorVecMatf[1]);

    cdc::Vector3 n0, n1, n2, n3, n4;
    MulVec3Matrix(&n0, &camera->cwTransform2f, &camera->viewVolumeNormal[0]);
    MulVec3Matrix(&n1, &camera->cwTransform2f, &camera->viewVolumeNormal[1]);
    MulVec3Matrix(&n2, &camera->cwTransform2f, &camera->viewVolumeNormal[2]);
    MulVec3Matrix(&n3, &camera->cwTransform2f, &camera->viewVolumeNormal[3]);
    MulVec3Matrix(&n4, &camera->cwTransform2f, &camera->viewVolumeNormal[4]);

    // Store frustum plane normals into rows of vvNormalWorVecMatf[0] and [1]
    camera->vvNormalWorVecMatf[0].col0.x = n0.x;
    camera->vvNormalWorVecMatf[0].col1.x = n0.y;
    camera->vvNormalWorVecMatf[0].col2.x = n0.z;

    camera->vvNormalWorVecMatf[0].col0.y = n1.x;
    camera->vvNormalWorVecMatf[0].col1.y = n1.y;
    camera->vvNormalWorVecMatf[0].col2.y = n1.z;

    camera->vvNormalWorVecMatf[0].col0.z = n2.x;
    camera->vvNormalWorVecMatf[0].col1.z = n2.y;
    camera->vvNormalWorVecMatf[0].col2.z = n2.z;

    camera->vvNormalWorVecMatf[1].col0.x = n3.x;
    camera->vvNormalWorVecMatf[1].col1.x = n3.y;
    camera->vvNormalWorVecMatf[1].col2.x = n3.z;

    camera->vvNormalWorVecMatf[1].col0.y = n4.x;
    camera->vvNormalWorVecMatf[1].col1.y = n4.y;
    camera->vvNormalWorVecMatf[1].col2.y = n4.z;
}

// 0x0048ddd0 (PDB: 0x0048f8e0)
void CAMERA_CalculateWCTransform(CameraCore* camera)
{
    cdc::Euler negRot;
    cdc::Vector3 negPos;
    cdc::Vector3 negEffectPos;

    if ((g_gameTracker.flags & 0x80008) == 0) {
        SetVec(&negRot, -camera->Rotation.x, -camera->Rotation.y, -camera->Rotation.z, 0.0f);
        SetVec(&negPos, -camera->position.x, -camera->position.y, -camera->position.z, 1.0f);
        SetVec(&negEffectPos, -camera->effectPosition.x, -camera->effectPosition.y, -camera->effectPosition.z, 1.0f);
    } else {
        camera->nondebugPos = camera->position;
        SetVec(&negRot, -camera->debugRot.x, -camera->debugRot.y, -camera->debugRot.z, 0.0f);
        SetVec(&negPos, -camera->debugPos.x, -camera->debugPos.y, -camera->debugPos.z, 1.0f);
        negEffectPos = negPos;
    }

    cdc::Matrix rotMat;
    BuildRotationMatrix(&rotMat, &negRot);

    camera->wcTransform2f = rotMat;
    camera->wcTransformNoShakef = rotMat;
    MulVec3Matrix(&camera->wcTransformNoShakef.col3, &rotMat, &negPos);

    camera->wcTransformf = rotMat;
    MulVec3Matrix(&camera->wcTransformf.col3, &rotMat, &negEffectPos);
}

// 0x0048e100 (PDB: 0x0048fc10)
void CAMERA_SetViewVolume(CameraCore* camera)
{
    CAMERA_CalculateViewVolumeNormals(camera);
    CAMERA_CalcVVClipInfo(camera);
}

// 0x0048e120 (PDB: 0x0048fc30)
void CAMERA_SetDefaultViewVolume(CameraCore* camera)
{
    CAMERA_CalculateViewVolumeNormals(camera);
    CAMERA_CalcVVClipInfo(camera);
    camera->defaultvvNormalWorVecMatf[0] = camera->vvNormalWorVecMatf[0];
    camera->defaultvvNormalWorVecMatf[1] = camera->vvNormalWorVecMatf[1];
}

// 0x0048e150 (PDB: 0x0048fc60)
void CAMERA_SetProjDistance2(CameraCore* camera, float projDist)
{
    if (g_widescreen != 0) {
        camera->screenXRatio = 1.0f;
        camera->screenYRatio = 4.0f / 3.0f;
    } else {
        camera->screenXRatio = 1.0f;
        camera->screenYRatio = 1.0f;
    }
    camera->screenFullXRatio = camera->screenXRatio;
    camera->screenFullYRatio = camera->screenYRatio * (4.0f / 3.0f);
    camera->projDistance = projDist;

    CAMERA_CalculateWCTransform(camera);
    TransposeMatrix3x3(&camera->cwTransform2f, &camera->wcTransform2f);
    CAMERA_CalculateViewVolumeNormals(camera);
    CAMERA_CalcVVClipInfo(camera);
}

// 0x0048e210 (PDB: 0x0048fd20)
void CAMERA_SetProjDistance(CameraCore* camera, float projDist)
{
    CAMERA_SetProjDistance2(camera, projDist);
    camera->newProjDistance = projDist;
    camera->projDistanceSpd = 0.0f;
}
