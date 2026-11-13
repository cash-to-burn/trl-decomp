#include "game/Multibody/src/Buoyancy.h"

namespace {

// 0x00504920 (PDB: 0x00506560 / ?TetVolume@@YAMPAVVector3@cdc@@ABV12@111@Z)
float TetVolume(
    cdc::Vector3* outMoment,
    const cdc::Vector3& v0,
    const cdc::Vector3& v1,
    const cdc::Vector3& v2,
    const cdc::Vector3& p)
{
    const cdc::Vector3 e1 = v2 - v0;
    const cdc::Vector3 e2 = p - v0;
    cdc::Vector3 crossVec;
    cdc::Cross3(&crossVec, e1, e2);

    const cdc::Vector3 e0 = v1 - v0;
    const float vol6 = e0.x * crossVec.x + e0.y * crossVec.y + e0.z * crossVec.z;

    outMoment->x += vol6 * (v0.x + v1.x + v2.x + p.x);
    outMoment->y += vol6 * (v0.y + v1.y + v2.y + p.y);
    outMoment->z += vol6 * (v0.z + v1.z + v2.z + p.z);
    return vol6;
}

// 0x005049d0 (PDB: 0x00506610 / ?ClipConfiguration@@YAMPAVVector3@cdc@@ABV12@111MMM@Z)
float ClipConfiguration(
    cdc::Vector3* outMoment,
    const cdc::Vector3& p0,
    const cdc::Vector3& a,
    const cdc::Vector3& b,
    const cdc::Vector3& c,
    float da,
    float db,
    float dc)
{
    const float tab = da / (da - db);
    const cdc::Vector3 ab = a + (b - a) * tab;

    if (da <= 0.0f) {
        if (dc <= 0.0f) {
            const float tbc = db / (db - dc);
            const cdc::Vector3 bc = b + (c - b) * tbc;
            return TetVolume(outMoment, p0, a, ab, c) +
                   TetVolume(outMoment, p0, ab, bc, c);
        }
        const float tac = da / (da - dc);
        const cdc::Vector3 ac = a + (c - a) * tac;
        return TetVolume(outMoment, p0, a, ab, ac);
    }

    if (dc <= 0.0f) {
        const float tac = da / (da - dc);
        const cdc::Vector3 ac = a + (c - a) * tac;
        return TetVolume(outMoment, p0, ab, b, c) +
               TetVolume(outMoment, p0, ac, ab, c);
    }

    const float tbc = db / (db - dc);
    const cdc::Vector3 bc = b + (c - b) * tbc;
    return TetVolume(outMoment, p0, ab, b, bc);
}

} // namespace

// 0x00504820 (PDB: 0x00506460 / ?SphereBuoyancy@@YAMPAVVector3@cdc@@ABV12@1MM@Z)
float SphereBuoyancy(
    cdc::Vector3* outCentroid,
    const cdc::Vector3& sphereCenter,
    const cdc::Vector3& planeNormal,
    float planeDist,
    float radius)
{
    const float signedDist =
        sphereCenter.x * planeNormal.x +
        sphereCenter.y * planeNormal.y +
        sphereCenter.z * planeNormal.z -
        planeDist;

    if (signedDist >= radius) {
        *outCentroid = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
        return 0.0f;
    }

    if (signedDist <= -radius) {
        *outCentroid = sphereCenter;
        constexpr float kFourPiOverThree = 4.188790321350098f;
        return kFourPiOverThree * radius * radius * radius;
    }

    const float h = radius - signedDist;
    const float twoRMinusH = 2.0f * radius - h;
    const float threeRMinusH = 3.0f * radius - h;
    const float centroidOffset = (0.75f * twoRMinusH * twoRMinusH) / threeRMinusH;

    *outCentroid = cdc::Vector3::Make(
        sphereCenter.x - planeNormal.x * centroidOffset,
        sphereCenter.y - planeNormal.y * centroidOffset,
        sphereCenter.z - planeNormal.z * centroidOffset,
        0.0f);

    constexpr float kPiOverThree = 1.0471975803375244f;
    return kPiOverThree * h * h * threeRMinusH;
}

