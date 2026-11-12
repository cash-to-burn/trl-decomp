#include "MSSave.h"
#include <new>

#ifdef _WIN32
#include <shlobj.h>
#endif

// Global singleton instance
MSSave *MSSave::s_instance = nullptr;

// Global paths (0x01138850, 0x01138a50)
wchar_t g_autosavePath[MAX_PATH] = {0};
wchar_t g_settingsPath[MAX_PATH] = {0};

// 0x005bb5c0 (PDB: 0x005bdf30)
MSSave *MSSavePtr()
{
    return MSSave::s_instance;
}

// 0x005bd910 (PDB: 0x005bed80)
MSSave *MSSave::Create()
{
    if (s_instance != nullptr) {
        return s_instance;
    }

    void *mem = malloc(sizeof(MSSave));
    if (mem != nullptr) {
        s_instance = new (mem) MSSave();
    } else {
        s_instance = nullptr;
    }
    return s_instance;
}

void MSSave::Destroy()
{
    if (s_instance != nullptr) {
        s_instance->~MSSave();
        free(s_instance);
        s_instance = nullptr;
    }
}

// 0x001bcf40 in PDB
bool MSSave::GetShellFolderFromRegistry(wchar_t *outPath)
{
#ifdef _WIN32
    HKEY hKey;
    DWORD type = REG_SZ;
    DWORD size = MAX_PATH * sizeof(wchar_t);

    LONG result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",
        0,
        KEY_READ,
        &hKey
    );

    if (result == ERROR_SUCCESS) {
        result = RegQueryValueExW(hKey, L"Personal", NULL, &type, (LPBYTE)outPath, &size);
        RegCloseKey(hKey);
        if (result == ERROR_SUCCESS) {
            // Strip trailing backslash
            size_t len = wcslen(outPath);
            if (len > 0 && outPath[len - 1] == L'\\') {
                outPath[len - 1] = L'\0';
            }
            return true;
        }
    }
#endif
    return false;
}

// 0x005bb6b0 (PDB: 0x001bd020)
bool MSSave::GetShellFolder(wchar_t *outPath)
{
#ifdef _WIN32
    HMODULE hShell = LoadLibraryA("shell32.dll");
    if (hShell != NULL) {
        typedef HRESULT (WINAPI *PFN_SHGetFolderLocation)(HWND, int, HANDLE, DWORD, PIDLIST_ABSOLUTE*);
        typedef BOOL (WINAPI *PFN_SHGetPathFromIDListW)(PCIDLIST_ABSOLUTE, LPWSTR);
        typedef HRESULT (WINAPI *PFN_SHGetMalloc)(LPMALLOC*);

        PFN_SHGetFolderLocation pfnGetFolderLocation =
            (PFN_SHGetFolderLocation)GetProcAddress(hShell, "SHGetFolderLocation");
        PFN_SHGetPathFromIDListW pfnGetPathFromIDListW =
            (PFN_SHGetPathFromIDListW)GetProcAddress(hShell, "SHGetPathFromIDListW");
        PFN_SHGetMalloc pfnGetMalloc =
            (PFN_SHGetMalloc)GetProcAddress(hShell, "SHGetMalloc");

        if (pfnGetFolderLocation && pfnGetPathFromIDListW && pfnGetMalloc) {
            PIDLIST_ABSOLUTE pidl = NULL;
            if (pfnGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, 0, &pidl) == S_OK) {
                if (pfnGetPathFromIDListW(pidl, outPath)) {
                    size_t len = wcslen(outPath);
                    if (len > 0 && outPath[len - 1] == L'\\') {
                        outPath[len - 1] = L'\0';
                    }
                    LPMALLOC pMalloc = NULL;
                    if (pfnGetMalloc(&pMalloc) == S_OK && pMalloc != NULL) {
                        pMalloc->Free(pidl);
                        pMalloc->Release();
                    }
                    FreeLibrary(hShell);
                    return true;
                }
            }
        }
        FreeLibrary(hShell);
    }
