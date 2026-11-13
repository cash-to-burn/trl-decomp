#include "game/Multibody/src/MultibodyTypes.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include "game/Multibody/src/Body.h"
#include "game/Multibody/src/ContactJoint.h"

extern void* getScratchAddr(int size);                   // 0x004019c0 (PDB: 0x004019f0)
extern int getScratchLeft();                             // 0x004019e0 (PDB: 0x00401a10)
extern void popScratchAddr(int size);                    // 0x004019f0 (PDB: 0x00401a20)
extern void MULTIBODY_PrintWarning(const char* fmt, ...); // 0x00552e70 (PDB: 0x00554e70)

namespace {

constexpr unsigned int kBodyFlagBad = 0x2u;
constexpr unsigned int kBodyFlagTag1 = 0x8u;
constexpr uint8_t kContactFlagVisited = 0x10u;
constexpr uint8_t kContactFlagHasBody1 = 0x80u;

// 0x00519bb0 (PDB: 0x0051b6e0 / ?Compute_fc@@YAXPAVVector3@cdc@@HPBV12@HQAY01HPBM@Z)
void Compute_fc(
    cdc::Vector3* fc,
    int nb,
    const ContactJacobianRow* B,
    int nc,
    const int (*JMap)[2],
    const float* lambda)
{
    const cdc::Vector3 zero = cdc::Vector3::Make(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < nb * 2; ++i) {
        fc[i] = zero;
    }

    for (int r = 0; r < nc; ++r) {
        const float lam = lambda[r];
        const int i1 = JMap[r][0];
        if (i1 >= 0) {
            fc[2 * i1 + 0] += B[r].v1 * lam;
            fc[2 * i1 + 1] += B[r].w1 * lam;
        }

        const int i2 = JMap[r][1];
        fc[2 * i2 + 0] += B[r].v2 * lam;
        fc[2 * i2 + 1] += B[r].w2 * lam;
    }
}

// 0x0051a150 (PDB: 0x0051bc80 / ?UpdateIslandSleep@@YAXPAV?$List@PAVBodyImpl@@@@HPAVBodyImpl@@MM_N@Z)
void UpdateIslandSleep(
    BodyImpl** bodies,
    int nb,
    float dt,
    float sleepDelay,
    bool autoSleep)
{
    const cdc::Vector3 zero = cdc::Vector3::Make(0.0f, 0.0f, 0.0f);
    int sleepReadyCount = 0;
    float maxSleepTimer = 0.0f;

    for (int i = 0; i < nb; ++i) {
        BodyImpl* body = bodies[i];
        body->F = zero;
        body->T = zero;

        if (!autoSleep) {
            continue;
        }

        const bool wasBad = (body->flags & kBodyFlagBad) != 0;
        body->flags &= ~kBodyFlagBad;
        if (!wasBad && body->sleep.autoSleep) {
            const float linThresh = body->sleep.linearThreshold;
            const float angThresh = body->sleep.angularThreshold;
            const float linSq = body->v.Len3Squared();
            const float angSq = body->omega.Len3Squared();
            if (linSq < linThresh * linThresh && angSq < angThresh * angThresh) {
                ++sleepReadyCount;
                if (body->sleepTimer > maxSleepTimer) {
                    maxSleepTimer = body->sleepTimer;
                }
            }
        }
    }

    if (!autoSleep) {
        return;
    }

    float newTimer = sleepDelay;
    if (sleepReadyCount == nb) {
        newTimer = std::max(0.0f, maxSleepTimer - dt);
    }

    for (int i = 0; i < nb; ++i) {
        BodyImpl* body = bodies[i];
        body->sleepTimer = newTimer;
        if (newTimer == 0.0f) {
            body->flags = (body->flags & ~Body::FLAG_TUNNEL_RISK) | Body::FLAG_SLEEPING;
            body->v = zero;
            body->omega = zero;
            body->Fc = zero;
            body->Tc = zero;
        }
    }
}

void ClampBodyVelocity(
    cdc::Vector3& vel,
    float maxSpeed,
    float maxSpeedSq,
    const char* warningFmt,
    const char* bodyName)
{
    if (vel.Len3Squared() <= maxSpeedSq) {
        return;
    }

    MULTIBODY_PrintWarning(warningFmt, bodyName);
    const float maxComp = std::max({ std::fabs(vel.x), std::fabs(vel.y), std::fabs(vel.z) });
    vel *= (1.0f / maxComp);
    vel *= maxSpeed;
}

void VisitConnectedBody(
    BodyImpl* selfBody,
    BodyImpl* otherBody,
    BodyImpl** islandStack,
    int& stackTop,
    BodyImpl** islandQueue,
    int& queueCount)
{
    if (otherBody != nullptr) {
        if ((otherBody->flags & kBodyFlagTag1) == 0) {
            otherBody->flags |= kBodyFlagTag1;
            otherBody->flags &= ~Body::FLAG_SLEEPING;
            islandStack[stackTop++] = otherBody;
        }
    } else if (selfBody->level == INT_MAX) {
        selfBody->level = 0;
        islandQueue[queueCount++] = selfBody;
    }
}

} // namespace

