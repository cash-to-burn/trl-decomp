#ifndef GAME_PLAYER_PLAYER_H
#define GAME_PLAYER_PLAYER_H

#include "cdc/runtime/cdcMath/Math.h"
#include "game/instance.h"
#include "game/gameloop.h"
#include <cstdint>

// Forward declarations
struct StateObj;
struct StateMachine;
struct AnimBlend;
struct TuneData;
struct Idle;
struct __AttackItem;
struct FatalityInterface;
struct InterpoBlendAnim;
struct NoHitBoneFXSegments;
struct TargetingInfo;
struct LayeredSound;
struct ProxyShape;
struct AnimFragmentTable;
struct TargetingSearch;
struct ControlMode;
struct Body;
struct Geometry;
struct SAnim;
struct MarkUpBox;
struct SMeleeMove;
struct CCombatReticle;
struct SenseTarget;
struct RNodeController;
struct FootLockController;
struct HandLockController;

#pragma pack(push, 1)

// Size: 0x08 bytes (TI: 0x64ed)
struct MessageQueue {
    void* head;
    void* tail;
};

// Size: 0x34 bytes (52 bytes, TI: 0x6974)
struct State {
    /* 0x000 */ StateObj* state;
    /* 0x004 */ StateObj* laststate;
    /* 0x008 */ StateMachine* stateMachine;
    /* 0x00c */ MessageQueue Event;
    /* 0x014 */ MessageQueue Defer;
    /* 0x01c */ int Data1;
    /* 0x020 */ int Data2;
    /* 0x024 */ int vanimIndex;
    /* 0x028 */ AnimBlend* vanim;
    /* 0x02c */ int frame;
    /* 0x030 */ int anim;
};

// Size: 0x38 bytes (56 bytes, TI: 0x6521)
struct CharacterState {
    /* 0x000 */ Instance* instance;
    /* 0x004 */ State playerState;
};

// Size: 0x0c bytes (12 bytes, TI: 0x657d)
struct HealthSystem {
    /* 0x000 */ float hitPoints;
    /* 0x004 */ float maxHitPoints;
    /* 0x008 */ float damageFrequency;
};

// Size: 0x14 bytes (20 bytes, TI: 0x6322)
struct SplashSystem {
    /* 0x000 */ void* splashHandler;
    /* 0x004 */ void* invSplashHandler;
    /* 0x008 */ unsigned char MaxXYDist;
    /* 0x009 */ unsigned char pad09;
    /* 0x00a */ unsigned char MaxZDist;
    /* 0x00b */ unsigned char pad0b;
    /* 0x00c */ unsigned char yCone;
    /* 0x00d */ unsigned char pad0d;
    /* 0x00e */ unsigned char zCone;
    /* 0x00f */ unsigned char pad0f;
    /* 0x010 */ int numSplash;
};

// Size: 0x10 bytes (16 bytes, TI: 0x631a)
struct AlgoSystem {
    /* 0x000 */ float horizoffset;
    /* 0x004 */ float vertoffset;
    /* 0x008 */ float maxuprot;
    /* 0x00c */ float maxdownrot;
};

// Size: 0x30 bytes (48 bytes, TI: 0x63d6)
struct PlayerHoldInfo {
    /* 0x000 */ Instance* mainInstance;
    /* 0x004 */ BaseInstance* bodyInstance;
    /* 0x008 */ Body* body;
    /* 0x00c */ Geometry* geom;
    /* 0x010 */ cdc::Vector3 bodyGrabPoint;
    /* 0x020 */ cdc::Vector3 bodyGrabNormal;
};

// Size: 0x28 bytes (40 bytes, TI: 0x7785)
struct GeneralAttributes {
    /* 0x000 */ float height;
    /* 0x004 */ float crouchHeight;
    /* 0x008 */ float ankleUpperOffset;
    /* 0x00c */ float ankleLowerOffset;
    /* 0x010 */ float ankleLength;
    /* 0x014 */ float mass;
    /* 0x018 */ float leftFootLockHipLimitMin;
    /* 0x01c */ float leftFootLockHipLimitMax;
    /* 0x020 */ float rightFootLockHipLimitMin;
    /* 0x024 */ float rightFootLockHipLimitMax;
};