#endif
    return false;
}

// 0x005bb7d0 (PDB: 0x001bd140)
const wchar_t *MSSave::GetRootDir(wchar_t *outPath)
{
    if (!GetShellFolderFromRegistry(outPath)) {
        if (!GetShellFolder(outPath)) {
            wcscpy(outPath, L"C:\\My Documents");
        }
    }

    wcscat(outPath, L"\\Tomb Raider - Legend");
#ifdef _WIN32
    CreateDirectoryW(outPath, NULL);
#endif
    return outPath;
}

// 0x001bd210 in PDB
const wchar_t *MSSave::GetRootDirDrive()
{
    static wchar_t s_drive[4] = L"C:\\";
    wchar_t root[MAX_PATH];
    if (GetShellFolderFromRegistry(root) || GetShellFolder(root)) {
        if (root[0] != L'\0' && root[1] == L':') {
            s_drive[0] = root[0];
            s_drive[1] = L':';
            s_drive[2] = L'\\';
            s_drive[3] = L'\0';
        }
    }
    return s_drive;
}

// 0x005bd7c0 (PDB: 0x005be290)
MSSave::MSSave()
{
    m_FileHandle = NULL;
    memset(&m_Overlapped, 0, sizeof(m_Overlapped));
    m_BufferSize = 0;
    m_Buffer = nullptr;
    m_State = STATE_IDLE;
    m_EnumHandle = NULL;
    m_NumSaves = 0;

    for (int i = 0; i < 50; i++) {
        memset(&m_SaveInfo[i], 0, sizeof(SaveInfo));
    }

    m_IsSetupDone = false;
    m_UIState = UI_IDLE;
    m_UINextState = UI_IDLE;
    m_ArcadeMode = true;
    m_SettingsValid = false;
    m_IsAutosaveValid = false;
    m_IsAutosaveEmpty = true;
    m_StartNewGame = false;
    m_IsSavesUpdated = false;
    m_DoAutosave = false;
    m_IsAutosaving = false;
    m_NumBlockNeeded = 0;

    memset(m_SaveGamesFolder, 0, sizeof(m_SaveGamesFolder));
    memset(m_RootDir, 0, sizeof(m_RootDir));
    memset(&m_AutoSaveSlot, 0, sizeof(m_AutoSaveSlot));
    memset(&m_SaveSlot, 0, sizeof(m_SaveSlot));
    memset(&m_SaveHeader, 0, sizeof(m_SaveHeader));
    memset(&m_Save, 0, sizeof(m_Save));
    m_Load = nullptr;
    memset(&m_Settings, 0, sizeof(m_Settings));

    m_UISavesIndex = 0;
    memset(m_DialogYesOption, 0, sizeof(m_DialogYesOption));
    memset(m_DialogNoOption, 0, sizeof(m_DialogNoOption));

    m_MenuCommand_menu = nullptr;
    m_MenuCommand_menuItem = nullptr;
    m_MenuCommand_startChapterIndex = 0;
    m_MenuCommand_textParam = nullptr;
    m_MenuCommand_ctrl = menu_ctrl_none;

    // Resolve directories and paths
    GetRootDir(m_RootDir);

    swprintf(g_autosavePath, MAX_PATH, L"%ls\\autosave.dat", m_RootDir);
    swprintf(g_settingsPath, MAX_PATH, L"%ls\\settings.dat", m_RootDir);

    wcscpy(m_SaveGamesFolder, m_RootDir);
    wcscat(m_SaveGamesFolder, L"\\Saved Games");
#ifdef _WIN32
    CreateDirectoryW(m_SaveGamesFolder, NULL);
#endif
}

MSSave::~MSSave()
{
#ifdef _WIN32
    if (m_FileHandle != NULL && m_FileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_FileHandle);
        m_FileHandle = NULL;
    }
    if (m_EnumHandle != NULL && m_EnumHandle != INVALID_HANDLE_VALUE) {
        FindClose(m_EnumHandle);
        m_EnumHandle = NULL;
    }