// 0x00519cb0 (PDB: 0x0051b7e0 / ?LCPSetup@MultibodySystemImpl@@QAEXHHM@Z)
void MultibodySystemImpl::LCPSetup(int numRows, int numBodies, float invDt)
{
    for (int i = 0; i < numBodies; ++i) {
        BodyImpl* body = bodies[i];
        fc[2 * i + 0] = body->v * invDt + body->F * body->invMass;
        fc[2 * i + 1] = body->omega * invDt + cdc::Mul3x3(invI_global[i], body->T);
    }

    const cdc::Vector3 zero = cdc::Vector3::Make(0.0f, 0.0f, 0.0f);
    for (int r = 0; r < numRows; ++r) {
        const int i1 = JMap[r][0];
        const int i2 = JMap[r][1];

        float fc1Dot = 0.0f;
        float jb1Dot = 0.0f;
        if (i1 >= 0) {
            const BodyImpl* b1 = bodies[i1];
            B[r].v1 = J[r].v1 * b1->invMass;
            B[r].w1 = cdc::Mul3x3(invI_global[i1], J[r].w1);

            fc1Dot = cdc::Dot3(J[r].v1, fc[2 * i1 + 0]) + cdc::Dot3(J[r].w1, fc[2 * i1 + 1]);
            jb1Dot = cdc::Dot3(J[r].v1, B[r].v1) + cdc::Dot3(J[r].w1, B[r].w1);
        } else {
            B[r].v1 = zero;
            B[r].w1 = zero;
        }

        const BodyImpl* b2 = bodies[i2];
        B[r].v2 = J[r].v2 * b2->invMass;
        B[r].w2 = cdc::Mul3x3(invI_global[i2], J[r].w2);

        const float fc2Dot = cdc::Dot3(J[r].v2, fc[2 * i2 + 0]) + cdc::Dot3(J[r].w2, fc[2 * i2 + 1]);
        const float jb2Dot = cdc::Dot3(J[r].v2, B[r].v2) + cdc::Dot3(J[r].w2, B[r].w2);

        rhs[r] = (cVel[r] - cPos[r] * beta) * invDt - (fc1Dot + fc2Dot);
        d[r] = sorOmega / (jb1Dot + jb2Dot + gamma[r]);
    }

    Compute_fc(fc, numBodies, B, numRows, JMap, lambda);
}

// 0x0051a310 (PDB: 0x0051be40 / ?LCPIter@MultibodySystemImpl@@QAEXHHH@Z)
void MultibodySystemImpl::LCPIter(int numRows, int /*numBodies*/, int iterations)
{
    for (int iter = 0; iter < iterations; ++iter) {
        for (int r = 0; r < numRows; ++r) {
            const int i1 = JMap[r][0];
            const int i2 = JMap[r][1];

            float delta = rhs[r] - gamma[r] * lambda[r];
            if (i1 >= 0) {
                delta -= (cdc::Dot3(J[r].v1, fc[2 * i1 + 0]) + cdc::Dot3(J[r].w1, fc[2 * i1 + 1]));
            }
            delta -= (cdc::Dot3(J[r].v2, fc[2 * i2 + 0]) + cdc::Dot3(J[r].w2, fc[2 * i2 + 1]));

            const float oldLambda = lambda[r];
            float newLambda = oldLambda + delta * d[r];
            if (newLambda > hi[r]) {
                newLambda = hi[r];
            }
            if (newLambda < lo[r]) {
                newLambda = lo[r];
            }
            lambda[r] = newLambda;

            const float dLambda = newLambda - oldLambda;
            if (i1 >= 0) {
                fc[2 * i1 + 0] += B[r].v1 * dLambda;
                fc[2 * i1 + 1] += B[r].w1 * dLambda;
            }
            fc[2 * i2 + 0] += B[r].v2 * dLambda;
            fc[2 * i2 + 1] += B[r].w2 * dLambda;
        }
    }
}

