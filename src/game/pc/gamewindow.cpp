#include "game/pc/gamewindow.h"
#include "game/pc/pcmouse.h"

HWND g_hWnd = NULL;

// 0x00403c00
LRESULT CALLBACK RegularWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN32
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    case WM_SYSCOMMAND:
        // Prevent screensaver and monitor low power state during gameplay
        if (wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER) {
            return 0;
        }
        break;
    case WM_SYSKEYDOWN:
        // Block Alt key navigation while allowing Alt+F4 (VK_F4 = 0x73)
        if ((lParam & 0x20000000) && wParam != VK_F4) {
            return 0;
        }
        break;
    case WM_LBUTTONDOWN:
        PCMOUSE_UpdateButtonDown(0);
        return 0;
    case WM_LBUTTONUP:
        PCMOUSE_UpdateButtonUp(0);
        return 0;
    case WM_RBUTTONDOWN:
        PCMOUSE_UpdateButtonDown(1);
        return 0;
    case WM_RBUTTONUP:
        PCMOUSE_UpdateButtonUp(1);
        return 0;
    case WM_MOUSEWHEEL:
        PCMOUSE_ScrollMouseWheel((int)(short)HIWORD(wParam) / 120);
        return 0;
    case WM_MOUSEMOVE: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;
        if (width > 0 && height > 0) {
            float normX = (float)(short)LOWORD(lParam) / (float)width;
            float normY = (float)(short)HIWORD(lParam) / (float)height;
            PCMOUSE_SetCursorPos(normX, normY);
        }
        return 0;
    }
    default:
        break;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
#else
    return 0;
#endif
}

// 0x00403fd0
void GAMEWINDOW_Init(void)
{
#ifdef _WIN32
    WNDCLASSA wc = {};
    wc.style = 0;
    wc.lpfnWndProc = RegularWndProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = LoadIconA(wc.hInstance, MAKEINTRESOURCEA(0x66));
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = TRL_WINDOW_CLASS;
    RegisterClassA(&wc);

    // Default window dimensions if fullscreen coordinates not yet retrieved
    int x = 0;
    int y = 0;
    int w = 1024;
    int h = 768;

    g_hWnd = CreateWindowExA(
        WS_EX_TOPMOST,
        TRL_WINDOW_CLASS,
        TRL_WINDOW_TITLE,
        WS_POPUP | WS_VISIBLE,
        x, y, w, h,
        NULL,
        NULL,
        wc.hInstance,
        NULL
    );

    ShowWindow(g_hWnd, SW_MAXIMIZE);
#endif
}

// 0x004040c0
void GAMEWINDOW_Deinit(void)
{
#ifdef _WIN32
    if (g_hWnd) {
        DestroyWindow(g_hWnd);
        g_hWnd = NULL;
    }
#endif
}

// 0x004040e0
int GAMEWINDOW_Poll(void)
{
#ifdef _WIN32
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE)) {
        if (!GetMessageA(&msg, NULL, 0, 0)) {
            return 1; // WM_QUIT
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
#endif
    return 0;
}
