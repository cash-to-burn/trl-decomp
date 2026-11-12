#include "game/pc/D3D/rnd_d3d.h"
#include "game/pc/gamewindow.h"

static bool s_d3dManagerInitialized = false;
static bool s_d3dDeviceInitialized = false;

// 0x00413880
bool D3D_InitManager(void)
{
#ifdef _WIN32
    HMODULE hD3D9 = LoadLibraryA("d3d9.dll");
    if (!hD3D9) {
        return false;
    }
    FreeLibrary(hD3D9);
    s_d3dManagerInitialized = true;
    return true;
#else
    return false;
#endif
}

// 0x004139a0
bool D3D_InitDevice(bool param)
{
    (void)param;
    if (!s_d3dManagerInitialized) {
        return false;
    }
    s_d3dDeviceInitialized = true;
    return true;
}

// 0x00413930
void D3D_ShutdownDevice(void)
{
    s_d3dDeviceInitialized = false;
    s_d3dManagerInitialized = false;
}

// 0x00413a20
void RND_Shutdown(void)
{
    // Release texture caches and render targets prior to device cleanup
}
