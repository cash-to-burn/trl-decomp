#include "game/Multibody/collision/CollidePlane.h"

namespace {

int TestSpherePointAgainstPlane(
    CPoint* out,
    const cdc::Vector3& center,
    float radius,
    const cdc::Vector3& planeNormal,
    float planeDist)
{
    const float sep = cdc::Dot3(center, planeNormal) - radius - planeDist;
    if (sep > 0.0f) {
        return 0;
    }

    out->separation = sep;
    out->normal = planeNormal * -1.0f;
    out->position = center - planeNormal * radius;
    return 1;
}

} // namespace

// 0x004fd100 (PDB: 0x004fee00)
int CollideSphereAndPlane(
    CPoint* cp,
    const cdc::Vector3& pos,
    float rad,
    const cdc::Vector3& planeNormal,
    float planeDist)
{
    return TestSpherePointAgainstPlane(cp, pos, rad, planeNormal, planeDist);
}

// 0x004fd1d0 (PDB: 0x004feed0)
int CollideCapsuleAndPlane(
    CPoint* cp,
    const cdc::Vector3& p0,
    const cdc::Vector3& p1,
    float rad,
    const cdc::Vector3& planeNormal,
    float planeDist)
{
    int count = 0;
    count += TestSpherePointAgainstPlane(&cp[count], p0, rad, planeNormal, planeDist);
    count += TestSpherePointAgainstPlane(&cp[count], p1, rad, planeNormal, planeDist);
    return count;
}

// 0x004fd350 (PDB: 0x004ff050)
int CollideBoxAndPlane(
    CPoint* cp,
    const cdc::Vector3& boxCenter,
    const cdc::Quat& boxRot,
    const cdc::Vector3& boxHalfExtents,
    const cdc::Vector3& planeNormal,
    float planeDist)
{
    const cdc::Quat qInv = ~boxRot;
    const cdc::Vector3 localN = qInv.Rotate(planeNormal);
    const cdc::Vector3 absN = MultibodyCollision::AbsVector3(localN);

    cdc::Vector3 corners[4];
    const float hx = boxHalfExtents.x;
    const float hy = boxHalfExtents.y;
    const float hz = boxHalfExtents.z;

    if (absN.x > absN.y && absN.x > absN.z) {
        const float sx = ((localN.x >= 0.0f) ? 1.0f : -1.0f) * (-hx);
        corners[0] = cdc::Vector3::Make(sx, -hy, hz, 0.0f);
        corners[1] = cdc::Vector3::Make(sx, -hy, -hz, 0.0f);
        corners[2] = cdc::Vector3::Make(sx, hy, -hz, 0.0f);
        corners[3] = cdc::Vector3::Make(sx, hy, hz, 0.0f);
    } else if (absN.y > absN.z) {
        const float sy = ((localN.y >= 0.0f) ? 1.0f : -1.0f) * (-hy);
        corners[0] = cdc::Vector3::Make(-hx, sy, hz, 0.0f);
        corners[1] = cdc::Vector3::Make(-hx, sy, -hz, 0.0f);
        corners[2] = cdc::Vector3::Make(hx, sy, -hz, 0.0f);
        corners[3] = cdc::Vector3::Make(hx, sy, hz, 0.0f);
    } else {
        const float sz = ((localN.z >= 0.0f) ? 1.0f : -1.0f) * (-hz);
        corners[0] = cdc::Vector3::Make(-hx, hy, sz, 0.0f);
        corners[1] = cdc::Vector3::Make(-hx, -hy, sz, 0.0f);
        corners[2] = cdc::Vector3::Make(hx, -hy, sz, 0.0f);
        corners[3] = cdc::Vector3::Make(hx, hy, sz, 0.0f);
    }

    int count = 0;
    for (int i = 0; i < 4; ++i) {
        const cdc::Vector3 worldCorner = boxCenter + boxRot.Rotate(corners[i]);
        const float sep = cdc::Dot3(worldCorner, planeNormal) - planeDist;
        if (sep <= 0.0f) {
            cp[count].position = worldCorner;
            cp[count].normal = planeNormal * -1.0f;
            cp[count].separation = sep;
            ++count;
        }
    }

    return count;
}
