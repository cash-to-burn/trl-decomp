#pragma once

#ifndef GAME_SAVE_MSSAVE_H
#define GAME_SAVE_MSSAVE_H

#include "common.h"

// Forward declarations for UI types
struct UIMenu;
struct UIMenuItem;
enum menu_ctrl_t {
    menu_ctrl_none = 0
};

class MSSave {
public:
    enum UIState {
        UI_IDLE = 0,
        UI_DIALOG_POP,
        UI_DIALOG_CLOSE,
        UI_DIALOG_CLOSING,
        UI_UNPAUSE,
        UI_SETUP,
        UI_SETUP_GETTING_SAVEINFO,
        UI_SETUP_PLAYER_DATA_RETRY,
        UI_SETUP_PLAYER_DATA,
        UI_SETUP_LOADING_SETTINGS,
        UI_SETUP_LOADING_AUTOSAVE,
        UI_SETUP_CHECK_PLAYER_DATA,
        UI_SETUP_FIX_PLAYER_DATA,
        UI_SETUP_SAVE_PLAYER_DATA,
        UI_SETUP_SAVE_SETTINGS,
        UI_SETUP_SAVING_SETTINGS,
        UI_SETUP_AUTOSAVE,
        UI_SETUP_AUTOSAVING,
        UI_SETUP_CHECK_SPACE_FOR_SAVE,
        UI_SETUP_ARCADE_MODE,
        UI_SETUP_DONE,
        UI_MAINMENU_INIT,
        UI_MAINMENU_LOADING_GAME,
        UI_MAINMENU_LOADED_GAME,
        UI_SAVEMENU_OPEN_CONFIRMED,
        UI_SAVEMENU_OPEN,
        UI_SAVEGAME_OVERWRITE,
        UI_SAVEGAME,
        UI_SAVING_GAME,
        UI_SAVEGAME_GETTING_SAVEINFO,
        UI_SAVEGAME_DELETE,
        UI_STARTNEWGAME_CONFIRMED,
        UI_STARTNEWGAME_TESTSAVE,
        UI_STARTNEWGAME_TESTSAVE_CONFIRMED,
        UI_STARTNEWGAME,
        UI_RESUMEGAME_CONFIRMED,
        UI_RESUMEGAME,
        UI_LOADGAME_CONFIRMED,
        UI_LOADGAME,
        UI_LOADGAME_LOADING,
        UI_LOADGAME_AUTOSAVING,
        UI_LOADGAME_DONE,
        UI_RUNGAME_CONFIRMED,
        UI_RUNGAME_TESTSAVE,
        UI_RUNGAME_TESTSAVE_CONFIRMED,
        UI_RUNGAME,
        UI_RUNGAME_LOADING,
        UI_RUNGAME_AUTOSAVING,
        UI_RUNGAME_QUIT_AUTOSAVING,
        UI_RUNGAME_QUIT_FADING,
        UI_BOOT_TO_MEMORY
    };

    enum State {
        STATE_IDLE = 0,
        STATE_READING = 1,
        STATE_WRITING = 2,
        STATE_DELETING = 3
    };

    struct SaveSlot {
        char m_Name[96];
        wchar_t m_Filename[130];
        uint64_t m_FileTime;
    };

    struct SaveInfo {
        SaveSlot m_Slot;
        int32_t m_Level;
        uint32_t m_PlayTimeSec;
        uint8_t m_PercentComplete;
        uint8_t m_Padding[3];
        uint64_t m_FileTime;
    };

    struct SaveHeader {
        uint8_t m_Data[492];
        uint32_t m_Checksum;
    };

    struct Save {
        uint8_t m_Header[2048];
        uint8_t m_Data[98304];
        uint8_t m_Padding[492];
        uint32_t m_Checksum;
    };

    struct Settings {
        uint8_t m_Data[96];
        uint8_t m_Padding[1928];
        uint32_t m_Checksum;
    };

    // Public members matching original memory offsets
    HANDLE m_FileHandle;
#ifdef _WIN32
    OVERLAPPED m_Overlapped;
#else
    uint8_t m_Overlapped[20];
#endif
    uint32_t m_BufferSize;
    void *m_Buffer;
    State m_State;
    HANDLE m_EnumHandle;
    uint32_t m_NumSaves;

