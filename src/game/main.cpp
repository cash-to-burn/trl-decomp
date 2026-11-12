#include "game/main.h"
#include "game/pc/pclocals.h"
#include "game/pc/gamewindow.h"
#include "game/pc/mainvm.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

// 0x0045c530 (PDB: 0x0045dc90)
int ProcessArgs(char *argData, void *gameTracker, void *appData)
{
    (void)argData;
    (void)gameTracker;
    (void)appData;

    // Checks command line and \\TR7\\game\\pc\\TR7.arg flags:
    // -NOHEALTH, -NOHINTS, -MAINMENU, -NOMAINMENU, -PLAYER, -CHAPTER,
    // -SRCONFIG, -EASYCHEAT, -NOVIBRATION, -NOTRACE, -FONTNAME
    return 0;
}

// 0x0045c940 (PDB: 0x0045e0d0)
void MAIN_DoMainInit(void)
{
    // Loads core engine shaders: sprintf(path, "%s%s.drm", g_buildDir, "shaders")
    // Loads general audio banks: generalbank.drm, globalsoundinfo.drm
    // Initializes master UI, menus, and input combinations
}

// 0x0045cc10 (PDB: 0x0045e3a0)
int MainG2(void *appData)
{
    char baseAreaName[256] = {};
    char gameTrackerStub[256] = {};

    // 1. Process game arguments
    if (ProcessArgs(baseAreaName, gameTrackerStub, appData) != 0) {
        return 0;
    }

    // 2. Top-level subsystem initialization
    MAIN_DoMainInit();

    // 3. Core engine tick and dispatch loop
    while (!MainG2_UpdateLoop()) {
        // Main game frame update
    }

    return 0;
}
