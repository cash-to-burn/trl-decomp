#include "game/pc/pclocals.h"
#include "game/archive/ArchiveFileSystem.h"
#include <cstdio>
#include <cstring>

char g_buildDir[256] = "PCENGLISH\\";     // 0x01111df0
char g_languageName[256] = "PCENGLISH"; // 0x01111e10
static int s_currentLanguageID = 0;

// 0x0045c0e0
void SetupBuildDir(const char *dir)
{
    if (!dir) return;

    strncpy(g_languageName, dir, sizeof(g_languageName) - 1);
    g_languageName[sizeof(g_languageName) - 1] = '\0';

    snprintf(g_buildDir, sizeof(g_buildDir), "%s\\", dir);

    char soundDir[256];
    char cineDir[256];
    snprintf(soundDir, sizeof(soundDir), "%ssndstrm\\", g_buildDir);
    snprintf(cineDir, sizeof(cineDir), "%s", g_buildDir);
    // MultiplexStream directories set here in full engine
    (void)soundDir;
    (void)cineDir;
}

// 0x004e3fa0
bool localstr_verifyexistence(void)
{
    char localBinPath[256];
    snprintf(localBinPath, sizeof(localBinPath), "%slocal\\locals.bin", g_buildDir);

    if (g_pArchiveFS && g_pArchiveFS->FileExists(localBinPath)) {
        return true;
    }
    FILE *f = fopen(localBinPath, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

// 0x004e4010
void localstr_set_language(int langId)
{
    s_currentLanguageID = langId;
    // In original binary, loads strings table from locals.bin into memory
}

// 0x004e4050
const char *localstr_get(int stringId)
{
    switch (stringId) {
    case 0x11: return "Error";
    case 0x12: return "Failed to initialize Direct3D.";
    case 0x14: return "Please insert Tomb Raider: Legend CD/DVD.";
    case 0x1c: return "DirectX 9.0c is required.";
    case 0x21: return "Direct3D could not be initialized with current settings.";
    case 0x22: return "Configuration Error";
    case 0x2c: return "Direct3D hardware acceleration not found.";
    case 0x2d: return "Tomb Raider: Legend requires Windows 2000 or Windows XP.";
    case 0x2e: return "Another instance of Tomb Raider: Legend is already running.";
    default:   return "";
    }
}

// 0x004e4050 / PCLOCALS_GetFixedIDLocal
const char *PCLOCALS_GetFixedIDLocal(int localId)
{
    return localstr_get(localId);
}