// 0x0051a5a0 (PDB: 0x0051c0d0 / ?TimeStep@MultibodySystemImpl@@QAEXHHHMM@Z)
void MultibodySystemImpl::TimeStep(int numBodies, int numJoints, int numContacts, float dt, int /*unused*/)
{
    inv_dt = (dt != 0.0f) ? (1.0f / dt) : 0.0f;

    JointParams params;
    params.dt = dt;
    params.thickness = thickness;
    params.threshold = threshold;
    params.invBeta = (beta > 0.0f) ? (1.0f / beta) : 0.0f;

    for (int i = 0; i < numBodies; ++i) {
        BodyImpl* body = bodies[i];
        body->index = i;

        cdc::Matrix R;
        R.Build(body->q);
        cdc::Matrix RT;
        cdc::Transpose(&RT, R);
        cdc::Matrix tmp;
        cdc::Mul3x3(&tmp, body->invI, RT);
        cdc::Mul3x3(&invI_global[i], R, tmp);

        body->T += body->Tc;
        body->F += body->Fc + gravity * body->mass;

        const float linDamp = std::min(1.0f, std::max(0.0f, 1.0f - dt * body->linearDamping));
        body->v *= linDamp;

        const float angDamp = std::min(1.0f, std::max(0.0f, 1.0f - dt * body->angularDamping));
        body->omega *= angDamp;
    }

    int totalRows = 0;
    for (int j = 0; j < numJoints; ++j) {
        JointImpl* joint = joints[j];
        joint->ComputeLimits(&params);
        totalRows += joint->numRows;
    }
    for (int c = 0; c < numContacts; ++c) {
        totalRows += contactJoints[c]->numRows;
    }

    if (totalRows > 0) {
        cdc::Vector3* savedFc = fc;
        int (*savedJMap)[2] = JMap;
        float* savedRhs = rhs;
        float* savedGamma = gamma;
        float* savedLambda = lambda;
        float* savedD = d;
        float* savedLo = lo;
        float* savedHi = hi;

        int scratchBytes = 0;
        const int neededBytes = (numBodies + totalRows) << 5;
        if (neededBytes < getScratchLeft()) {
            scratchBytes = neededBytes;
            uint8_t* ptr = static_cast<uint8_t*>(getScratchAddr(scratchBytes));
            fc = reinterpret_cast<cdc::Vector3*>(ptr);
            ptr += numBodies * 32;
            JMap = reinterpret_cast<int (*)[2]>(ptr);
            ptr += totalRows * 8;
            rhs = reinterpret_cast<float*>(ptr);
            ptr += totalRows * 4;
            gamma = reinterpret_cast<float*>(ptr);
            ptr += totalRows * 4;
            lambda = reinterpret_cast<float*>(ptr);
            ptr += totalRows * 4;
            d = reinterpret_cast<float*>(ptr);
            ptr += totalRows * 4;
            lo = reinterpret_cast<float*>(ptr);
            ptr += totalRows * 4;
            hi = reinterpret_cast<float*>(ptr);
        }

        const cdc::Vector3 zero = cdc::Vector3::Make(0.0f, 0.0f, 0.0f);
        for (int r = 0; r < totalRows; ++r) {
            J[r].v1 = zero;
            J[r].w1 = zero;
            J[r].v2 = zero;
            J[r].w2 = zero;
        }

        uint8_t iterations = static_cast<uint8_t>(stepIterations);
        int row = 0;

        for (int j = 0; j < numJoints; ++j) {
            JointImpl* joint = joints[j];
            joint->ComputeJacobian(J, cPos, cVel, lo, hi, row, &params);
            if (joint->stepIterations < iterations) {
                iterations = joint->stepIterations;
            }

            const int i1 = (joint->body1 != nullptr) ? joint->body1->index : -1;
            const int i2 = joint->body2->index;
            for (int k = 0; k < joint->numRows; ++k) {
                JMap[row][0] = i1;
                JMap[row][1] = i2;
                lambda[row] = joint->lambda[k];
                gamma[row] = joint->gamma[k] * inv_dt;
                ++row;
            }
        }

        for (int c = 0; c < numContacts; ++c) {
            ContactJoint* cj = contactJoints[c];
            cj->ComputeJacobian(
                J,
                cPos,
                cVel,
                lo,
                hi,
                row,
                reinterpret_cast<const ContactStepParams*>(&params));

            const int i1 = ((cj->flags & kContactFlagHasBody1) != 0) ? cj->body1->index : -1;
            const int i2 = cj->body2->index;
            const int normalRows = cj->flags & 0xf;
            for (int k = 0; k < cj->numRows; ++k) {
                JMap[row][0] = i1;
                JMap[row][1] = i2;
                lambda[row] = cj->lambda[k];
                gamma[row] = (k < normalRows) ? (cj->gamma * inv_dt) : 0.0f;
                ++row;
            }
        }

        LCPSetup(totalRows, numBodies, inv_dt);
        LCPIter(totalRows, numBodies, iterations);

        row = 0;
        for (int j = 0; j < numJoints; ++j) {
            JointImpl* joint = joints[j];
            joint->force = zero;
            joint->torque = zero;

            const cdc::Vector3 r2 = joint->body2->q.Rotate(joint->anchor2);
            for (int k = 0; k < joint->numRows; ++k) {
                const float solvedLambda = lambda[row];
                joint->lambda[k] = solvedLambda;

                const cdc::Vector3 rowForce = J[row].v2 * solvedLambda;
                joint->force += rowForce;

                cdc::Vector3 rCrossF;
                cdc::Cross3(&rCrossF, r2, rowForce);
                joint->torque += J[row].w2 * solvedLambda - rCrossF;
                ++row;
            }
        }

        for (int c = 0; c < numContacts; ++c) {
            ContactJoint* cj = contactJoints[c];
            for (int k = 0; k < cj->numRows; ++k) {
                cj->lambda[k] = lambda[row++];
            }
        }

        for (int i = 0; i < numBodies; ++i) {
            BodyImpl* body = bodies[i];
            if ((body->flags & kBodyFlagBad) == 0) {
                body->v += fc[2 * i + 0] * dt;
                body->omega += fc[2 * i + 1] * dt;
            }
        }

        if (scratchBytes != 0) {
            popScratchAddr(scratchBytes);
            fc = savedFc;
            JMap = savedJMap;
            rhs = savedRhs;
            gamma = savedGamma;
            lambda = savedLambda;
            d = savedD;
            lo = savedLo;
            hi = savedHi;
        }
    }

    for (int i = 0; i < numBodies; ++i) {
        BodyImpl* body = bodies[i];
        if ((body->flags & kBodyFlagBad) != 0) {
            continue;
        }

        body->v += body->F * (dt * body->invMass);
        body->omega += cdc::Mul3x3(invI_global[i], body->T) * dt;

        ClampBodyVelocity(
            body->v,
            600.0f,
            360000.0f,
            "WARNING: Body %s is translating too fast!!!\n",
            body->name);

        if ((body->flags & Body::FLAG_NO_ANGULAR_CLAMP) == 0) {
            ClampBodyVelocity(
                body->omega,
                1.5f,
                2.25f,
                "WARNING: Body %s is spinning too fast!!!\n",
                body->name);
        }

        body->UpdatePosition(dt);
    }
}

