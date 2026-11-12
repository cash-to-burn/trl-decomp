#include "game/gameloop.h"
#include "game/main.h"
#include "game/Stream/stream.h"
#include "game/pc/mainvm.h"
#include "game/pc/timer.h"
#include <cstdio>
#include <cstring>
#include <cmath>

static int s_loadingDisplayActive = 0; // 0x11166e0 (s_loadingIcon)

// 0x0044f5d0 (PDB: 0x00450950)
void GAMELOOP_AllocStaticMemory(void)
{
    // Allocates static vertex pool and system buffers
}

// 0x0044f630 (PDB: 0x004509b0)
void GAMELOOP_InitGameTracker(void)
{
    memset(&g_gameTracker, 0, sizeof(GameTracker));
    g_gameTracker.timeMult = 1.0f;
    g_gameTracker.actualTimeMult = 1.0f;
    g_gameTracker.globalTimeMult = 1.0f;
    g_gameTracker.unmodifiedActualTimeMult = 1.0f;
    g_gameTracker.timeDilation = 1.0f;
    g_gameTracker.gameMode = 2; // Default to running mode
}

// 0x0044f6d0 (PDB: 0x00450a50)
void GAMELOOP_SystemInit(GameTracker *gameTracker)
{
    if (gameTracker == nullptr) {
        return;
    }

    GAMELOOP_DisplayGameElements(gameTracker);
}

// 0x0044f710 (PDB: 0x00450a90)
void GAMELOOP_ResetGameStates(void)
{
    g_gameTracker.levelDone = 0;
    g_gameTracker.levelChange = 0;
    g_gameTracker.savingGame = 0;
}

// 0x0044f740 (PDB: 0x00450ac0)
void GAMELOOP_ClearGameTracker(void)
{
    g_gameTracker.block_collide_override = 0;
    g_gameTracker.raziel_collide_override = 0;
    g_gameTracker.sound_collide_override = 0;
    g_gameTracker.attackwave_collide_override = 0;
    g_gameTracker.monster_collide_override = 0;
    g_gameTracker.monster_land_collide_override = 0;
    g_gameTracker.physics_line_draw_override = 0;
}

// 0x0044f7e0 (PDB: 0x00450b60)
void GAMELOOP_IntroduceInstances(GameTracker *gameTracker)
{
    (void)gameTracker;
}

// 0x0044f820 (PDB: 0x00450ba0)
void GAMELOOP_LoadingDisplayProcess(void *instance, GameTracker *gameTracker)
{
    (void)instance;
    (void)gameTracker;
}

// 0x0044f830 (PDB: 0x00450bb0)
void GAMELOOP_InitRunning(char *baseAreaName, char *playerName)
{
    (void)playerName;
    GAMELOOP_ClearGameTracker();
    GAMELOOP_ResetAllScreenWipes();

    if (baseAreaName != nullptr && baseAreaName[0] != '\0') {
        GAMELOOP_LevelLoadAndInit(baseAreaName, &g_gameTracker);
    }
}

// 0x0044fa30 (PDB: 0x00450db0)
void GAMELOOP_CloseRunning(void)
{
    STREAM_DumpAllLevels(0, 1);
}

// 0x0044faa0 (PDB: 0x00450e30)
void GAMELOOP_DisplayRunning(StreamUnit *unit)
{
    (void)unit;
}

// 0x0044fba0 (PDB: 0x00450f30)
void GAMELOOP_InitStaticDefault(char *baseAreaname, char *playerName)
{
    (void)baseAreaname;
    (void)playerName;
}

// 0x0044fc50 (PDB: 0x00450fe0)
void GAMELOOP_InitStaticChapter(char *baseAreaname, char *playerName, int chapterIndex)
{
    (void)baseAreaname;
    (void)playerName;
    (void)chapterIndex;
}

// 0x0044fd40 (PDB: 0x004510d0)
void GAMELOOP_InitStaticDeath(char *baseAreaname, char *playerName)
{
    (void)baseAreaname;
    (void)playerName;
}

// 0x0044fd90 (PDB: 0x00451120)
void GAMELOOP_DisplayStatic(void)
{
}