// Size: 0xc68 bytes (3176 bytes, TI: 0x774e)
struct OldRazielData {
    /* 0x000 */ ControlMode* controlModeList;
    /* 0x004 */ float CntrlZoneHist;
    /* 0x008 */ float analogWalkSpeed;
    /* 0x00c */ float runSpeed;
    /* 0x010 */ float UpSlopeSpeed;
    /* 0x014 */ float DownSlopeSpeed;
    /* 0x018 */ float runAccel;
    /* 0x01c */ float runToWalkSpeed;
    /* 0x020 */ float walkToRunSpeed;
    /* 0x024 */ float startRunSpeed;
    /* 0x028 */ float walkRunTransFrames;
    /* 0x02c */ float autofaceSpeed;
    /* 0x030 */ float slideLandDecel;
    /* 0x034 */ Idle* idleList;
    /* 0x038 */ unsigned int idleListCount;
    /* 0x03c */ __AttackItem*** attackList;
    /* 0x040 */ void* animPatternList;
    /* 0x044 */ FatalityInterface*** grabList;
    /* 0x048 */ void* animList;
    /* 0x04c */ InterpoBlendAnim*** interpoBlendList;
    /* 0x050 */ unsigned int interpoBlendListCount;
    /* 0x054 */ int EarnedAbilities;
    /* 0x058 */ NoHitBoneFXSegments* noHitBoneFXSegmentsList;
    /* 0x05c */ short numNoHitBoneFXSegments;
    /* 0x05e */ short pad5e;
    /* 0x060 */ void* hudQuads;
    /* 0x064 */ short numHudQuads;
    /* 0x066 */ short pad66;
    /* 0x068 */ TargetingInfo* targetinginfo;
    /* 0x06c */ void* hudHealth;
    /* 0x070 */ void* hudWeapon;
    /* 0x074 */ float HealthInitial;
    /* 0x078 */ float LowHealth;
    /* 0x07c */ unsigned char HealthChunkRegenTime;
    /* 0x07d */ unsigned char pad7d;
    /* 0x07e */ signed char NumHealthChunks;
    /* 0x07f */ signed char bDebugDontDie;
    /* 0x080 */ float fDamageFallDist;
    /* 0x084 */ float fHalfHealthFallDist;
    /* 0x088 */ float fDeathFallDist;
    /* 0x08c */ float fSaveFallDamageMult;
    /* 0x090 */ int iSaveFallButtonWindow;
    /* 0x094 */ int dInvincibilityGracePeriod;
    /* 0x098 */ unsigned char dDamageHeavyThreshold;
    /* 0x099 */ unsigned char pad99;
    /* 0x09a */ unsigned char dDamageMediumThreshold;
    /* 0x09b */ unsigned char pad9b;
    /* 0x09c */ unsigned short uMinRespawnHealthpackCount;
    /* 0x09e */ unsigned short uPad16;
    /* 0x0a0 */ unsigned short uHideGear;
    /* 0x0a2 */ unsigned short uPad16b;
    /* 0x0a4 */ LayeredSound* soundRagdoll;
    /* 0x0a8 */ unsigned int uWaterSurfacingGaspSound;
    /* 0x0ac */ unsigned int uWaterSurfacingOkSound;
    /* 0x0b0 */ ProxyShape* SwimProxyParams;
    /* 0x0b4 */ ProxyShape* TreadProxyParams;
    /* 0x0b8 */ ProxyShape* ClimbOutProxyParams;
    /* 0x0bc */ float SwimPhysicsFallDamping;
    /* 0x0c0 */ float SwimPhysicsWaterFrequency;
    /* 0x0c4 */ float SwimPhysicsWaterAmplitude;
    /* 0x0c8 */ float SwimPhysicsUnderAcceleration;
    /* 0x0cc */ float SwimPhysicsUnderDeceleration;
    /* 0x0d0 */ float SwimPhysicsUnderKickVelocity;
    /* 0x0d4 */ float SwimPhysicsUnderKickAccel;
    /* 0x0d8 */ float SwimPhysicsUnderVelocity;
    /* 0x0dc */ float SwimPhysicsUnderKickDecel;
    /* 0x0e0 */ float SwimPhysicsBobUpVelocity;
    /* 0x0e4 */ float SwimPhysicsBobUpAcceleration;
    /* 0x0e8 */ float SwimPhysicsSurfVelocity;
    /* 0x0ec */ float SwimPhysicsSurfAccelerationIn;
    /* 0x0f0 */ float SwimPhysicsSurfAccelerationOut;
    /* 0x0f4 */ float SwimPhysicsSurfKickVelocity;
    /* 0x0f8 */ float SwimPhysicsSurfKickAccel;
    /* 0x0fc */ float SwimPhysicsSurfMinRotation;
    /* 0x100 */ float SwimPhysicsSurfMaxRotation;
    /* 0x104 */ float SwimPhysicsSurfKickDecel;
    /* 0x108 */ uint8_t pad_data[0xc68 - 0x108];
};

