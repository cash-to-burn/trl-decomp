#include "game/pc/mainvm.h"
#include "game/pc/gamewindow.h"
#include "game/pc/setup.h"
#include "game/pc/timer.h"
#include "game/pc/pclocals.h"
#include "game/pc/D3D/rnd_d3d.h"
#include "game/pc/pcinput.h"
#include "game/Stream/stream.h"
#include "game/archive/ArchiveFileSystem.h"
#include "game/Save/MSSave.h"
#include "game/main.h"

HANDLE hMainThread = NULL;             // 0x00f2c3a4
DWORD iMainThreadId = 0;               // 0x00f2c3a0
char g_mutexName[32] = TRL_MUTEX_NAME; // 0x00f10424
int quitdone = 0;

// 0x00401950 (PDB: 0x00401940)
void MessageBoxUTF8(HWND hWnd, const cdc::UTF8String &msg, const cdc::UTF8String &caption, UINT uType)
{
#ifdef _WIN32
    WCHAR wMsg[1024];
    WCHAR wCaption[1024];
    msg.ConvertToWCHAR(wMsg, 1024);
    caption.ConvertToWCHAR(wCaption, 1024);
    MessageBoxW(hWnd, wMsg, wCaption, uType);
#endif
}

// 0x00401a10 (PDB: 0x00401f00)
BOOL CALLBACK FindGameEnumWindowsProc(HWND hwnd, LPARAM lParam)
{
#ifdef _WIN32
    char tmp[1024];
    WINDOWPLACEMENT placement;
    int len = GetClassNameA(hwnd, tmp, sizeof(tmp));
    if (len > 0) {
        const char *targetClass = *(const char **)lParam;
        if (_stricmp(tmp, targetClass) == 0) {
            SetFocus(hwnd);
            placement.length = sizeof(WINDOWPLACEMENT);
            if (GetWindowPlacement(hwnd, &placement) && placement.showCmd == SW_SHOWMINIMIZED) {
                ShowWindow(hwnd, SW_RESTORE);
            }
            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            SetForegroundWindow(hwnd);
            ((HWND *)lParam)[1] = hwnd;
            return FALSE; // Target window found, stop enumeration
        }
    }
#endif
    return TRUE; // Continue enumerating
}

// 0x00401b50 (PDB: 0x00402070)
void TerminateGame(void)
{
#ifdef _WIN32
    DWORD curId = GetCurrentThreadId();
    if (curId != iMainThreadId && hMainThread) {
        SuspendThread(hMainThread);
        RND_Shutdown();
        D3D_ShutdownDevice();
        GAMEWINDOW_Deinit();
        TerminateProcess(GetCurrentProcess(), (UINT)-1);
        return;
    }
    RND_Shutdown();
    D3D_ShutdownDevice();
    GAMEWINDOW_Deinit();
    ExitProcess(0);
#endif
}

// 0x00401ab0 (PDB: 0x00401fa0)
bool MainG2_UpdateLoop(void)
{
    uint64_t startUS = TIMER_GetTimeUS();

    // 1. Input poll & device updates
    InputDevice::Update();

    // 2. Poll streaming loader queue
    STREAM_PollLoadQueue();

    // 3. Process Windows messages (returns 1 on WM_QUIT)
    if (GAMEWINDOW_Poll()) {
        return false; // Quit requested
    }

    uint64_t elapsedUS = TIMER_GetTimeUS() - startUS;
    (void)elapsedUS;
    return true; // Keep running
}

