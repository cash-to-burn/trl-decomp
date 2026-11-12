#include "cdc/runtime/cdcRender/pc/shared/PCDeviceManager.h"
#include <cstdlib>
#include <cstring>

namespace cdc {

PCDeviceManager *PCDeviceManager::s_pInstance = nullptr;

// 0x00ec6ff0
PCDeviceManager::PCDeviceManager(HMODULE hD3D9, void *pD3D9)
    : m_pContextList(nullptr), m_refCount(1), m_status(0), m_hWnd(NULL),
      m_hD3D9(hD3D9), m_pD3D9(pD3D9), m_pDevice(nullptr)
{
    memset(&m_config, 0, sizeof(m_config));
}

// 0x00ec6ea0
PCDeviceManager::~PCDeviceManager()
{
    ReleaseDevice(0xFFFFFFFF);
    if (m_hD3D9) {
#ifdef _WIN32
        FreeLibrary(m_hD3D9);
#endif
        m_hD3D9 = NULL;
    }
}

// 0x00ec70f0
PCDeviceManager *PCDeviceManager::CreateInstance()
{
    if (s_pInstance) {
        s_pInstance->m_refCount++;
        return s_pInstance;
    }
#ifdef _WIN32
    HMODULE hModule = LoadLibraryA("d3d9.dll");
    if (!hModule) {
        return nullptr;
    }
    typedef void *(__stdcall *Direct3DCreate9Func)(UINT);
    Direct3DCreate9Func createFunc = (Direct3DCreate9Func)GetProcAddress(hModule, "Direct3DCreate9");
    if (!createFunc) {
        FreeLibrary(hModule);
        return nullptr;
    }
    void *pD3D9 = createFunc(32); // D3D_SDK_VERSION
    if (!pD3D9) {
        FreeLibrary(hModule);
        return nullptr;
    }
    s_pInstance = new PCDeviceManager(hModule, pD3D9);
    return s_pInstance;
#else
    return nullptr;
#endif
}

void PCDeviceManager::DestroyInstance()
{
    if (s_pInstance) {
        s_pInstance->m_refCount--;
        if (s_pInstance->m_refCount <= 0) {
            delete s_pInstance;
            s_pInstance = nullptr;
        }
    }
}

// 0x00ec6350
bool PCDeviceManager::SetVideoConfig(HWND hWnd, const PCVideoConfig *config)
{
    m_hWnd = hWnd;
    if (!config) return false;
    memcpy(&m_config, config, sizeof(PCVideoConfig));
    return CreateDevice(config);
}

// 0x00ec5f80
bool PCDeviceManager::CreateDevice(const PCVideoConfig *config)
{
    (void)config;
    // In full engine: queries adapter modes and calls IDirect3D9::CreateDevice
    return true;
}

// 0x00ec6230
void PCDeviceManager::ReleaseDevice(uint32_t flags)
{
    (void)flags;
    // Releases swap chains, render targets, and volatile vertex/index buffers
}

// 0x00ec62f0
void PCDeviceManager::UpdateDeviceStatus()
{
    // Checks IDirect3DDevice9::TestCooperativeLevel():
    // - D3DERR_DEVICELOST (0x88760868)
    // - D3DERR_DEVICENOTRESET (0x88760869)
}

} // namespace cdc
