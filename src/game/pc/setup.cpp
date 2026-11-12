#include "game/pc/setup.h"
#include <cstring>

int g_languageID = 0;           // 0x00f11ca0
int g_subtitleLanguageID = 0;   // 0x00f11ca4
int g_fullInstall = 0;          // 0x00f11d44
int g_mouseSensitivity = 50;    // 0x00f11c8c
int g_eaxSupport = 0;           // 0x00f11c84
int g_useReverb = 1;            // 0x00f11c88
int g_disableForceFeedback = 0; // 0x00f11c78
int g_dontPauseOnFocusLoss = 0; // 0x00f11c7c
int g_voVolume = 100;           // 0x00f11c98
int g_musicVolume = 100;        // 0x00f11c90
int g_sfxVolume = 100;          // 0x00f11c94
int g_hints = 1;                // 0x00f11c9c
int g_vibration = 1;            // 0x00f11ca8
int g_widescreen = 0;           // 0x00f11cac
int g_combatMode = 0;           // 0x00f11cb0
int g_cameraXInverted = 0;      // 0x00f11cb4
int g_cameraYInverted = 0;      // 0x00f11cb8
int g_aimXInverted = 0;         // 0x00f11cbc
int g_aimYInverted = 0;         // 0x00f11cc0
char g_gamepadConfig[128] = {}; // 0x00f11cc4

#define REG_GAME_KEY "SOFTWARE\\Crystal Dynamics\\Tomb Raider: Legend"
#define REG_GRAPHICS_KEY "SOFTWARE\\Crystal Dynamics\\Tomb Raider: Legend\\Graphics"

// 0x00404a40
int SETUP_LoadRegistrySettings(void)
{
#ifdef _WIN32
    HKEY hKey = NULL;
    DWORD type = 0;
    DWORD size = 0;
    char langStr[32] = {};

    // 1. Read machine install settings from HKLM
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, REG_GAME_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        size = sizeof(g_fullInstall);
        RegQueryValueExA(hKey, "FullInstall", NULL, &type, (LPBYTE)&g_fullInstall, &size);

        size = sizeof(langStr);
        if (RegQueryValueExA(hKey, "LanguageString", NULL, &type, (LPBYTE)langStr, &size) == ERROR_SUCCESS) {
            if (_stricmp(langStr, "English") == 0) g_languageID = 0;
            else if (_stricmp(langStr, "French") == 0) g_languageID = 1;
            else if (_stricmp(langStr, "German") == 0) g_languageID = 2;
            else if (_stricmp(langStr, "Italian") == 0) g_languageID = 3;
            else if (_stricmp(langStr, "Spanish") == 0) g_languageID = 4;
            else if (_stricmp(langStr, "Polish") == 0) g_languageID = 7;
            else if (_stricmp(langStr, "Russian") == 0) g_languageID = 9;
        }
        RegCloseKey(hKey);
    }

    // 2. Read user settings from HKCU
    if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_GAME_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        size = sizeof(DWORD);
        RegQueryValueExA(hKey, "MouseSensitivity", NULL, &type, (LPBYTE)&g_mouseSensitivity, &size);
        RegQueryValueExA(hKey, "EAXSupport", NULL, &type, (LPBYTE)&g_eaxSupport, &size);
        RegQueryValueExA(hKey, "UseReverb", NULL, &type, (LPBYTE)&g_useReverb, &size);
        RegQueryValueExA(hKey, "DisableForceFeedback", NULL, &type, (LPBYTE)&g_disableForceFeedback, &size);
        RegQueryValueExA(hKey, "DontPauseOnFocusLoss", NULL, &type, (LPBYTE)&g_dontPauseOnFocusLoss, &size);
        RegQueryValueExA(hKey, "VoVolume", NULL, &type, (LPBYTE)&g_voVolume, &size);
        RegQueryValueExA(hKey, "MusicVolume", NULL, &type, (LPBYTE)&g_musicVolume, &size);
        RegQueryValueExA(hKey, "SfxVolume", NULL, &type, (LPBYTE)&g_sfxVolume, &size);
        RegQueryValueExA(hKey, "Hints", NULL, &type, (LPBYTE)&g_hints, &size);

        size = sizeof(g_gamepadConfig);
        RegQueryValueExA(hKey, "Gamepad", NULL, &type, (LPBYTE)g_gamepadConfig, &size);

        RegCloseKey(hKey);
        return 1;
    }
#endif
    return 0;
}

