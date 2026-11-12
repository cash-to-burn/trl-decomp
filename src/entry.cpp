#include "common.h"
#include "game/pc/mainvm.h"

// 0x00ee6ef8 (PDB: 0x00ee9878 / _WinMainCRTStartup)
extern "C" int entry(void)
{
#ifdef _WIN32
    // 1. Query OS version info
    OSVERSIONINFOA osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    GetVersionExA(&osvi);

    // 2. Query module handle
    HINSTANCE hInstance = GetModuleHandleA(NULL);

    // 3. Process startup info to determine window display state
    STARTUPINFOA startupInfo;
    memset(&startupInfo, 0, sizeof(startupInfo));
    GetStartupInfoA(&startupInfo);

    int nCmdShow = SW_SHOWDEFAULT;
    if (startupInfo.dwFlags & STARTF_USESHOWWINDOW) {
        nCmdShow = startupInfo.wShowWindow;
    }

    // 4. Retrieve command line
    LPSTR lpCmdLine = GetCommandLineA();
    // Skip executable name in command line if present
    if (lpCmdLine && *lpCmdLine == '"') {
        lpCmdLine++;
        while (*lpCmdLine && *lpCmdLine != '"') lpCmdLine++;
        if (*lpCmdLine == '"') lpCmdLine++;
    } else if (lpCmdLine) {
        while (*lpCmdLine && *lpCmdLine > ' ') lpCmdLine++;
    }
    while (lpCmdLine && *lpCmdLine && *lpCmdLine <= ' ') lpCmdLine++;

    // 5. Invoke application WinMain
    int mainRet = WinMain(hInstance, NULL, lpCmdLine, nCmdShow);

    return mainRet;
#else
    return 0;
#endif
}
