#pragma once

#ifndef GAME_RAGDOLL_H
#define GAME_RAGDOLL_H

#include <cstddef>
#include <cstdint>
#include "cdc/runtime/cdcMath/Math.h"

struct Body {
    uint8_t pad00[0x80];
    /* 0x080 */ cdc::Vector3 m_position;
    /* 0x090 */ cdc::Quat m_quaternion;

    // 0x00503aa0 (PDB: 0x00505740)
    cdc::Vector3 GetPosition() const { return m_position; }
    // 0x00503ad0 (PDB: 0x00505770)
    cdc::Quat GetQuaternion() const { return m_quaternion; }
};

static_assert(offsetof(Body, m_position) == 0x80, "Body::m_position offset mismatch");
static_assert(offsetof(Body, m_quaternion) == 0x90, "Body::m_quaternion offset mismatch");

// Size: 0xc0 (TI: 0x7b04)
struct RagdollBone {
    uint8_t pad00[0x7c];
    /* 0x07c */ int m_parent;
    uint8_t pad80[0x2c];
    /* 0x0ac */ Body* m_body;
    uint8_t padb0[0x10];
};

static_assert(sizeof(RagdollBone) == 0xc0, "RagdollBone size mismatch");
static_assert(offsetof(RagdollBone, m_parent) == 0x7c, "RagdollBone::m_parent offset mismatch");
static_assert(offsetof(RagdollBone, m_body) == 0xac, "RagdollBone::m_body offset mismatch");

// Size: 0xb70 (TI: 0x7d4d)
struct Ragdoll {
    /* 0x000 */ RagdollBone m_bones[15];
    uint8_t padb40[0x2c];
    /* 0xb6c */ int m_numBones;

    // 0x0051d1e0 (PDB: 0x0051ed10)
    RagdollBone* GetBone(int index) { return &m_bones[index]; }
};

static_assert(sizeof(Ragdoll) == 0xb70, "Ragdoll size mismatch");
static_assert(offsetof(Ragdoll, m_numBones) == 0xb6c, "Ragdoll::m_numBones offset mismatch");

// Size: 0x20 (TI: 0x888a)
struct RagdollClientBone {
    /* 0x00 */ int segment;
    uint8_t pad04[0x0c];
    /* 0x10 */ cdc::Vector3 offset;
};

static_assert(sizeof(RagdollClientBone) == 0x20, "RagdollClientBone size mismatch");
static_assert(offsetof(RagdollClientBone, offset) == 0x10, "RagdollClientBone::offset offset mismatch");

// Size: 0x130 (TI: 0x88bc)
struct RagdollClient {
    uint8_t pad000[0xf0];
    /* 0x0f0 */ int m_boneCount;
    /* 0x0f4 */ RagdollClientBone* m_bones;
    uint8_t pad0f8[0x08];
    /* 0x100 */ cdc::Vector3 m_rootOffset1;
    /* 0x110 */ cdc::Vector3 m_rootOffset2;
    /* 0x120 */ Ragdoll* m_ragdoll;
    uint8_t pad124[0x0c];
};

static_assert(sizeof(RagdollClient) == 0x130, "RagdollClient size mismatch");
static_assert(offsetof(RagdollClient, m_boneCount) == 0xf0, "RagdollClient::m_boneCount offset mismatch");
static_assert(offsetof(RagdollClient, m_bones) == 0xf4, "RagdollClient::m_bones offset mismatch");
static_assert(offsetof(RagdollClient, m_rootOffset1) == 0x100, "RagdollClient::m_rootOffset1 offset mismatch");
static_assert(offsetof(RagdollClient, m_rootOffset2) == 0x110, "RagdollClient::m_rootOffset2 offset mismatch");
static_assert(offsetof(RagdollClient, m_ragdoll) == 0x120, "RagdollClient::m_ragdoll offset mismatch");

#endif // GAME_RAGDOLL_H
