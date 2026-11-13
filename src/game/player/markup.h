#ifndef GAME_PLAYER_MARKUP_H
#define GAME_PLAYER_MARKUP_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/instance.h"
#include <cstdint>

struct PolyLine;
struct SAPBox;

#pragma pack(push, 1)

// Size: 0x30 bytes (TI: 0x5a9d)
struct MarkUp {
    /* 0x000 */ int32_t OverrideMovementCamera;
    /* 0x004 */ int32_t DTPCameraDataID;
    /* 0x008 */ int32_t DTPMarkupDataID;
    /* 0x00c */ uint32_t flags;
    /* 0x010 */ int16_t introID;
    /* 0x012 */ int16_t markupID;
    /* 0x014 */ float pos[3];
    /* 0x020 */ uint8_t bbox[12];
    /* 0x02c */ PolyLine* polyLine;
};

// Size: 0x40 bytes (TI: 0x6880)
struct MarkUpBox {
    /* 0x000 */ NodeType node;
    /* 0x008 */ SAPBox* sapbox;
    /* 0x00c */ BaseInstance* instance;
    /* 0x010 */ cdc::Vector3 velocity;
    /* 0x020 */ MarkUp* markup;
    /* 0x024 */ uint32_t flags;
    /* 0x028 */ float attach_delay;
    /* 0x02c */ float attach_frames;
    /* 0x030 */ uint32_t debugColor;
    /* 0x034 */ int nSegments;
    /* 0x038 */ void* associateRef;
    /* 0x03c */ uint32_t pad03c;
};

// Size: 0x0c bytes (TI: 0x8e40)
struct MarkupDTPCam {
    /* 0x000 */ int32_t DTPCameraDataID;
    /* 0x004 */ uint16_t startSegIdx;
    /* 0x006 */ uint16_t endSegIdx;
    /* 0x008 */ int8_t startSegOffset;
    /* 0x009 */ int8_t endSegOffset;
    /* 0x00a */ uint16_t flags;
};

// Size: 0x10 bytes (TI: 0x8c76)
struct PlayerMarkupCamInfo {
    /* 0x000 */ MarkUpBox* mub;
    /* 0x004 */ MarkupDTPCam* mkpCam;
    /* 0x008 */ int DTPCameraDataID;
    /* 0x00c */ int OverrideMovementCamera;
};

#pragma pack(pop)

static_assert(sizeof(MarkUp) == 0x30, "MarkUp size mismatch");
static_assert(sizeof(MarkUpBox) == 0x40, "MarkUpBox size mismatch");
static_assert(sizeof(MarkupDTPCam) == 0x0c, "MarkupDTPCam size mismatch");
static_assert(sizeof(PlayerMarkupCamInfo) == 0x10, "PlayerMarkupCamInfo size mismatch");

#endif // GAME_PLAYER_MARKUP_H