// Size: 0xc98 bytes (3224 bytes, TI: 0x7eab)
struct PlayerProp {
    /* 0x000 */ unsigned int playerVersion;
    /* 0x004 */ TuneData* tuneData;
    /* 0x008 */ GeneralAttributes general;
    /* 0x030 */ OldRazielData oldData;
};

// Size: 0x1000 bytes (4096 bytes, TI: 0x66db)
struct Player {
    /* 0x000 */ uint8_t smoothRideState[0x14]; // SmoothRideState (20 bytes)
    /* 0x014 */ int footPrint;
    /* 0x018 */ int frame;
    /* 0x01c */ int anim;
    /* 0x020 */ int vanim;
    /* 0x024 */ int Mode;
    /* 0x028 */ int jumpMode;
    /* 0x02c */ int jumpSuccess;
    /* 0x030 */ int jumpCombos;
    /* 0x034 */ bool mirrorMode;
    /* 0x035 */ bool CriticalSection;
    /* 0x036 */ uint8_t pad36[2];
    /* 0x038 */ unsigned int uInvState;
    /* 0x03c */ int RotationSegment;
    /* 0x040 */ cdc::Vector3 iVelocity;
    /* 0x050 */ cdc::Vector3 worldMomentum;
    /* 0x060 */ float worldMomentumFriction;
    /* 0x064 */ float Magnitude;
    /* 0x068 */ float ZDirection;
    /* 0x06c */ float Bearing;
    /* 0x070 */ float LastBearing;
    /* 0x074 */ int steeringMode;
    /* 0x078 */ float steeringVelocity;
    /* 0x07c */ float steeringAcceleration;
    /* 0x080 */ float steeringLockRotation;
    /* 0x084 */ float steeringDestRotation;
    /* 0x088 */ float movementMinRate;
    /* 0x08c */ float movementMaxRate;
    /* 0x090 */ float movementMinAnalog;
    /* 0x094 */ float movementMaxAnalog;
    /* 0x098 */ float movementMaxSpeed;
    /* 0x09c */ float gotHitTimer;
    /* 0x0a0 */ float fWeaponSwitchTimer;
    /* 0x0a4 */ float timeMultMultiplier;
    /* 0x0a8 */ uint8_t padA8[8];
    /* 0x0b0 */ cdc::Matrix spatialMatrix;
    /* 0x0f0 */ uint8_t sensoryData[0x190];     // SensoryData (0x190 bytes)
    /* 0x280 */ BaseInstance* attachedPlatform;
    /* 0x284 */ unsigned char dropOffHeight;
    /* 0x285 */ uint8_t pad285[3];
    /* 0x288 */ float fallZVelocity;
    /* 0x28c */ uint8_t leftLeg[0x68];          // LegIK (0x68 bytes)
    /* 0x2f4 */ uint8_t rightLeg[0x68];         // LegIK (0x68 bytes)
    /* 0x35c */ int smoothMode;
    /* 0x360 */ int lockSegment;
    /* 0x364 */ int collideRadius;
    /* 0x368 */ uint8_t pad368[8];
    /* 0x370 */ cdc::Vector3 rNodeTranslation;
    /* 0x380 */ cdc::Vector3 rNodeTransSlewing;
    /* 0x390 */ cdc::Vector3 rNodeTransLeaning;
    /* 0x3a0 */ float rNodeLeanAmount;
    /* 0x3a4 */ uint8_t invLists[60];           // SInvList[60]
    /* 0x3e0 */ signed char iCurItem;
    /* 0x3e1 */ signed char iCurWeapon;
    /* 0x3e2 */ signed char iCurGear;
    /* 0x3e3 */ signed char iLastGearSubType;
    /* 0x3e4 */ signed char iCurWeaponSubType;
    /* 0x3e5 */ signed char iNextWeapon;
    /* 0x3e6 */ signed char iCombatLockMode;
    /* 0x3e7 */ uint8_t pad3e7;
    /* 0x3e8 */ uint8_t invItems[88];           // SInvItem[88]
    /* 0x440 */ unsigned int uCombatFlags;
    /* 0x444 */ unsigned short uAmmoBank[9];
    /* 0x456 */ uint8_t pad456[10];
    /* 0x460 */ cdc::Vector3 alignOffset;
    /* 0x470 */ cdc::Vector3 alignStartPoint;
    /* 0x480 */ uint8_t forcedTarget[0x14];     // SenseTarget (0x14 bytes)
    /* 0x494 */ SenseTarget* pAimAssistTarget;
    /* 0x498 */ uint8_t pad498[8];
    /* 0x4a0 */ cdc::Euler vAimAssistOldRot;
    /* 0x4b0 */ cdc::Euler vAimAssistOldForce;
    /* 0x4c0 */ void* lockCameraTarget;
    /* 0x4c4 */ BaseInstance* pAccAimVictim;
    /* 0x4c8 */ float fAccAimDist;
    /* 0x4cc */ float fHolsterTimer;
    /* 0x4d0 */ float fAccAimSwingTimer;
    /* 0x4d4 */ float fLoseLockTimer;
    /* 0x4d8 */ uint8_t pad4d8[8];
    /* 0x4e0 */ float vCombatFlick[2];
    /* 0x4e8 */ uint8_t pad4e8[8];
    /* 0x4f0 */ cdc::Vector3 lastBulletHitPos;
    /* 0x500 */ SenseTarget* pPrevGrappleTarget;
    /* 0x504 */ float fGrappleBlingTimer;
    /* 0x508 */ Instance* pLastAttackingWeapon;
    /* 0x50c */ int eLastZoomMode;
    /* 0x510 */ uint8_t meleeData[0x20];        // SMeleeData (0x20 bytes)
    /* 0x530 */ SMeleeMove* pMeleeMove;
    /* 0x534 */ bool bTargetedCombatJump;
    /* 0x535 */ bool bMeleeOK;
    /* 0x536 */ uint8_t pad536[2];
    /* 0x538 */ CCombatReticle* pCombatReticle;
    /* 0x53c */ float fAATransitionTimer;
    /* 0x540 */ unsigned int uAATimeTracker;
    /* 0x544 */ uint8_t pad544[4];
    /* 0x548 */ unsigned int uHealEffect;
    /* 0x54c */ float fHealTimer;
    /* 0x550 */ cdc::Vector3 wallJumpNormal;
    /* 0x560 */ cdc::Vector3 slopeNormal;
    /* 0x570 */ cdc::Vector3 floorNormal;
    /* 0x580 */ float speed;
    /* 0x584 */ float speedscale;
    /* 0x588 */ float flatspeed;
    /* 0x58c */ float speedBlend;
    /* 0x590 */ float speedBlendInc;
    /* 0x594 */ float runTime;
    /* 0x598 */ BaseInstance* standingUpon;
    /* 0x59c */ BaseInstance* lastStandingUpon;
    /* 0x5a0 */ float HitPoints;
    /* 0x5a4 */ float DamageFrequency;
    /* 0x5a8 */ float invincibleTimer;
    /* 0x5ac */ int invincibleAttackType;
    /* 0x5b0 */ int Abilities;
    /* 0x5b4 */ float slipSlope;
    /* 0x5b8 */ uint8_t checkPoint[0x18];       // CheckPoint
    /* 0x5d0 */ float idleTime;
    /* 0x5d4 */ Idle* currentIdle;
    /* 0x5d8 */ Idle* lastIdle;
    /* 0x5dc */ float lastIdleTime;
    /* 0x5e0 */ float fallZ;
    /* 0x5e4 */ uint8_t pad5e4[12];
    /* 0x5f0 */ cdc::Vector3 attachedMovement;
    /* 0x600 */ int puzzleAnim;
    /* 0x604 */ int puzzleMode;
    /* 0x608 */ __AttackItem* attack;
    /* 0x60c */ __AttackItem** attackSequence;
    /* 0x610 */ int currentAttack;
    /* 0x614 */ Instance* attackedBy;
    /* 0x618 */ FatalityInterface** fatalitySequence;
    /* 0x61c */ FatalityInterface* fatalityInterface;
    /* 0x620 */ short fatalityIndex;
    /* 0x622 */ short fatalityHost;
    /* 0x624 */ int fatality;
    /* 0x628 */ int blockDirection;
    /* 0x62c */ Instance* blockInstance;
    /* 0x630 */ Instance* hitSender;
    /* 0x634 */ int ignoreHit;
    /* 0x638 */ MessageQueue padCommands;
    /* 0x640 */ unsigned int passedMask;
    /* 0x644 */ uint8_t pad644[12];
    /* 0x650 */ cdc::Vector3 extraRot;
    /* 0x660 */ int blankPad;
    /* 0x664 */ int effectsFlags;
    /* 0x668 */ float effectsFadeSource;
    /* 0x66c */ float effectsFadeDest;
    /* 0x670 */ float effectsFadeStep;
    /* 0x674 */ float effectsFade;
    /* 0x678 */ uint8_t restOfPlayer[0x1000 - 0x678];
};