// 0x004044c0
void SETUP_LoadUserPreferences(void)
{
#ifdef _WIN32
    HKEY hKey = NULL;
    DWORD type = 0;
    DWORD size = 0;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_GAME_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        size = sizeof(DWORD);
        RegQueryValueExA(hKey, "Language", NULL, &type, (LPBYTE)&g_languageID, &size);
        RegQueryValueExA(hKey, "SubtitleLanguage", NULL, &type, (LPBYTE)&g_subtitleLanguageID, &size);
        RegQueryValueExA(hKey, "Vibration", NULL, &type, (LPBYTE)&g_vibration, &size);
        RegQueryValueExA(hKey, "Widescreen", NULL, &type, (LPBYTE)&g_widescreen, &size);
        RegQueryValueExA(hKey, "CombatMode", NULL, &type, (LPBYTE)&g_combatMode, &size);
        RegQueryValueExA(hKey, "CameraXInverted", NULL, &type, (LPBYTE)&g_cameraXInverted, &size);
        RegQueryValueExA(hKey, "CameraYInverted", NULL, &type, (LPBYTE)&g_cameraYInverted, &size);
        RegQueryValueExA(hKey, "AimXInverted", NULL, &type, (LPBYTE)&g_aimXInverted, &size);
        RegQueryValueExA(hKey, "AimYInverted", NULL, &type, (LPBYTE)&g_aimYInverted, &size);

        size = sizeof(g_gamepadConfig);
        RegQueryValueExA(hKey, "Gamepad", NULL, &type, (LPBYTE)g_gamepadConfig, &size);

        RegCloseKey(hKey);
    }
#endif
}

// 0x00404e00
void SETUP_SaveRegistrySettings(void)
{
#ifdef _WIN32
    HKEY hKey = NULL;
    DWORD disposition = 0;

    if (RegCreateKeyExA(HKEY_CURRENT_USER, REG_GAME_KEY, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &disposition) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "MouseSensitivity", 0, REG_DWORD, (const BYTE *)&g_mouseSensitivity, sizeof(DWORD));
        RegSetValueExA(hKey, "EAXSupport", 0, REG_DWORD, (const BYTE *)&g_eaxSupport, sizeof(DWORD));
        RegSetValueExA(hKey, "UseReverb", 0, REG_DWORD, (const BYTE *)&g_useReverb, sizeof(DWORD));
        RegSetValueExA(hKey, "DisableForceFeedback", 0, REG_DWORD, (const BYTE *)&g_disableForceFeedback, sizeof(DWORD));
        RegSetValueExA(hKey, "DontPauseOnFocusLoss", 0, REG_DWORD, (const BYTE *)&g_dontPauseOnFocusLoss, sizeof(DWORD));
        RegSetValueExA(hKey, "VoVolume", 0, REG_DWORD, (const BYTE *)&g_voVolume, sizeof(DWORD));
        RegSetValueExA(hKey, "MusicVolume", 0, REG_DWORD, (const BYTE *)&g_musicVolume, sizeof(DWORD));
        RegSetValueExA(hKey, "SfxVolume", 0, REG_DWORD, (const BYTE *)&g_sfxVolume, sizeof(DWORD));
        RegSetValueExA(hKey, "Hints", 0, REG_DWORD, (const BYTE *)&g_hints, sizeof(DWORD));
        RegSetValueExA(hKey, "Vibration", 0, REG_DWORD, (const BYTE *)&g_vibration, sizeof(DWORD));
        RegSetValueExA(hKey, "Widescreen", 0, REG_DWORD, (const BYTE *)&g_widescreen, sizeof(DWORD));
        RegSetValueExA(hKey, "CombatMode", 0, REG_DWORD, (const BYTE *)&g_combatMode, sizeof(DWORD));
        RegSetValueExA(hKey, "CameraXInverted", 0, REG_DWORD, (const BYTE *)&g_cameraXInverted, sizeof(DWORD));
        RegSetValueExA(hKey, "CameraYInverted", 0, REG_DWORD, (const BYTE *)&g_cameraYInverted, sizeof(DWORD));
        RegSetValueExA(hKey, "AimXInverted", 0, REG_DWORD, (const BYTE *)&g_aimXInverted, sizeof(DWORD));
        RegSetValueExA(hKey, "AimYInverted", 0, REG_DWORD, (const BYTE *)&g_aimYInverted, sizeof(DWORD));
        RegSetValueExA(hKey, "Gamepad", 0, REG_BINARY, (const BYTE *)g_gamepadConfig, sizeof(g_gamepadConfig));
        RegCloseKey(hKey);
    }
#endif
}

// 0x004056c0
int SETUP_DoSetupDialog(int allowWindowSwitch, int forceDialog)
{
    // If not forced and settings are valid, proceed directly
    if (!forceDialog) {
        return 1;
    }
    // Stub for displaying interactive dialog: returns 1 for OK, 0 for Cancel
    return 1;
}

// 0x004057c0
void SETUP_LocalizeSetupDialog(void)
{
    // Stub for populating localized strings for dialog buttons/labels
}