// 0x00504ba0 (PDB: 0x005067e0 / ?BoxBuoyancy@@YAMPAVVector3@cdc@@ABV12@ABVQuat@2@11M@Z)
float BoxBuoyancy(
    cdc::Vector3* outCentroid,
    const cdc::Vector3& boxCenter,
    const cdc::Quat& boxRot,
    const cdc::Vector3& boxWidth,
    const cdc::Vector3& planeNormal,
    float planeDist)
{
    const cdc::Vector3 h = boxWidth * 0.5f;
    const cdc::Vector3 localNormal = (~boxRot).Rotate(planeNormal);
    const float localPlaneDist = planeDist - (
        boxCenter.x * planeNormal.x +
        boxCenter.y * planeNormal.y +
        boxCenter.z * planeNormal.z);

    const cdc::Vector3 verts[8] = {
        cdc::Vector3::Make(-h.x, -h.y, -h.z),
        cdc::Vector3::Make(-h.x, -h.y,  h.z),
        cdc::Vector3::Make(-h.x,  h.y, -h.z),
        cdc::Vector3::Make(-h.x,  h.y,  h.z),
        cdc::Vector3::Make( h.x, -h.y, -h.z),
        cdc::Vector3::Make( h.x, -h.y,  h.z),
        cdc::Vector3::Make( h.x,  h.y, -h.z),
        cdc::Vector3::Make( h.x,  h.y,  h.z)
    };

    float dists[8];
    int submergedCount = 0;
    int lastSubmergedIdx = 0;
    constexpr float kSubmergedTolerance = -0.28f;
    for (int i = 0; i < 8; ++i) {
        dists[i] =
            verts[i].x * localNormal.x +
            verts[i].y * localNormal.y +
            verts[i].z * localNormal.z -
            localPlaneDist;
        if (dists[i] < kSubmergedTolerance) {
            ++submergedCount;
            lastSubmergedIdx = i;
        }
    }

    if (submergedCount == 0) {
        *outCentroid = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
        return 0.0f;
    }

    const cdc::Vector3 p0 = verts[lastSubmergedIdx] - localNormal * dists[lastSubmergedIdx];

    static constexpr int kBoxTris[12][3] = {
        { 0, 1, 3 }, { 0, 3, 2 },
        { 6, 3, 7 }, { 6, 2, 3 },
        { 4, 6, 5 }, { 6, 7, 5 },
        { 4, 5, 0 }, { 0, 5, 1 },
        { 5, 7, 1 }, { 7, 3, 1 },
        { 0, 6, 4 }, { 0, 2, 6 }
    };

    float totalVol6 = 0.0f;
    cdc::Vector3 moment = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);

    for (int i = 0; i < 12; ++i) {
        const int i0 = kBoxTris[i][0];
        const int i1 = kBoxTris[i][1];
        const int i2 = kBoxTris[i][2];
        const float d0 = dists[i0];
        const float d1 = dists[i1];
        const float d2 = dists[i2];

        if (d0 * d1 <= 0.0f) {
            totalVol6 += ClipConfiguration(&moment, p0, verts[i0], verts[i1], verts[i2], d0, d1, d2);
        } else if (d0 * d2 <= 0.0f) {
            totalVol6 += ClipConfiguration(&moment, p0, verts[i1], verts[i2], verts[i0], d1, d2, d0);
        } else if (d1 * d2 <= 0.0f) {
            totalVol6 += ClipConfiguration(&moment, p0, verts[i2], verts[i0], verts[i1], d2, d0, d1);
        } else if (d0 <= 0.0f && d1 <= 0.0f && d2 <= 0.0f) {
            totalVol6 += TetVolume(&moment, p0, verts[i0], verts[i1], verts[i2]);
        }
    }

    constexpr float kMinVol6 = 0.021952f; // 0.28^3
    if (totalVol6 <= kMinVol6) {
        *outCentroid = cdc::Vector3::Make(0.0f, 0.0f, 0.0f, 0.0f);
        return 0.0f;
    }

    const cdc::Vector3 localCentroid = moment * (0.25f / totalVol6);
    const cdc::Vector3 worldCentroid = boxRot.Rotate(localCentroid);
    *outCentroid = boxCenter + worldCentroid;

    constexpr float kOneSixth = 1.0f / 6.0f;
    return totalVol6 * kOneSixth;
}
