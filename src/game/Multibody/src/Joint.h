#ifndef GAME_MULTIBODY_SRC_JOINT_H
#define GAME_MULTIBODY_SRC_JOINT_H

#include <cstdint>
#include "cdc/runtime/cdcMath/Math.h"

class Body;
class Joint;

// PDB: LimitReport (size: 0x10)
struct LimitReport {
    enum HitType {
        HIT_LOWER_LIMIT = 0,
        HIT_UPPER_LIMIT = 1
    };

    const Joint* joint; // 0x00
    float speed;        // 0x04
    int limitNumber;    // 0x08
    HitType hitType;    // 0x0c
};
static_assert(sizeof(LimitReport) == 0x10, "LimitReport size mismatch");

// PDB: ShoulderParams (size: 0x50)
struct alignas(16) ShoulderParams {
    cdc::Vector3 anchor;    // 0x00
    cdc::Vector3 coneAxis;  // 0x10
    cdc::Vector3 twistAxis; // 0x20
    Body* parent;           // 0x30
    Body* child;            // 0x34
    float coneAngle;        // 0x38
    float twistLow;         // 0x3c
    float twistHigh;        // 0x40
    float friction;         // 0x44
    uint32_t pad48[2];      // 0x48..0x4f
};
static_assert(sizeof(ShoulderParams) == 0x50, "ShoulderParams size mismatch");

// PDB: SplineJointParams (size: 0x50)
struct alignas(16) SplineJointParams {
    cdc::Vector3 position;    // 0x00
    cdc::Quat quaternion;     // 0x10
    cdc::Vector3 direction;   // 0x20
    float speed;              // 0x30
    float minMotorForce;      // 0x34
    float maxMotorForce;      // 0x38
    float maxLateralForce;    // 0x3c
    float maxTorque;          // 0x40
    uint32_t pad44[3];        // 0x44..0x4f
};
static_assert(sizeof(SplineJointParams) == 0x50, "SplineJointParams size mismatch");

// PDB: JointAxisParams (size: 0x40)
struct alignas(16) JointAxisParams {
    cdc::Vector3 axis;      // 0x00
    cdc::Vector3 r1;        // 0x10
    cdc::Vector3 r2;        // 0x20
    float invBeta;          // 0x30
    float angularThickness; // 0x34
    float linearThickness;  // 0x38
    int index;              // 0x3c
};
static_assert(sizeof(JointAxisParams) == 0x40, "JointAxisParams size mismatch");

// PDB: Joint (size: 0x01)
class Joint {
public:
    enum Type {
        UnknownJoint = 0,
        RevoluteJoint = 1,
        PrismaticJoint = 2,
        PlanarJoint = 3,
        SphericalJoint = 4,
        RopeJoint = 5,
        PulleyJoint = 6,
        UniversalJoint = 7,
        SwingJoint = 8,
        CartesianJoint = 9,
        WeldJoint = 10,
        PointMotor = 11,
        LinkJoint = 12,
        WheelJoint = 13,
        WheelJoint2 = 14,
        ConicalLimit = 15,
        SplineJoint = 16,
        AngularMotor = 17,
        ShoulderJoint = 18
    };

    // 0x0050cf30 (PDB: 0x0050ea20 / ?SetRevolute@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@1@Z)
    void SetRevolute(Body* b1, Body* b2, const cdc::Vector3& anchor, const cdc::Vector3& axis);

    // 0x0050d140 (PDB: 0x0050ec30 / ?SetPrismatic@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@1@Z)
    void SetPrismatic(Body* b1, Body* b2, const cdc::Vector3& anchor, const cdc::Vector3& axis);

    // 0x0050d350 (PDB: 0x0050ee40 / ?SetPlanar@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@11@Z)
    void SetPlanar(
        Body* b1,
        Body* b2,
        const cdc::Vector3& anchor,
        const cdc::Vector3& normal,
        const cdc::Vector3& tangent);

    // 0x0050d5c0 (PDB: 0x0050f0b0 / ?SetSpherical@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@@Z)
    void SetSpherical(Body* b1, Body* b2, const cdc::Vector3& anchor);

