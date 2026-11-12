#pragma once

#ifndef GAME_SPLINE_H
#define GAME_SPLINE_H

#include "cdc/runtime/cdcMath/Math.h"
#include <cstddef>
#include <cstdint>

// 0x6ce5 in PDB / size: 0x40 (64 bytes)
struct SplineKey {
    /* 0x00 */ cdc::Vector3 point;
    /* 0x10 */ cdc::Vector3 d1;
    /* 0x20 */ cdc::Vector3 d2;
    /* 0x30 */ float t0;
    /* 0x34 */ float tf;
    /* 0x38 */ float count;
    /* 0x3c */ float invCount;
};
static_assert(sizeof(SplineKey) == 0x40, "SplineKey size mismatch");

// size: 0x20 (32 bytes)
struct RSplineKey {
    /* 0x00 */ cdc::Quat q;
    /* 0x10 */ float t0;
    /* 0x14 */ float tf;
    /* 0x18 */ float count;
    /* 0x1c */ float invCount;
};
static_assert(sizeof(RSplineKey) == 0x20, "RSplineKey size mismatch");

// size: 0x08 (8 bytes)
struct SplineState {
    /* 0x00 */ float t;
    /* 0x04 */ uint16_t currKey;
    /* 0x06 */ uint16_t pad06;
};
static_assert(sizeof(SplineState) == 0x08, "SplineState size mismatch");

// Header: 0x10 bytes, total with keys[1]: 0x50 bytes
struct Spline {
    /* 0x00 */ uint16_t numKeys;
    /* 0x02 */ uint8_t type;
    /* 0x03 */ uint8_t flags;
    /* 0x04 */ float count;
    /* 0x08 */ uint8_t pad08[8];
    /* 0x10 */ SplineKey keys[1];

    // 0x00467d50 (PDB: 0x00469710 / ?GetPoint@Spline@@QBE?AVVector3@cdc@@GM@Z)
    cdc::Vector3 GetPoint(uint16_t key, float t) const;
    // 0x00468990 (PDB: 0x00469990 / ?GetPoint@Spline@@QBE?AVVector3@cdc@@ABUSplineState@@@Z)
    cdc::Vector3 GetPoint(const SplineState& state) const;
};
static_assert(offsetof(Spline, keys) == 0x10, "Spline::keys offset mismatch");
static_assert(sizeof(Spline) == 0x50, "Spline size mismatch");

// Header: 0x10 bytes, total with keys[1]: 0x30 bytes
struct RSpline {
    /* 0x00 */ uint16_t numKeys;
    /* 0x02 */ uint8_t type;
    /* 0x03 */ uint8_t flags;
    /* 0x04 */ float count;
    /* 0x08 */ uint8_t pad08[8];
    /* 0x10 */ RSplineKey keys[1];
};
static_assert(offsetof(RSpline, keys) == 0x10, "RSpline::keys offset mismatch");
static_assert(sizeof(RSpline) == 0x30, "RSpline size mismatch");

// size: 0x70 (112 bytes)
struct MultiSpline {
    /* 0x00 */ cdc::Matrix curRotMatrix;
    /* 0x40 */ Spline* positional;
    /* 0x44 */ RSpline* rotational;
    /* 0x48 */ Spline* scaling;
    /* 0x4c */ SplineState curPositional;
    /* 0x54 */ SplineState curRotational;
    /* 0x5c */ SplineState curScaling;
    /* 0x64 */ uint8_t pad64[12];

    // 0x0042b8d0 (PDB: 0x0042c6b0 / ?GetPoint@MultiSpline@@QBE?AVVector3@cdc@@XZ)
    cdc::Vector3 GetPoint() const;
    // 0x004682c0 (PDB: 0x00469c80 / ?GetNumFrames@MultiSpline@@QBEMXZ)
    float GetNumFrames() const;
    // 0x00469160 (PDB: 0x0046ab20 / ?SetFrameNumber@MultiSpline@@QAEXM@Z)
    void SetFrameNumber(float frame);
};
static_assert(sizeof(MultiSpline) == 0x70, "MultiSpline size mismatch");

#endif // GAME_SPLINE_H