// Size: 0x1070 bytes (4208 bytes, TI: 0x65bb)
struct PlayerData {
    /* 0x000 */ CharacterState state;           // 56 bytes (0x000 - 0x038)
    /* 0x038 */ HealthSystem health;            // 12 bytes (0x038 - 0x044)
    /* 0x044 */ SplashSystem splash;            // 20 bytes (0x044 - 0x058)
    /* 0x058 */ AlgoSystem algo;                // 16 bytes (0x058 - 0x068)
    /* 0x068 */ uint8_t pad068[8];              // alignment padding
    /* 0x070 */ Player work;                    // 4096 bytes (0x070 - 0x1070)
};

// Size: 0x510 bytes (1296 bytes, TI: 0x6b3b)
struct PlayerSM {
    /* 0x000 */ uint8_t stateMachineBase[0x4c]; // StateMachine base (76 bytes)
    /* 0x04c */ uint8_t pad04c[0x14];
    /* 0x060 */ cdc::Matrix vehicleMatrix;      // 0x60 - 0xa0
    /* 0x0a0 */ uint8_t pad0a0[0x30];
    /* 0x0d0 */ signed char state;              // 0x0d0
    /* 0x0d1 */ uint8_t pad0d1[0x510 - 0x0d1];
};

#pragma pack(pop)