    // 0x0050d6d0 (PDB: 0x0050f1c0 / ?SetRope@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@1M@Z)
    void SetRope(
        Body* b1,
        Body* b2,
        const cdc::Vector3& anchor1,
        const cdc::Vector3& anchor2,
        float maxLength);

    // 0x0050d890 (PDB: 0x0050f380 / ?SetPulley@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@111M@Z)
    void SetPulley(
        Body* b1,
        Body* b2,
        const cdc::Vector3& groundAnchor1,
        const cdc::Vector3& groundAnchor2,
        const cdc::Vector3& anchor1,
        const cdc::Vector3& anchor2,
        float ratio);

    // 0x0050da40 (PDB: 0x0050f530 / ?SetUniversal@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@11@Z)
    void SetUniversal(
        Body* b1,
        Body* b2,
        const cdc::Vector3& anchor,
        const cdc::Vector3& axis1,
        const cdc::Vector3& axis2);

    // 0x0050dce0 (PDB: 0x0050f7d0 / ?SetSwing@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@11@Z)
    void SetSwing(
        Body* b1,
        Body* b2,
        const cdc::Vector3& anchor,
        const cdc::Vector3& axis1,
        const cdc::Vector3& axis2);

    // 0x0050df80 (PDB: 0x0050fa70 / ?SetCartesian@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@@Z)
    void SetCartesian(Body* b1, Body* b2, const cdc::Vector3& anchor);

    // 0x0050e240 (PDB: 0x0050fd30 / ?SetWeld@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@@Z)
    void SetWeld(Body* b1, Body* b2, const cdc::Vector3& anchor);

    // 0x0050e580 (PDB: 0x00510050 / ?SetPointMotor@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@1@Z)
    void SetPointMotor(
        Body* b1,
        Body* b2,
        const cdc::Vector3& anchor1,
        const cdc::Vector3& anchor2);

    // 0x0050e6c0 (PDB: 0x00510190 / ?SetLink@Joint@@QAEXPAV1@0M@Z)
    void SetLink(Joint* joint1, Joint* joint2, float ratio);

    // ?SetWheel@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@11MM@Z
    void SetWheel(
        Body* b1,
        Body* b2,
        const cdc::Vector3& anchor,
        const cdc::Vector3& suspensionAxis,
        const cdc::Vector3& wheelAxis,
        float maxSuspensionForce,
        float maxWheelForce);

    // 0x0050e970 (PDB: 0x00510440 / ?SetWheel2@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@11MM1MMM@Z)
    void SetWheel2(
        Body* b1,
        Body* b2,
        const cdc::Vector3& anchor,
        const cdc::Vector3& bx,
        const cdc::Vector3& by,
        float maxForce1,
        float maxForce2,
        const cdc::Vector3& bz,
        float radius,
        float coord1,
        float coord2);

    // 0x00509a70 (PDB: 0x0050b540 / ?UpdateWheel2@Joint@@QAEXABVVector3@cdc@@00MM0MMM@Z)
    void UpdateWheel2(
        const cdc::Vector3& anchor,
        const cdc::Vector3& bx,
        const cdc::Vector3& by,
        float maxForce1,
        float maxForce2,
        const cdc::Vector3& bz,
        float radius,
        float coord1,
        float coord2);

    // 0x0050ea70 (PDB: 0x00510540 / ?UpdatePrismaticAnchor@Joint@@QAEXXZ)
    void UpdatePrismaticAnchor();

    // 0x00509b40 (PDB: 0x0050b610 / ?UpdateRope@Joint@@QAEXABVVector3@cdc@@@Z)
    void UpdateRope(const cdc::Vector3& anchor);

    // 0x00509b70 (PDB: 0x0050b640 / ?UpdateUniversal@Joint@@QAEXABVVector3@cdc@@000@Z)
    void UpdateUniversal(
        const cdc::Vector3& y1,
        const cdc::Vector3& z1,
        const cdc::Vector3& y2,
        const cdc::Vector3& z2);

