#include "game/main.h"
#include "game/pc/pclocals.h"
#include "game/pc/gamewindow.h"
#include "game/pc/mainvm.h"
#include "game/pc/setup.h"
#include "game/pc/pcinput.h"
#include "game/gameloop.h"
#include "game/sound/sound.h"
#include "game/Stream/stream.h"
#include "game/obtable.h"
#include "game/instance.h"
#include "game/globalinfo.h"
#include "game/archive/ArchiveFileSystem.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

// Global game tracker and option states
int g_noHealth = 0;
int g_mainMenu = 1;
int g_startChapter = 0;
int g_chapterVars = 0;
int g_easyCheat = 0;
int g_noVibration = 0;
int g_noTrace = 0;
char g_fontName[64] = "default";
char g_baseAreaName[64] = "";
GameTracker g_gameTracker = {};
GlobalInfo* globalInfo = nullptr; // 0x010c0998 (PDB: 0x010c16d8)

void GameTracker::ResetOverrides()
{
    block_collide_override = 0;
    raziel_collide_override = 0;
    sound_collide_override = 0;
    attackwave_collide_override = 0;
    monster_collide_override = 0;
    monster_land_collide_override = 0;
    physics_line_draw_override = 0;
    birthStreamUnitID = 0;
    attachedID = 0;
    face = nullptr;
}

void GameTracker::ChangeMode(int mode)
{
    nextGameMode = mode;
    gameMode = (uint8_t)mode;
}

// 0x0045c450 (PDB: 0x0045dba0)
void *FSHelper_ReadFile(const char *filename, unsigned char memTag, cdc::FileSystem *fs)
{
    (void)memTag;
    if (fs == nullptr) {
        fs = g_pArchiveFS;
        if (fs == nullptr) {
            fs = g_pFileSystem;
        }
    }
    if (fs == nullptr) {
        return nullptr;
    }

    uint32_t fileSize = fs->GetFileSize(filename);
    if (fileSize == 0) {
        return nullptr;
    }

    char *buffer = (char *)malloc(fileSize + 1);
    if (buffer == nullptr) {
        return nullptr;
    }

    // Direct read via OS fallback or archive
    FILE *fp = fopen(filename, "rb");
    if (fp != nullptr) {
        size_t bytesRead = fread(buffer, 1, fileSize, fp);
        fclose(fp);
        buffer[bytesRead] = '\0';
        return buffer;
    }

    // Attempt relative read from game directory
    char fullPath[512];
    snprintf(fullPath, sizeof(fullPath), ".%s", filename);
    fp = fopen(fullPath, "rb");
    if (fp != nullptr) {
        size_t bytesRead = fread(buffer, 1, fileSize, fp);
        fclose(fp);
        buffer[bytesRead] = '\0';
        return buffer;
    }

    free(buffer);
    return nullptr;
}

// 0x0045bb80 - Case-insensitive flag search
const char *Arg_FindFlag(const char *flag, const char *argStr)
{
    if (flag == nullptr || argStr == nullptr) {
        return nullptr;
    }

    size_t flagLen = strlen(flag);
    const char *p = argStr;

    while (*p != '\0' && *p != '\n' && *p != '\r') {
        if (_strnicmp(p, flag, flagLen) == 0) {
            return p + flagLen;
        }
        p++;
    }
    return nullptr;
}

// 0x0045bc40 - Argument value extractor
void Arg_GetValue(const char *flag, const char *argStr, char *outVal)
{
    outVal[0] = '\0';
    const char *p = Arg_FindFlag(flag, argStr);
    if (p == nullptr) {
        return;
    }

    // Skip delimiters (' ', '\t', '=', '"')
    while (*p == ' ' || *p == '\t' || *p == '=' || *p == '"') {
        p++;
    }

    char temp[256];
    size_t i = 0;
    while (*p != '\0' && *p != '\r' && *p != '\n' && *p != ' ' && *p != '\t' && *p != '"' && i < sizeof(temp) - 1) {
        temp[i++] = *p++;
    }
    temp[i] = '\0';

    strcpy(outVal, temp);
}

// 0x0045bbd0 - Identifier prefix extractor (e.g. "c01" -> "c")
void Arg_GetPrefix(const char *str, char *outPrefix)
{
    outPrefix[0] = '\0';
    if (str == nullptr) {
        return;
    }

    const char *p = str;
    char *out = outPrefix;

    while (*p != '\0' && *p != '-' && (isalpha((unsigned char)*p) || *p == '_')) {
        *out++ = *p++;
    }
    *out = '\0';
}

// 0x0045bc00 - Digit suffix extractor (e.g. "c01" -> "01")
void Arg_GetDigits(const char *str, char *outDigits)
{
    outDigits[0] = '\0';
    if (str == nullptr) {
        return;
    }

    const char *p = str;
    while (*p != '\0' && *p != '-' && !isdigit((unsigned char)*p)) {
        p++;
    }

    char *out = outDigits;
    while (*p != '\0' && *p != '-' && isdigit((unsigned char)*p)) {
        *out++ = *p++;
    }
    *out = '\0';
}

