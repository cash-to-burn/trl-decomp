#pragma once

#ifndef CDC_PCDEVICEMANAGER_H
#define CDC_PCDEVICEMANAGER_H

#include "common.h"

namespace cdc {

struct PCVideoConfig {
    uint32_t width;             // 0x00
    uint32_t height;            // 0x04
    uint32_t refreshRate;       // 0x08
    uint32_t fullscreen;        // 0x0C
    uint32_t vsync;             // 0x10
    uint32_t multisample;       // 0x14
    uint32_t aspectRatio;       // 0x18
    uint32_t depthFormat;       // 0x1C
    uint32_t adapterIndex;      // 0x20
    uint32_t displayFormat;     // 0x24
    uint32_t flags;             // 0x28
    uint32_t unused[5];         // Total: 16 DWORDs = 64 bytes
};

class PCDeviceManager {
public:
    // 0x00ec6ff0
    PCDeviceManager(HMODULE hD3D9, void *pD3D9);
    // 0x00ec6ea0
    ~PCDeviceManager();

    // 0x00ec70f0
    static PCDeviceManager *CreateInstance();
    static void DestroyInstance();
    static PCDeviceManager *GetInstance() { return s_pInstance; }

    // 0x00ec6350
    bool SetVideoConfig(HWND hWnd, const PCVideoConfig *config);

    // 0x00ec5f80
    bool CreateDevice(const PCVideoConfig *config);

    // 0x00ec6230
    void ReleaseDevice(uint32_t flags = 0xFFFFFFFF);

    // 0x00ec62f0
    void UpdateDeviceStatus();

    HWND GetHWnd() const { return m_hWnd; }
    void *GetDirect3D() const { return m_pD3D9; }
    void *GetDevice() const { return m_pDevice; }

    static PCDeviceManager *s_pInstance; // 0x01392d58

private:
    void *m_pContextList;        // 0x04
    int m_refCount;              // 0x0C
    int m_status;                // 0x10
    HWND m_hWnd;                 // 0x14
    HMODULE m_hD3D9;             // 0x18
    void *m_pD3D9;               // 0x1C (IDirect3D9*)
    void *m_pDevice;             // 0x20 (IDirect3DDevice9*)
    uint8_t m_pad[0x134];
    PCVideoConfig m_config;      // 0x158 (64 bytes)
};

} // namespace cdc

#endif // CDC_PCDEVICEMANAGER_H