    // 0x0050eb40 (PDB: 0x00510610 / ?SetConicalLimit@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@1M@Z)
    void SetConicalLimit(
        Body* b1,
        Body* b2,
        const cdc::Vector3& axis1,
        const cdc::Vector3& axis2,
        float angle);

    // 0x0050ec10 (PDB: 0x005106e0 / ?SetAngularMotor@Joint@@QAEXPAVBody@@0ABVVector3@cdc@@@Z)
    void SetAngularMotor(Body* b1, Body* b2, const cdc::Vector3& axis);

    // 0x0050ec90 (PDB: 0x00510760 / ?SetSpline@Joint@@QAEXPAVBody@@ABVVector3@cdc@@P6AXPAUSplineJointParams@@PBV1@M@Z@Z)
    void SetSpline(
        Body* b,
        const cdc::Vector3& anchor,
        void (*callback)(SplineJointParams*, const Joint*, float));

    // 0x00514700 (PDB: 0x005161f0 / ?SetShoulder@Joint@@QAEXAAUShoulderParams@@@Z)
    void SetShoulder(ShoulderParams& params);

    // 0x00509ca0 (PDB: 0x0050b770 / ?SetSoftness@Joint@@QAEXQAM@Z)
    void SetSoftness(float* softness);

    // 0x0050ed60 (PDB: 0x00510830 / ?SetRelaxation@Joint@@QAEXM@Z)
    void SetRelaxation(float relaxation);

    // 0x00509ce0 (PDB: 0x0050b7b0 / ?SetStepIterations@Joint@@QAEXE@Z)
    void SetStepIterations(uint8_t iterations);

    // 0x00509e30 (PDB: 0x0050b900 / ?SetClientData@Joint@@QAEXPAX@Z)
    void SetClientData(void* data);

    // 0x00509e40 (PDB: 0x0050b910 / ?GetClientData@Joint@@QBEPAXXZ)
    void* GetClientData() const;

    // 0x00509e50 (PDB: 0x0050b920 / ?SetDestructionCallback@Joint@@QAEXP6AXPAXPAV1@@Z@Z)
    void SetDestructionCallback(void (*cb)(void*, Joint*));

    // 0x00509d50 (PDB: 0x0050b820 / ?GetType@Joint@@QBE?AW4Type@1@XZ)
    Type GetType() const;

    // 0x00509d60 (PDB: 0x0050b830 / ?GetBody1@Joint@@QBEPAVBody@@XZ)
    Body* GetBody1() const;

    // 0x00509d70 (PDB: 0x0050b840 / ?GetBody2@Joint@@QBEPAVBody@@XZ)
    Body* GetBody2() const;

    // 0x00509d80 (PDB: 0x0050b850 / ?GetAnchor1@Joint@@QBE?AVVector3@cdc@@XZ)
    cdc::Vector3 GetAnchor1() const;

    // 0x00509df0 (PDB: 0x0050b8c0 / ?GetAnchor2@Joint@@QBE?AVVector3@cdc@@XZ)
    cdc::Vector3 GetAnchor2() const;

    // PDB: 0x005e5490 / ?GetPulleyGroundAnchor1@Joint@@QBE?AVVector3@cdc@@XZ
    cdc::Vector3 GetPulleyGroundAnchor1() const;

    // PDB: 0x00479550 / ?GetPulleyGroundAnchor2@Joint@@QBE?AVVector3@cdc@@XZ
    cdc::Vector3 GetPulleyGroundAnchor2() const;

    // 0x0050ee80 (PDB: 0x00510950 / ?GetRevoluteAngle@Joint@@QBEMXZ)
    float GetRevoluteAngle() const;

    // 0x0050ee60 (PDB: 0x00510930 / ?GetRevoluteAngleSpeed@Joint@@QBEMXZ)
    float GetRevoluteAngleSpeed() const;

    // 0x0050edc0 (PDB: 0x00510890 / ?GetPrismaticTranslation@Joint@@QBEMXZ)
    float GetPrismaticTranslation() const;