    SaveInfo m_SaveInfo[50];

    bool m_IsSetupDone;
    UIState m_UIState;
    UIState m_UINextState;
    bool m_ArcadeMode;
    bool m_SettingsValid;
    bool m_IsAutosaveValid;
    bool m_IsAutosaveEmpty;
    bool m_StartNewGame;
    bool m_IsSavesUpdated;
    bool m_DoAutosave;
    bool m_IsAutosaving;
    uint32_t m_NumBlockNeeded;

    wchar_t m_SaveGamesFolder[260];
    wchar_t m_RootDir[260];

    SaveSlot m_AutoSaveSlot;
    SaveSlot m_SaveSlot;
    SaveHeader m_SaveHeader;
    Save m_Save;
    Save *m_Load;
    Settings m_Settings;

    uint32_t m_UISavesIndex;
    char m_DialogYesOption[128];
    char m_DialogNoOption[128];

    UIMenu *m_MenuCommand_menu;
    UIMenuItem *m_MenuCommand_menuItem;
    int32_t m_MenuCommand_startChapterIndex;
    const char *m_MenuCommand_textParam;
    menu_ctrl_t m_MenuCommand_ctrl;

public:
    MSSave();
    ~MSSave();

    static MSSave *s_instance;
    static MSSave *Create();
    static void Destroy();

    static bool GetShellFolderFromRegistry(wchar_t *outPath);
    static bool GetShellFolder(wchar_t *outPath);
    static const wchar_t *GetRootDir(wchar_t *outPath);
    static const wchar_t *GetRootDirDrive();

    bool CheckSignature();
    bool FileExists(const char *filename);
    uint32_t GetFileSize(const char *filename);
    uint32_t GetNumFreeBytes();
    bool SaveToDisk(const char *filename, void *data, uint32_t size);
    bool LoadFromDisk(const char *filename, void *data, uint32_t size);
    bool ValidateLoad();
    const char *GetNewSaveName(char *outName);
    bool SaveGameExists(const char *filename);
    bool CreateUniqueSlot(SaveSlot &slot);
    uint32_t GetNumSaves() const;
    const char *GetSaveName(uint32_t index) const;
    void ClearSettings(Settings &settings);
    void UpdateSaveGame(Save &save);
    void AutoSave(const Save &save);
    void AutoSave();
    void LoadGame(const SaveSlot &slot, Save &save);
    void LoadGame(uint32_t index);
    bool DeleteSlot(const SaveSlot &slot) const;
    void DeleteGame(uint32_t index);
    void Setup();
    void InitMainMenu();
    void InitLoadMenu();
    void InitSaveMenu();
    void RunGame(const Save &save);
    bool IsAutoSaveValid() const;
    bool IsAutoSaveCompleted() const;
    bool CanLoad() const;
    bool CanSave() const;
    bool CanSaveNew() const;
    bool IsSavesUpdated();
    static void UICommand(void *param);
    bool TestSaveAbility(UIState state, bool param2, bool param3);
    void CloseScreen(UIState state);
    void UpdateTransfer();
    void UpdateAutosave();
    void ClearSaveGame(Save &save);
    void SaveGame(const Save &save);
    void SaveGame(uint32_t index);
    void TryStartNewGame();
    void TryStartNewGame(UIMenu *menu, UIMenuItem *menuItem, int startChapterIndex, const char *textParam, menu_ctrl_t ctrl);
    void ResumeGame();
    void LoadGameAndAutosave(uint32_t index);
    void LoadAndRunGame(uint32_t index);
    void GetSaveInfo();
    void UpdateUI();
    void Update();
    bool IsTransferDone();
};

// Global singleton accessor (0x005bb5c0 / PDB 0x005bdf30)
MSSave *MSSavePtr();

// Global paths (0x01138850, 0x01138a50)
extern wchar_t g_autosavePath[MAX_PATH];
extern wchar_t g_settingsPath[MAX_PATH];

#endif // GAME_SAVE_MSSAVE_H
