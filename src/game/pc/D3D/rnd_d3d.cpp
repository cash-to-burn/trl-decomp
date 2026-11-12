#include "game/pc/D3D/rnd_d3d.h"
#include "game/pc/gamewindow.h"
#include "cdc/runtime/cdcRender/pc/shared/PCDeviceManager.h"

// 0x00413880
bool D3D_InitManager(void)
{
    return cdc::PCDeviceManager::CreateInstance() != nullptr;
}

// 0x004139a0
bool D3D_InitDevice(bool param)
{
    (void)param;
    cdc::PCDeviceManager *pMgr = cdc::PCDeviceManager::GetInstance();
    if (!pMgr) return false;
    return pMgr->CreateDevice(nullptr);
}

// 0x00413930
void D3D_ShutdownDevice(void)
{
    cdc::PCDeviceManager::DestroyInstance();
}

// 0x00413a20
void RND_Shutdown(void)
{
    cdc::PCDeviceManager *pMgr = cdc::PCDeviceManager::GetInstance();
    if (pMgr) {
        pMgr->ReleaseDevice(0xFFFFFFFF);
    }
}