// 0x0045c530 (PDB: 0x0045dc90)
int ProcessArgs(char *argData, void *gameTracker, void *appData)
{
    (void)appData;
    GameTracker *gt = (GameTracker *)gameTracker;

    char prefix[32] = "";
    char digits[32] = "";
    char argBuffer[2048] = "";
    char tempVal[256] = "";

    // 1. Attempt to read \TR7\game\pc\TR7.arg configuration file
    char *fileArgs = (char *)FSHelper_ReadFile("\\TR7\\game\\pc\\TR7.arg", 0, g_pArchiveFS);
    if (fileArgs == nullptr) {
        fileArgs = (char *)FSHelper_ReadFile("\\TR7\\game\\pc\\TR7.arg", 0, g_pFileSystem);
    }

    if (fileArgs != nullptr) {
        strncpy(argBuffer, fileArgs, sizeof(argBuffer) - 1);
        Arg_GetPrefix(fileArgs, prefix);
        Arg_GetDigits(fileArgs, digits);
    }

    // 2. Format base area name
    if (argData != nullptr) {
        snprintf(argData, 64, "%s%s", prefix, digits);
        strncpy(g_baseAreaName, argData, sizeof(g_baseAreaName) - 1);
    }

    // 3. Parse command flags
    if (Arg_FindFlag("-NOHEALTH", argBuffer)) {
        g_noHealth = 1;
    }

    if (Arg_FindFlag("-NOHINTS", argBuffer)) {
        if (gt != nullptr) {
            gt->flags |= 0x800000;
        }
    }

    if (Arg_FindFlag("-MAINMENU", argBuffer)) {
        g_mainMenu = 1;
    }

    if (Arg_FindFlag("-NOMAINMENU", argBuffer)) {
        g_mainMenu = 0;
    }

    if (Arg_FindFlag("-PLAYER", argBuffer)) {
        Arg_GetValue("-PLAYER", argBuffer, tempVal);
    }

    if (Arg_FindFlag("-CHAPTER", argBuffer)) {
        Arg_GetValue("-CHAPTER", argBuffer, tempVal);
        g_startChapter = atoi(tempVal);
        if (gt != nullptr) {
            gt->attachedID = g_startChapter;
        }
    }

    if (Arg_FindFlag("-CHAPTERVARS", argBuffer)) {
        Arg_GetValue("-CHAPTERVARS", argBuffer, tempVal);
        g_chapterVars = atoi(tempVal);
    }

    if (Arg_FindFlag("-SRCONFIG", argBuffer)) {
        Arg_GetValue("-SRCONFIG", argBuffer, tempVal);
        SetupBuildDir(tempVal);
    }

    if (Arg_FindFlag("-NOMONSTERATTACK", argBuffer)) {
    }

    if (Arg_FindFlag("-NOMONSTERHEALTH", argBuffer)) {
    }

    if (Arg_FindFlag("-DRAWMONSTERCOMBAT", argBuffer)) {
    }

    if (Arg_FindFlag("-DRAWMONSTERATTACK", argBuffer)) {
    }

    if (Arg_FindFlag("-EASYCHEAT", argBuffer)) {
        g_easyCheat = 1;
    }

    if (Arg_FindFlag("-NOVIBRATION", argBuffer)) {
        g_noVibration = 1;
    }

    if (Arg_FindFlag("-NOTRACE", argBuffer)) {
        g_noTrace = 1;
    }

    if (Arg_FindFlag("-FONTNAME", argBuffer)) {
        Arg_GetValue("-FONTNAME", argBuffer, g_fontName);
    } else {
        strcpy(g_fontName, "default");
    }

    if (fileArgs != nullptr) {
        free(fileArgs);
    }

    return 0;
}

// 0x0045c940 (PDB: 0x0045e0d0)
void MAIN_DoMainInit(void)
{
    // 1. Initialize DirectInput devices
    InputDevice::Create();

    // 2. Initialize streaming and object list
    STREAM_Init();
    OBTABLE_Init();
    INSTANCE_InitInstanceList();

    // 3. Initialize sound subsystem and load core audio banks
    SOUND_Init();
    SOUND_LoadBank("generalbank", -1);
    SOUND_LoadBank("globalsoundinfo", -1);

    // 3. Verify shaders.drm existence in archive
    char shaderPath[256];
    snprintf(shaderPath, sizeof(shaderPath), "%sshaders.drm", g_buildDir);
    if (g_pArchiveFS != nullptr && g_pArchiveFS->FileExists(shaderPath)) {
        // Core engine shaders loaded
    }

    // 4. Set initial mode on game tracker
    if (g_mainMenu != 0) {
        g_gameTracker.ChangeMode(3); // Main menu mode (0xa000)
    } else {
        g_gameTracker.ChangeMode(2); // Gameplay mode (0x5000)
    }
}

// 0x0045cc10 (PDB: 0x0045e3a0)
int MainG2(void *appData)
{
    // 1. Setup build directory
    SetupBuildDir("PC-W");

    // 2. Process command line and configuration arguments
    if (ProcessArgs(g_baseAreaName, &g_gameTracker, appData) != 0) {
        return 0;
    }

    // 3. Reset collision and intro tracker states
    g_gameTracker.ResetOverrides();

    // 4. Initialize assets, sound, shaders, and UI
    MAIN_DoMainInit();
    SETUP_LoadUserPreferences();

    // 5. Initial mode selection
    if (g_mainMenu == 0) {
        g_gameTracker.gameMode = 2; // In-game gameplay
    } else {
        g_gameTracker.gameMode = 3; // Main menu
    }

    // 6. Master update loop
    while (!quitdone && MainG2_UpdateLoop()) {
        switch (g_gameTracker.gameMode) {
        case 1: // Attract / Demo intro mode
            break;
        case 2: // Running gameplay mode
            GAMELOOP_MainLoop(&g_gameTracker);
            break;
        case 3: // Main menu mode
            break;
        case 4: // Pause / in-game options menu
            break;
        case 6: // Chapter change
            g_gameTracker.gameMode = 2;
            break;
        case 7: // Cinematic sequence
            break;
        default:
            break;
        }
    }

    return 0;
}