#endif
}

// 0x005be880 in PDB
uint32_t MSSave::GetNumSaves() const
{
    return m_NumSaves;
}

// 0x005be890 in PDB
const char *MSSave::GetSaveName(uint32_t index) const
{
    if (index < m_NumSaves && index < 50) {
        return m_SaveInfo[index].m_Slot.m_Name;
    }
    return "";
}

// 0x005be8b0 in PDB
void MSSave::ClearSettings(Settings &settings)
{
    memset(&settings, 0, sizeof(Settings));
}

// 0x005beaf0 in PDB
bool MSSave::IsAutoSaveValid() const
{
    return m_IsAutosaveValid;
}

// 0x005beb20 in PDB
bool MSSave::IsAutoSaveCompleted() const
{
    return !m_IsAutosaving;
}

// 0x005beb50 in PDB
bool MSSave::CanLoad() const
{
    return (m_NumSaves > 0) || m_IsAutosaveValid;
}

// 0x005beb70 in PDB
bool MSSave::CanSave() const
{
    return true;
}

// 0x005bebc0 in PDB
bool MSSave::CanSaveNew() const
{
    return m_NumSaves < 50;
}

// 0x005bec10 in PDB
bool MSSave::IsSavesUpdated()
{
    bool updated = m_IsSavesUpdated;
    m_IsSavesUpdated = false;
    return updated;
}

// 0x005bdf10 in PDB
bool MSSave::IsTransferDone()
{
    return m_State == STATE_IDLE;
}

// 0x005bedb0 in PDB
void MSSave::UpdateTransfer()
{
    // Async transfer check loop
    if (m_State == STATE_IDLE) {
        return;
    }
}

// 0x005bee60 in PDB
void MSSave::UpdateAutosave()
{
    if (!m_DoAutosave) {
        return;
    }
    m_DoAutosave = false;
}

// 0x005beef0 in PDB
void MSSave::ClearSaveGame(Save &save)
{
    memset(&save, 0, sizeof(Save));
}

// 0x005bef10 in PDB
void MSSave::SaveGame(const Save &save)
{
    // Write save game to slot
    (void)save;
}

void MSSave::SaveGame(uint32_t index)
{
    (void)index;
}

// 0x005bef70 in PDB
void MSSave::TryStartNewGame()
{
    m_StartNewGame = true;
}

void MSSave::TryStartNewGame(UIMenu *menu, UIMenuItem *menuItem, int startChapterIndex, const char *textParam, menu_ctrl_t ctrl)
{
    m_MenuCommand_menu = menu;
    m_MenuCommand_menuItem = menuItem;
    m_MenuCommand_startChapterIndex = startChapterIndex;
    m_MenuCommand_textParam = textParam;
    m_MenuCommand_ctrl = ctrl;
    m_StartNewGame = true;
}

// 0x005beff0 in PDB
void MSSave::ResumeGame()
{
    m_UIState = UI_IDLE;
}

// 0x005bf060 in PDB
void MSSave::InitSaveMenu()
{
    m_UIState = UI_SAVEMENU_OPEN;
}

// 0x005bf100 in PDB
void MSSave::DeleteGame(uint32_t index)
{
    (void)index;
}

// 0x005bf160 in PDB
void MSSave::LoadGameAndAutosave(uint32_t index)
{
    (void)index;
}

// 0x005bf1f0 in PDB
void MSSave::LoadAndRunGame(uint32_t index)
{
    (void)index;
}

// 0x005bf300 in PDB
void MSSave::GetSaveInfo()
{
    // Scans m_SaveGamesFolder for *.dat files and fills m_SaveInfo array
}

// 0x005bf4b0 in PDB
void MSSave::UpdateUI()
{
}

// 0x005c0090 in PDB
void MSSave::Update()
{
    UpdateTransfer();
    UpdateUI();
}