// 0x00401c60 (PDB: 0x00402150)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifdef _WIN32
    _strlwr(lpCmdLine);

    // 1. Thread affinity setup: pin main thread to CPU 0
    HANDLE hCurProcess = GetCurrentProcess();
    HANDLE hCurThread = GetCurrentThread();
    DuplicateHandle(hCurProcess, hCurThread, hCurProcess, &hMainThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
    iMainThreadId = GetCurrentThreadId();
    SetThreadAffinityMask(GetCurrentThread(), 1);

    // 2. Load registry settings and user preferences
    int hasRegistrySettings = SETUP_LoadRegistrySettings();
    SETUP_LoadUserPreferences();

    // 3. Parse command line options
    int forceOptions = 0;
    if (strstr(lpCmdLine, "-setup")) {
        forceOptions = 1;
    } else if (strstr(lpCmdLine, "-config")) {
        forceOptions = 2;
    }

    // 4. Initialize core subsystems
    MSSave::Create();
    InitFS();
    TIMER_Init();
    InitArchive();

    // 5. Setup localized directories
    SetupBuildDir("PC-W");
    if (!localstr_verifyexistence()) {
        SetupBuildDir("PCENGLISH");
    }
    localstr_set_language(g_languageID);
    SETUP_LocalizeSetupDialog();

    // 6. Mutex single-instance check
    HANDLE hMutexAppIsRunning = CreateMutexA(NULL, FALSE, g_mutexName);
    if (hMutexAppIsRunning != NULL && GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutexAppIsRunning);

        // Bring existing window to front
        struct {
            const char *className;
            HWND foundHwnd;
        } enumData = { TRL_WINDOW_CLASS, NULL };

        EnumWindows(FindGameEnumWindowsProc, (LPARAM)&enumData);
        if (enumData.foundHwnd == NULL) {
            cdc::UTF8String title(localstr_get(0x11)); // "Error"
            cdc::UTF8String errorMsg(localstr_get(0x2e)); // "Another instance of Tomb Raider: Legend is already running."
            MessageBoxUTF8(NULL, errorMsg, title, MB_OK | MB_ICONERROR);
        }
        return 1;
    }

    if (hMutexAppIsRunning == NULL) {
        return 1;
    }

    // 7. Direct3D subsystem initialization & OS version check
    if (!D3D_InitManager()) {
        cdc::UTF8String title(localstr_get(0x11));
        cdc::UTF8String errorMsg(localstr_get(0x12)); // "Failed to initialize Direct3D."
        MessageBoxUTF8(NULL, errorMsg, title, MB_OK | MB_ICONERROR);
        CloseHandle(hMutexAppIsRunning);
        return 0;
    }

    OSVERSIONINFOA verInfo;
    verInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    GetVersionExA(&verInfo);

    // Requires Windows 2000 / XP or newer (NT 5.0+)
    if (verInfo.dwPlatformId <= VER_PLATFORM_WIN32_NT && verInfo.dwMajorVersion < 5) {
        cdc::UTF8String title(localstr_get(0x11));
        cdc::UTF8String msg(localstr_get(0x2d)); // "Tomb Raider: Legend requires Windows 2000 or Windows XP."
        MessageBoxUTF8(NULL, msg, title, MB_OK | MB_ICONERROR);
        CloseHandle(hMutexAppIsRunning);
        return 0;
    }

    // 8. Configuration dialog check
    int showDialog = (forceOptions != 0 || !hasRegistrySettings);
    int configOk = SETUP_DoSetupDialog(1, showDialog);

    if (configOk && forceOptions != 1) {
        // Disable screensaver while game runs
        BOOL oldScreensaver = TRUE;
        SystemParametersInfoA(SPI_GETSCREENSAVEACTIVE, 0, &oldScreensaver, 0);
        SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, FALSE, NULL, 0);

        do {
            GAMEWINDOW_Init();

            // Direct3D Device initialization
            if (D3D_InitDevice(false)) {
                SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
                SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

                // Run main game state machine loop
                MainG2(0);

                // Clean shutdown
                RND_Shutdown();
                ReleaseFS();
                D3D_ShutdownDevice();
                GAMEWINDOW_Deinit();
                MSSave::Destroy();
                SETUP_SaveRegistrySettings();

                // Restore screensaver
                SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, oldScreensaver, NULL, 0);
                CloseHandle(hMutexAppIsRunning);
                return 0;
            }

            GAMEWINDOW_Deinit();
            MessageBoxA(NULL, localstr_get(0x21), localstr_get(0x22), MB_OK | MB_ICONERROR);
            configOk = SETUP_DoSetupDialog(1, 1);
        } while (configOk);

        SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, oldScreensaver, NULL, 0);
    }

    ReleaseFS();
    D3D_ShutdownDevice();
    CloseHandle(hMutexAppIsRunning);
    return 0;
#else
    return 0;
#endif
}