// 0x0051b2d0 (PDB: 0x0051ce00 / ?TimeStepIslands@MultibodySystemImpl@@QAEXM_N@Z)
void MultibodySystemImpl::TimeStepIslands(float dt, bool autoSleep)
{
    if (nb == 0) {
        return;
    }

    for (BodyImpl* body = bodyList; body != nullptr; body = body->next) {
        body->flags &= ~kBodyFlagTag1;
        body->level = INT_MAX;
    }

    for (JointImpl* joint = firstJoint; joint != nullptr; joint = joint->next) {
        joint->flags &= ~JointImpl::FLAG_TAGS;
    }

    constexpr unsigned int kSkipBodyMask =
        Body::FLAG_SLEEPING | kBodyFlagBad | kBodyFlagTag1 | Body::FLAG_DISABLED;

    for (BodyImpl* root = bodyList; root != nullptr; root = root->next) {
        if ((root->flags & kSkipBodyMask) != 0) {
            continue;
        }

        root->flags |= kBodyFlagTag1;
        islandStack[0] = root;
        int stackTop = 1;
        int islandNb = 0;
        int islandNj = 0;
        int islandNc = 0;
        int queueCount = 0;

        while (stackTop > 0) {
            BodyImpl* body = islandStack[--stackTop];
            bodies[islandNb++] = body;

            for (JointNode* jNode = body->firstJoint; jNode != nullptr; jNode = jNode->next) {
                JointImpl* joint = jNode->joint;
                if ((joint->flags & JointImpl::FLAG_TAG1) != 0) {
                    continue;
                }
                joint->flags |= JointImpl::FLAG_TAG1;
                joints[islandNj++] = joint;
                VisitConnectedBody(body, jNode->otherBody, islandStack, stackTop, islandQueue, queueCount);
            }

            for (ContactNode* cNode = body->firstContact; cNode != nullptr; cNode = cNode->next) {
                ContactJoint* cj = cNode->joint;
                if ((cj->flags & kContactFlagVisited) != 0) {
                    continue;
                }
                cj->flags |= kContactFlagVisited;
                contactJoints[islandNc++] = cj;

                BodyImpl* otherBody = ((cj->flags & kContactFlagHasBody1) != 0) ? cNode->otherBody : nullptr;
                VisitConnectedBody(body, otherBody, islandStack, stackTop, islandQueue, queueCount);
            }
        }

        TimeStep(islandNb, islandNj, islandNc, dt, 0);
        UpdateIslandSleep(bodies, islandNb, dt, sleepTimerInit, autoSleep);
    }
}