// Exact memory layout static assertions against PDB
static_assert(sizeof(MessageQueue) == 0x08, "MessageQueue size mismatch");
static_assert(sizeof(State) == 0x34, "State size mismatch");
static_assert(sizeof(CharacterState) == 0x38, "CharacterState size mismatch");
static_assert(sizeof(HealthSystem) == 0x0c, "HealthSystem size mismatch");
static_assert(sizeof(SplashSystem) == 0x14, "SplashSystem size mismatch");
static_assert(sizeof(AlgoSystem) == 0x10, "AlgoSystem size mismatch");
static_assert(sizeof(PlayerHoldInfo) == 0x30, "PlayerHoldInfo size mismatch");
static_assert(sizeof(GeneralAttributes) == 0x28, "GeneralAttributes size mismatch");
static_assert(sizeof(OldRazielData) == 0xc68, "OldRazielData size mismatch");
static_assert(sizeof(PlayerProp) == 0xc98, "PlayerProp size mismatch");
static_assert(sizeof(Player) == 0x1000, "Player size mismatch");
static_assert(sizeof(PlayerData) == 0x1070, "PlayerData size mismatch");
static_assert(sizeof(PlayerSM) == 0x510, "PlayerSM size mismatch");

// Subsystem globals
extern Player* gPlayer;                         // 0x0111707c (PDB: 0x0111822c)
extern PlayerSM* gPlayerSM;                     // 0x01117494 (PDB: 0x01118644)
extern PlayerProp* playerData;                  // 0x01116f90 (PDB: 0x01118140)
extern Instance* s_playerInstance;              // 0x010e52bc (PDB: 0x010e646c)
extern float s_timeScale;                       // 0x010e53d8

// Core object callback functions for "lara", "lara_winter", "amanda_player", etc.
void RazielInit(Instance* instance, GameTracker* gameTracker);             // 0x0059d5b0 (PDB: 0x0059fe40)
void RazielProcess(Instance* instance, GameTracker* gameTracker);          // 0x0059aa90 (PDB: 0x0059d310)
void RazielCollide(Instance* instance, GameTracker* gameTracker);          // 0x00565ce0 (PDB: 0x00567fc0)
unsigned int RazielQuery(Instance* instance, unsigned int query);         // 0x0058a3e0 (PDB: 0x0058ca40)
void RazielPost(Instance* instance, unsigned int message, unsigned int data); // 0x00595060 (PDB: 0x005978e0)
void RazielAdditionalCollide(Instance* instance, GameTracker* gameTracker);// 0x0058b060 (PDB: 0x0058d6c0)

// Player control helper routines
void RazielClearData(Instance* instance);                                  // 0x00565cd0 (PDB: 0x00567fb0)
void RAZIEL_SetLookAround(Instance* instance);                             // 0x00565ed0 (PDB: 0x005681b0)
void RAZIEL_ResetLookAround(Instance* instance);                           // 0x00565f40 (PDB: 0x00568220)
bool RAZIEL_OkToLookAround(Instance* instance);                            // 0x00565fba (PDB: 0x005682a0)
void RAZIEL_DebugHealthFillUp(void);                                       // 0x00579140 (PDB: 0x00579140)
void RAZIEL_DebugHurtRaziel(void);                                         // 0x005aa7b0 (PDB: 0x005aa7b0)
void RAZIEL_TurnHead(Instance* instance);                                  // 0x0057d930 (PDB: 0x0057d930)

#endif // GAME_PLAYER_PLAYER_H
