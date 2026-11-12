#ifndef GAME_GLOBALINFO_H
#define GAME_GLOBALINFO_H

#include <cstdint>

// Forward declarations
struct PlayerObjects;
struct MenuItems;
struct UIFadeGroupData;
struct ChapterSpec;
struct SoundEffectsOBSOLETE;
struct MenuPositions;
struct HardCodedHints;
struct TitleScreen;
struct DarkChronicle;
struct UnlocksTable;
struct ActionNames;
struct MovieRef;
struct PlayerLightList;
struct WetDripDefaults;
struct SuperActionDefaults;
struct InputConfigs;
struct FontList;
struct GameDifficultySettings;
struct RADInfo;
struct SoundGroup;
struct SoundGroupAssignmentsOBSOLETE;

// Size: 0x108 bytes (264 bytes, TI: 0x6d66)
struct CameraDefaults {
    /* 0x000 */ float auto_center_multiplier;
    /* 0x004 */ float auto_center_max_speed;
    /* 0x008 */ float rightstick_auto_center_disable_frames;
    /* 0x00c */ float rightstick_auto_center_disable_return;
    /* 0x010 */ float lead_amount;
    /* 0x014 */ float vertical_bias;
    /* 0x018 */ float focus_height;
    /* 0x01c */ float zoomed_in_focus_height;
    /* 0x020 */ float focus_height_combat;
    /* 0x024 */ float high_distance;
    /* 0x028 */ float medium_distance;
    /* 0x02c */ float zero_distance;
    /* 0x030 */ float low_distance;
    /* 0x034 */ float high_tilt_angle;
    /* 0x038 */ float medium_tilt_angle;
    /* 0x03c */ float zero_tilt_angle;
    /* 0x040 */ float low_tilt_angle;
    /* 0x044 */ float high_stop_tilt_adjust_speed;
    /* 0x048 */ float high_tilt_adjust_speed;
    /* 0x04c */ float med_tilt_adjust_speed;
    /* 0x050 */ float zero_tilt_adjust_speed;
    /* 0x054 */ float low_tilt_adjust_speed;
    /* 0x058 */ float low_stop_tilt_adjust_speed;
    /* 0x05c */ float spring_degree_upper;
    /* 0x060 */ float spring_degree_lower;
    /* 0x064 */ float toward_dead_zone_start;
    /* 0x068 */ float toward_dead_zone_end;
    /* 0x06c */ float close_distance_dead;
    /* 0x070 */ float close_min_distance_dead;
    /* 0x074 */ float away_dead_zone_start;
    /* 0x078 */ float away_dead_zone_end;
    /* 0x07c */ float dead_to_dead_mult;
    /* 0x080 */ float dead_to_dead_start;
    /* 0x084 */ float combat_camera_distance;
    /* 0x088 */ float combat_camera_zoffset;
    /* 0x08c */ float combat_camera_vbias;
    /* 0x090 */ float combat_camera_tilt;
    /* 0x094 */ float combat_camera_rotspeed;
    /* 0x098 */ float combat_camera_rotdamp;
    /* 0x09c */ float combat_camera_rotation_offset;
    /* 0x0a0 */ float combat_camera_melee_rotation_offset;
    /* 0x0a4 */ float max_binoc_width;
    /* 0x0a8 */ float min_binoc_width;
    /* 0x0ac */ float binoc_max_speed;
    /* 0x0b0 */ float binoc_acc;
    /* 0x0b4 */ float binoc_start_blur;
    /* 0x0b8 */ float binoc_min_blur;
    /* 0x0bc */ float binoc_max_blur;
    /* 0x0c0 */ float binoc_blur_adjust;
    /* 0x0c4 */ int globalDefaultCamera;
    /* 0x0c8 */ int turretCamera;
    /* 0x0cc */ int ropeCamera;
    /* 0x0d0 */ int ledgeCamera;
    /* 0x0d4 */ int thrownCamera;
    /* 0x0d8 */ int horizPoleCamera;
    /* 0x0dc */ int vertPoleCamera;
    /* 0x0e0 */ int ladderCamera;
    /* 0x0e4 */ int vehicleCamera;
    /* 0x0e8 */ int ziplineCamera;
    /* 0x0ec */ int crawlCamera;
    /* 0x0f0 */ int pushPullCamera;
    /* 0x0f4 */ int skydiveCamera;
    /* 0x0f8 */ int swimTreadCamera;
    /* 0x0fc */ int swimCamera;
    /* 0x100 */ int idleCamera;
    /* 0x104 */ int deathCamera;
};
static_assert(sizeof(CameraDefaults) == 0x108, "CameraDefaults size mismatch");

// Partial retail layout (PDB TI: 0x6dbf shows cameraDefaults at 0x5c; retail trl.exe accesses cameraDefaults at +0x64)
struct GlobalInfo {
    /* 0x000 */ void* serializationInfo;
    /* 0x004 */ PlayerObjects* playerObjects;
    /* 0x008 */ MenuItems* menuItems;
    /* 0x00c */ int fadeGroupCount;
    /* 0x010 */ UIFadeGroupData** fadeGroupData;
    /* 0x014 */ int chapterCount;
    /* 0x018 */ ChapterSpec* chapterList;
    /* 0x01c */ SoundEffectsOBSOLETE* soundEffectsOBSOLETE;
    /* 0x020 */ void* keyboardLocals;
    /* 0x024 */ MenuPositions* menuPositionsPS2;
    /* 0x028 */ MenuPositions* menuPositionsXBox;
    /* 0x02c */ MenuPositions* menuPositionsPC;
    /* 0x030 */ float automaticInstanceRemovalFloor;
    /* 0x034 */ HardCodedHints* hardCodedHints;
    /* 0x038 */ TitleScreen* titleScreen;
    /* 0x03c */ int numDarkChronicles;
    /* 0x040 */ DarkChronicle* darkChronicleList;
    /* 0x044 */ UnlocksTable* unlockableTable;
    /* 0x048 */ uint16_t costumeChangeEventVar;
    /* 0x04a */ uint16_t samplerDemoEventVar;
    /* 0x04c */ ActionNames* actionNames;
    /* 0x050 */ int attractMovieCount;
    /* 0x054 */ MovieRef* attractMovies;
    /* 0x058 */ PlayerLightList* playerLights;
    /* 0x05c */ uint8_t retailPad5c[8];
    /* 0x064 */ CameraDefaults* cameraDefaults;
    /* 0x068 */ WetDripDefaults* wetDripDefaults;
    /* 0x06c */ SuperActionDefaults* superActionDefaults;
    /* 0x070 */ InputConfigs* inputconfigs;
    /* 0x074 */ FontList* fontList;
    /* 0x078 */ GameDifficultySettings* gameDifficulty;
    /* 0x07c */ int cheatCodesDTPID;
    /* 0x080 */ RADInfo* pRADInfo;
    /* 0x084 */ float percentCompleteRatio;
    /* 0x088 */ int soundGroupCountOBSOLETE;
    /* 0x08c */ SoundGroup* soundGroupOBSOLETE;
    /* 0x090 */ SoundGroupAssignmentsOBSOLETE* soundGroupAssignmentsOBSOLETE;
};

extern GlobalInfo* globalInfo; // 0x010c0998 (PDB: 0x010c16d8)

#endif // GAME_GLOBALINFO_H