// 0x0044fdc0 (PDB: 0x00451150)
void GAMELOOP_CloseStatic(void)
{
}

// 0x0044fd90 (PDB: 0x00451120)
void GAMELOOP_DisplayStaticChapter(void)
{
}

// 0x0044fde0 (PDB: 0x00451170)
void GAMELOOP_SetLoadingDisplay(int display)
{
    s_loadingDisplayActive = display;
}

// 0x0044fdf0 (PDB: 0x00451180)
void GAMELOOP_InitStandardObjects(void)
{
}

// 0x0044fe10 (PDB: 0x004511a0)
void *GAMELOOP_IntroducePlayerInstance(int unitID, int playerObjectID, void *intro)
{
    (void)unitID;
    (void)playerObjectID;
    (void)intro;
    return nullptr;
}

// 0x0044ff60 (PDB: 0x004512e0)
void GAMELOOP_ResetAllScreenWipes(void)
{
    for (int i = 0; i < 12; i++) {
        g_gameTracker.wipe[i].current = 0.0f;
        g_gameTracker.wipe[i].target = 0.0f;
        g_gameTracker.wipe[i].rate = 0.0f;
    }
}

// 0x0044ff80 (PDB: 0x00451300)
int GAMELOOP_IsWipeDone(int type)
{
    if (type < 0 || type >= 12) {
        return 1;
    }
    return (g_gameTracker.wipe[type].current == g_gameTracker.wipe[type].target) ? 1 : 0;
}

// 0x0044ffb0 (PDB: 0x00451330)
int GAMELOOP_IsWipeStartingOrON(int type)
{
    if (type < 0 || type >= 12) {
        return 0;
    }
    return (g_gameTracker.wipe[type].current != g_gameTracker.wipe[type].target) ? 1 : 0;
}

// 0x00450010 (PDB: 0x00451390)
int GAMELOOP_AreAnyScreenFadesOnOrStarting(void)
{
    for (int i = 0; i < 12; i++) {
        if (g_gameTracker.wipe[i].current != g_gameTracker.wipe[i].target) {
            return 1;
        }
    }
    return 0;
}

// 0x00450040 (PDB: 0x004513c0)
void GAMELOOP_SetScreenWipe(int type, int target, int time)
{
    if (type < 0 || type >= 12) {
        return;
    }

    g_gameTracker.wipe[type].target = (float)target;
    if (time <= 0) {
        g_gameTracker.wipe[type].current = (float)target;
        g_gameTracker.wipe[type].rate = 0.0f;
    } else {
        float diff = (float)target - g_gameTracker.wipe[type].current;
        g_gameTracker.wipe[type].rate = diff / (float)time;
    }
}

// 0x004500a0 (PDB: 0x00451420)
void GAMELOOP_HandleScreenWipes(void)
{
    for (int i = 0; i < 12; i++) {
        ScreenWipe *w = &g_gameTracker.wipe[i];
        if (w->rate != 0.0f) {
            w->current += w->rate;
            if ((w->rate > 0.0f && w->current >= w->target) ||
                (w->rate < 0.0f && w->current <= w->target)) {
                w->current = w->target;
                w->rate = 0.0f;
            }
        }
    }
}

// 0x004506f0 (PDB: 0x00451a70)
StreamUnit *GAMELOOP_GetMainRenderUnit(void)
{
    if (g_gameTracker.mainDrawUnit != nullptr) {
        return g_gameTracker.mainDrawUnit;
    }
    return &StreamUnitList[0];
}

// 0x00450770 (PDB: 0x00451af0)
bool GAMELOOP_DisplayGameElements(GameTracker *gameTracker)
{
    (void)gameTracker;
    GAMELOOP_HandleScreenWipes();
    return true;
}

// 0x00450a30 (PDB: 0x00451db0)
void GAMELOOP_DisplayFrame(GameTracker *gameTracker)
{
    (void)gameTracker;
    // Renders active 3D world and screen elements
}