    // 0x0050edd0 (PDB: 0x005108a0 / ?GetPrismaticTranslationSpeed@Joint@@QBEMXZ)
    float GetPrismaticTranslationSpeed() const;

    // 0x0050ede0 (PDB: 0x005108b0 / ?GetPlanarTranslation1@Joint@@QBEMXZ)
    float GetPlanarTranslation1() const;

    // 0x0050edf0 (PDB: 0x005108c0 / ?GetPlanarTranslation2@Joint@@QBEMXZ)
    float GetPlanarTranslation2() const;

    // 0x0050ee80 (PDB: 0x00510950 / ?GetPlanarAngle@Joint@@QBEMXZ)
    float GetPlanarAngle() const;

    // 0x0050ee00 (PDB: 0x005108d0 / ?GetPlanarTranslationSpeed1@Joint@@QBEMXZ)
    float GetPlanarTranslationSpeed1() const;

    // 0x0050ee10 (PDB: 0x005108e0 / ?GetPlanarTranslationSpeed2@Joint@@QBEMXZ)
    float GetPlanarTranslationSpeed2() const;

    // 0x0050ee20 (PDB: 0x005108f0 / ?GetPlanarAngleSpeed@Joint@@QBEMXZ)
    float GetPlanarAngleSpeed() const;

    // 0x00509f00 (PDB: 0x0050b9d0 / ?GetSphericalAngularVelocity@Joint@@QBE?AVVector3@cdc@@XZ)
    cdc::Vector3 GetSphericalAngularVelocity() const;

    // ?GetPulleyLength1@Joint@@QBEMXZ
    float GetPulleyLength1() const;

    // ?GetPulleyLength2@Joint@@QBEMXZ
    float GetPulleyLength2() const;

    // 0x00509f70 (PDB: 0x0050ba40 / ?GetPulleySpeed1@Joint@@QBEMXZ)
    float GetPulleySpeed1() const;

    // 0x0050a050 (PDB: 0x0050bb20 / ?GetPulleySpeed2@Joint@@QBEMXZ)
    float GetPulleySpeed2() const;

    // 0x0050ee30 (PDB: 0x00510900 / ?GetUniversalAngle1@Joint@@QBEMXZ)
    float GetUniversalAngle1() const;

    // 0x0050ee40 (PDB: 0x00510910 / ?GetUniversalAngle2@Joint@@QBEMXZ)
    float GetUniversalAngle2() const;

    // 0x0050ee50 (PDB: 0x00510920 / ?GetUniversalAngleSpeed1@Joint@@QBEMXZ)
    float GetUniversalAngleSpeed1() const;

    // 0x0050ee60 (PDB: 0x00510930 / ?GetUniversalAngleSpeed2@Joint@@QBEMXZ)
    float GetUniversalAngleSpeed2() const;

    // 0x0050ee70 (PDB: 0x00510940 / ?GetSwingTranslation@Joint@@QBEMXZ)
    float GetSwingTranslation() const;

    // 0x0050ee80 (PDB: 0x00510950 / ?GetSwingAngle1@Joint@@QBEMXZ)
    float GetSwingAngle1() const;

    // 0x0050ee90 (PDB: 0x00510960 / ?GetSwingAngle2@Joint@@QBEMXZ)
    float GetSwingAngle2() const;

    // 0x0050eea0 (PDB: 0x00510970 / ?GetSwingTranslationSpeed@Joint@@QBEMXZ)
    float GetSwingTranslationSpeed() const;

    // 0x0050eeb0 (PDB: 0x00510980 / ?GetSwingAngleSpeed1@Joint@@QBEMXZ)
    float GetSwingAngleSpeed1() const;

    // 0x0050eec0 (PDB: 0x00510990 / ?GetSwingAngleSpeed2@Joint@@QBEMXZ)
    float GetSwingAngleSpeed2() const;

    // ?GetCartesianTranslation@Joint@@QBE?AVVector3@cdc@@XZ
    cdc::Vector3 GetCartesianTranslation() const;

    // 0x0050eed0 (PDB: 0x005109a0 / ?GetCartesianTranslationSpeed@Joint@@QBE?AVVector3@cdc@@XZ)
    cdc::Vector3 GetCartesianTranslationSpeed() const;