// 0x00450a90 (PDB: 0x00451e10)
float TimeWarpRamp(float current, float target, float rate)
{
    if (current < target) {
        current += rate;
        if (current > target) current = target;
    } else if (current > target) {
        current -= rate;
        if (current < target) current = target;
    }
    return current;
}

// 0x00450ad0 (PDB: 0x00451e50)
void GAMELOOP_DoTimeProcess(void)
{
    uint32_t now = TIMER_GetTimeMS();
    if (g_gameTracker.lastLoopTime != 0) {
        uint32_t dt = now - g_gameTracker.lastLoopTime;
        g_gameTracker.timeSinceLastGameFrame = (int)dt;
    }
    g_gameTracker.lastLoopTime = now;
    g_gameTracker.totalTime += g_gameTracker.timeSinceLastGameFrame;
}

// 0x004511c0 (PDB: 0x00452540)
void GAMELOOP_ModeStartRunning(int wasSet)
{
    (void)wasSet;
    g_gameTracker.gameMode = 2;
}

// 0x00451270 (PDB: 0x004525f0)
void GAMELOOP_ModeStartPause(bool systemPause)
{
    (void)systemPause;
    g_gameTracker.gameMode = 4;
}

// 0x00451360 (PDB: 0x004526e0)
void GAMELOOP_ChangeMode(int newMode)
{
    g_gameTracker.nextGameMode = newMode;
    g_gameTracker.gameMode = (uint8_t)newMode;
}

// 0x00451680 (PDB: 0x00452a00)
void GAMELOOP_ExitGame(int doneType)
{
    (void)doneType;
    quitdone = 1;
}

// 0x00451780 (PDB: 0x00452b00)
void GAMELOOP_RequestLevelChangeByName(char *name, GameTracker *gameTracker, int doneType)
{
    (void)doneType;
    if (name != nullptr && gameTracker != nullptr) {
        strncpy(gameTracker->baseAreaName, name, sizeof(gameTracker->baseAreaName) - 1);
        gameTracker->levelChange = 1;
    }
}

// 0x004517e0 (PDB: 0x00452b60)
void GAMELOOP_RequestLevelChange(char *levelName, int number, GameTracker *gameTracker)
{
    (void)number;
    GAMELOOP_RequestLevelChangeByName(levelName, gameTracker, 0);
}

// 0x00451820 (PDB: 0x00452ba0)
void GAMELOOP_LoadingDisplay(void)
{
    GAMELOOP_HandleScreenWipes();
}

// 0x00451910 (PDB: 0x00452c90)
int GAMELOOP_LevelLoadAndInit(char *baseAreaName, GameTracker *gameTracker)
{
    if (baseAreaName == nullptr || gameTracker == nullptr) {
        return 0;
    }

    // 1. Begin asynchronous load of stream unit
    StreamUnit *unit = STREAM_LoadLevel(baseAreaName, nullptr, 0);
    if (unit == nullptr) {
        return 0;
    }

    // 2. Wait until load finishes
    STREAM_WaitForUnit(unit);

    // 3. Mark unit as active
    gameTracker->mainDrawUnit = unit;
    unit->used = 2; // Active

    return 1;
}

// 0x00451f90 (PDB: 0x00453310)
void GAMELOOP_Process(GameTracker *gameTracker)
{
    if (gameTracker == nullptr) {
        return;
    }

    // 1. Update time and frame counters
    GAMELOOP_DoTimeProcess();

    // 2. Poll streaming loader queue
    STREAM_PollLoadQueue();

    // 3. Mark current main unit as needed
    STREAM_MarkUnitNeeded(gameTracker->StreamUnitID);

    // 4. Dump stale units
    STREAM_DumpAllUnitsNotNeeded();
}

// 0x00452360 (PDB: 0x004536e0)
void GAMELOOP_MainLoop(GameTracker *gameTracker)
{
    if (gameTracker == nullptr) {
        return;
    }

    // 1. Simulation and streaming step
    GAMELOOP_Process(gameTracker);

    // 2. Render presentation step
    GAMELOOP_DisplayFrame(gameTracker);

    // 3. Post-render system updates
    GAMELOOP_SystemInit(gameTracker);

    // 4. Advance frame counter
    gameTracker->frameCount++;
}