    // 0x0050a130 (PDB: 0x0050bc00 / ?GetSplineSpeed@Joint@@QBEMXZ)
    float GetSplineSpeed() const;

    // 0x0050a140 (PDB: 0x0050bc10 / ?GetLinkData@Joint@@QAEXAAM0@Z)
    void GetLinkData(float& coord1, float& coord2);

    // 0x0050a160 (PDB: 0x0050bc30 / ?SetLinkData@Joint@@QAEXMM@Z)
    void SetLinkData(float coord1, float coord2);

    // 0x00509e60 (PDB: 0x0050b930 / ?SetCollideConnected@Joint@@QAEX_N@Z)
    void SetCollideConnected(bool collide);

    // ?GetCollideConnected@Joint@@QBE_NXZ
    bool GetCollideConnected() const;

    // 0x00509e90 (PDB: 0x0050b960 / ?GetReactionForce@Joint@@QBE?AVVector3@cdc@@XZ)
    cdc::Vector3 GetReactionForce() const;

    // 0x00509ec0 (PDB: 0x0050b990 / ?GetReactionTorque@Joint@@QBE?AVVector3@cdc@@XZ)
    cdc::Vector3 GetReactionTorque() const;

    // 0x00509ef0 (PDB: 0x0050b9c0 / ?GetSplineMotorForce@Joint@@QBEMXZ)
    float GetSplineMotorForce() const;

    // 0x0050a190 (PDB: 0x0050bc60 / ?EnableLimit@Joint@@QAEXH_N@Z)
    void EnableLimit(int limitNumber, bool enable);

    // 0x0050a1e0 (PDB: 0x0050bcb0 / ?IsLimitEnabled@Joint@@QAE_NH@Z)
    bool IsLimitEnabled(int limitNumber);

    // 0x0050a210 (PDB: 0x0050bce0 / ?SetLimit@Joint@@QAEXHMM@Z)
    void SetLimit(int limitNumber, float low, float high);

    // 0x0050a230 (PDB: 0x0050bd00 / ?GetLimit@Joint@@QAEXHAAM0@Z)
    void GetLimit(int limitNumber, float& low, float& high);

    // ?GetLimitForce@Joint@@QBEMH@Z
    float GetLimitForce(int limitNumber) const;

    // 0x00509cf0 (PDB: 0x0050b7c0 / ?SetHasLimitListener@Joint@@QAEX_N@Z)
    void SetHasLimitListener(bool hasListener);

    // 0x00509d20 (PDB: 0x0050b7f0 / ?SetSmoothLimits@Joint@@QAEX_N@Z)
    void SetSmoothLimits(bool smooth);

    // 0x0050a260 (PDB: 0x0050bd30 / ?EnableMotor@Joint@@QAEXH_N@Z)
    void EnableMotor(int motorNumber, bool enable);

    // 0x0050a2d0 (PDB: 0x0050bda0 / ?IsMotorEnabled@Joint@@QAE_NH@Z)
    bool IsMotorEnabled(int motorNumber);

    // 0x0050a300 (PDB: 0x0050bdd0 / ?SetMotor@Joint@@QAEXHM@Z)
    void SetMotor(int motorNumber, float velocity);

    // 0x0050a320 (PDB: 0x0050bdf0 / ?GetMotor@Joint@@QAEMH@Z)
    float GetMotor(int motorNumber);

    // 0x0050a330 (PDB: 0x0050be00 / ?SetMotorMaxForce@Joint@@QAEXHM@Z)
    void SetMotorMaxForce(int motorNumber, float maxForce);

    // 0x0050a350 (PDB: 0x0050be20 / ?GetMotorMaxForce@Joint@@QAEMH@Z)
    float GetMotorMaxForce(int motorNumber);

    // 0x0050a360 (PDB: 0x0050be30 / ?GetMotorForce@Joint@@QBEMH@Z)
    float GetMotorForce(int motorNumber) const;
};

#endif // GAME_MULTIBODY_SRC_JOINT_H
