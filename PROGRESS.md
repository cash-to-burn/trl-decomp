# Tomb Raider: Legend (PC) Decompilation Progress

## Target Binary Information
- **Target Executable**: `trl.exe` (28,102,656 bytes, PE32)
- **Debug Symbols**: `tr7.pdb` (10,202,112 bytes, Microsoft C/C++ MSF v7.00 PDB)
- **Original Source Root**: `f:\tr7\`
- **Architecture**: x86 (32-bit Windows), MSVC 2003 (VC7.1)
- **Symbol Offset Note**:
  - `mainvm.c`: Retail `.text` offset shift of `-0x4f0` relative to PDB (`0x00402150` in PDB maps to `0x00401c60` in executable).
  - `main.c` / `ArchiveFileSystem`: Retail `.text` offset shift of `-0x1790` relative to PDB (`0x0045e3a0` in PDB maps to `0x0045cc10` in executable).
  - `Save` (`MSSave`): Retail `.text` offset shift of `-0xad0` to `-0x1470` relative to PDB (`0x005bed80` in PDB maps to `0x005bd910` in executable).
  - `gameloop.cpp`: Retail `.text` offset shift of `-0x1380` relative to PDB (`0x00450950` in PDB maps to `0x0044f5d0` in executable).
  - `Stream\stream.cpp`: Retail `.text` offset shift of `-0x2ac0` relative to PDB (`0x005d6700` in PDB maps to `0x005d3c40` in executable).

---

## Startup Sequence Call Graph (`entry()` -> `WinMain()`)

```
entry() [0x00ee6ef8 / PDB 0x00ee9878] (_WinMainCRTStartup)
│
├── GetVersionExA(&osvi)
├── GetModuleHandleA(NULL)
├── GetStartupInfoA(&startupInfo)
├── GetCommandLineA() / _wincmdln()
│
└── WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow) [0x00401c60 / PDB 0x00402150]
    │
    ├── DuplicateHandle(...) & SetThreadAffinityMask(GetCurrentThread(), 1)
    │   └── Locks main game thread to CPU Core 0 to prevent dual-core timer issues
    │
    ├── SETUP_LoadRegistrySettings() [0x00404a40]
    │   └── Reads "SOFTWARE\Crystal Dynamics\Tomb Raider: Legend" (Language, Audio, Gamepad)
    │
    ├── SETUP_LoadUserPreferences() [0x004044c0]
    │
    ├── Command-Line Parsing: "-setup" (forceOptions=1) / "-config" (forceOptions=2)
    │
    ├── MSSave::Create() [0x005bd910 / PDB 0x005bed80]
    │   ├── Allocates MSSave singleton instance (0x23578 bytes)
    │   └── MSSave::MSSave() [0x005bd7c0]
    │       ├── GetRootDir() [0x005bb7d0] -> GetShellFolderFromRegistry() / GetShellFolder()
    │       ├── Creates autosave path (%s\autosave.dat) & settings path (%s\settings.dat)
    │       └── Creates directory: "%s\Saved Games"
    │
    ├── InitFS() [0x0045c2a0 / PDB 0x0045d950] (Initializes native OS filesystem)
    │
    ├── TIMER_Init() [0x00402570 / PDB 0x00402af0] -> WIN_TimeUS() [0x00402520]
    │
    ├── InitArchive() [0x0045c300 / PDB 0x0045d9b0]
    │   └── ArchiveFileSystem::Open("BIGFILE.DAT") [0x0047a990] (Probes bigfile.000 ... bigfile.050)
    │
    ├── SetupBuildDir("PC-W") [0x0045c0e0 / PDB 0x0045d790]
    │   └── localstr_verifyexistence() [0x004e3fa0] -> if false: SetupBuildDir("PCENGLISH")
    │
    ├── localstr_set_language(g_languageID) [0x004e4010]
    ├── SETUP_LocalizeSetupDialog() [0x004057c0]
    │
    ├── CreateMutexA(NULL, FALSE, "TRLMUTEX") [0x00f10424]
    │   └── [Single-instance enforcement]:
    │       ├── EnumWindows(FindGameEnumWindowsProc, "TOMBRAIDER") [0x00401a10 / PDB 0x00401f00]
    │       └── MessageBoxUTF8() [0x00401950 / PDB 0x00401940] (If prior instance exists without window)
    │
    ├── D3D_InitManager() [0x00413880] -> PCDeviceManager::CreateInstance() [0x00ec70f0]
    │   └── Loads d3d9.dll, creates IDirect3D9, initializes PCDeviceManager singleton
    │
    ├── OS Version Verification (Requires Windows 2000 / XP, NT 5.0+)
    │
    ├── SETUP_DoSetupDialog(1, showDialog) [0x004056c0]
    │
    ├── Main Execution Cycle:
    │   ├── SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, FALSE) [Disable screensaver]
    │   ├── GAMEWINDOW_Init() [0x00403fd0]
    │   │   ├── RegisterClassA("TOMBRAIDER", RegularWndProc) [0x00403c00]
    │   │   └── CreateWindowExA(WS_EX_TOPMOST, "TOMBRAIDER", "Tomb Raider: Legend", ...)
    │   │
    │   ├── D3D_InitDevice(false) [0x004139a0] -> PCDeviceManager::CreateDevice() [0x00ec5f80]
    │   │   └── SetThreadPriority(THREAD_PRIORITY_NORMAL) & SetPriorityClass(NORMAL_PRIORITY_CLASS)
    │   │
    │   ├── MainG2(0) [0x0045cc10 / PDB 0x0045e3a0] (Core G2 Gameplay & Engine Loop)
    │   │   ├── ProcessArgs(baseAreaName, gameTracker, appData) [0x0045c530]
    │   │   │   ├── FSHelper_ReadFile("\\TR7\\game\\pc\\TR7.arg") [0x0045c450]
    │   │   │   └── Flag parsing (-NOHEALTH, -NOHINTS, -MAINMENU, -CHAPTER, -SRCONFIG, etc.)
    │   │   ├── MAIN_DoMainInit() [0x0045c940] (Loads shaders.drm, sound banks, UI)
    │   │   └── MainG2_UpdateLoop() [0x00401ab0] (Engine frame tick & state machine dispatch)
    │   │
    │   └── Shutdown & Cleanup:
    │       ├── RND_Shutdown() [0x00413a20] -> PCDeviceManager::ReleaseDevice() [0x00ec6230]
    │       ├── ReleaseFS() [0x0045c360 / PDB 0x0045da10] (Release Bigfile & FileSystem)
    │       ├── D3D_ShutdownDevice() [0x00413930] -> PCDeviceManager::DestroyInstance()
    │       ├── GAMEWINDOW_Deinit() [0x004040c0]
    │       ├── MSSave::Destroy()
    │       ├── SETUP_SaveRegistrySettings() [0x00404e00]
    │       ├── SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, oldScreensaver) [Restore screensaver]
    │       └── CloseHandle(hMutexAppIsRunning)
```

---

## Decompiled Modules & Functions

### `src/entry.cpp`
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x00ee6ef8` | `_WinMainCRTStartup` | `int entry(void)` | Decompiled & Verified |

### `src/game/pc/mainvm.cpp` & `src/game/pc/mainvm.h` (Original: `f:\tr7\game\pc\mainvm.c`)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x00401c60` | `_WinMain@16` | `int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)` | Decompiled & Verified |
| `0x00401a10` | `FindGameEnumWindowsProc` | `BOOL CALLBACK FindGameEnumWindowsProc(HWND, LPARAM)` | Decompiled & Verified |
| `0x00401950` | `MessageBoxUTF8` | `void MessageBoxUTF8(HWND, const cdc::UTF8String&, const cdc::UTF8String&, UINT)` | Decompiled & Verified |
| `0x00401b50` | `TerminateGame` | `void TerminateGame(void)` | Decompiled & Verified |
| `0x00401ab0` | `MainG2_UpdateLoop` | `bool MainG2_UpdateLoop(void)` | Decompiled & Verified |

### `src/game/Save/MSSave.cpp` & `src/game/Save/MSSave.h` (Original: `_final\game\Save\`)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x005bd910` | `?Create@MSSave@@SAPAV1@XZ` | `MSSave* MSSave::Create(void)` | Decompiled & Verified |
| `0x005bb5c0` | `MSSavePtr` | `MSSave* MSSavePtr(void)` | Decompiled & Verified |
| `0x005bd7c0` | `??0MSSave@@QAE@XZ` | `MSSave::MSSave(void)` | Decompiled & Verified |
| `0x005bb7d0` | `?GetRootDir@@YAPBDXZ` | `const wchar_t* MSSave::GetRootDir(wchar_t*)` | Decompiled & Verified |
| `0x005bb6b0` | `GetShellFolder` | `bool MSSave::GetShellFolder(wchar_t*)` | Decompiled & Verified |
| — | `GetShellFolderFromRegistry` | `bool MSSave::GetShellFolderFromRegistry(wchar_t*)` | Decompiled & Verified |
| — | `GetRootDirDrive` | `const wchar_t* MSSave::GetRootDirDrive(void)` | Decompiled & Verified |
| `0x005be880` | `?GetNumSaves@MSSave@@QBEIXZ` | `uint32_t MSSave::GetNumSaves(void) const` | Decompiled & Verified |
| `0x005be890` | `?GetSaveName@MSSave@@QBEPBDI@Z` | `const char* MSSave::GetSaveName(uint32_t) const` | Decompiled & Verified |
| `0x005beaf0` | `?IsAutoSaveValid@MSSave@@QBE_NXZ` | `bool MSSave::IsAutoSaveValid(void) const` | Decompiled & Verified |
| `0x005beb20` | `?IsAutoSaveCompleted@MSSave@@QBE_NXZ` | `bool MSSave::IsAutoSaveCompleted(void) const` | Decompiled & Verified |
| `0x005beb50` | `?CanLoad@MSSave@@QBE_NXZ` | `bool MSSave::CanLoad(void) const` | Decompiled & Verified |
| `0x005beb70` | `?CanSave@MSSave@@QBE_NXZ` | `bool MSSave::CanSave(void) const` | Decompiled & Verified |
| `0x005bebc0` | `?CanSaveNew@MSSave@@QBE_NXZ` | `bool MSSave::CanSaveNew(void) const` | Decompiled & Verified |
| `0x005bec10` | `?IsSavesUpdated@MSSave@@QAE_NXZ` | `bool MSSave::IsSavesUpdated(void)` | Decompiled & Verified |
| `0x005bdf10` | `?IsTransferDone@MSSave@@QAE_NXZ` | `bool MSSave::IsTransferDone(void)` | Decompiled & Verified |

### `src/game/main.cpp` & `src/game/main.h` (Original: `f:\tr7\game\main.c`)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x0045c450` | `?FSHelper_ReadFile@@YAPAHPADEPAVFileSystem@cdc@@@Z` | `void* FSHelper_ReadFile(const char*, unsigned char, cdc::FileSystem*)` | Decompiled & Verified |
| `0x0045bb80` | `Arg_FindFlag` | `const char* Arg_FindFlag(const char*, const char*)` | Decompiled & Verified |
| `0x0045bc40` | `Arg_GetValue` | `void Arg_GetValue(const char*, const char*, char*)` | Decompiled & Verified |
| `0x0045bbd0` | `Arg_GetPrefix` | `void Arg_GetPrefix(const char*, char*)` | Decompiled & Verified |
| `0x0045bc00` | `Arg_GetDigits` | `void Arg_GetDigits(const char*, char*)` | Decompiled & Verified |
| `0x0045c530` | `?ProcessArgs@@YAHPADPAUGameTracker@@PAX@Z` | `int ProcessArgs(char*, void*, void*)` | Decompiled & Verified |
| `0x0045c940` | `?MAIN_DoMainInit@@YAXXZ` | `void MAIN_DoMainInit(void)` | Decompiled & Verified |
| `0x0045cc10` | `?MainG2@@YAHPAX@Z` | `int MainG2(void *appData)` | Decompiled & Verified |

### `src/cdc/runtime/cdcRender/pc/shared/PCDeviceManager.cpp` & `.h` (Original: `cdcRender\pc\shared\PCDeviceManager.cpp`)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x00ec70f0` | `PCDeviceManager::Create` | `PCDeviceManager* PCDeviceManager::CreateInstance(void)` | Decompiled & Verified |
| `0x00ec6ff0` | `PCDeviceManager::PCDeviceManager` | `PCDeviceManager::PCDeviceManager(HMODULE, void*)` | Decompiled & Verified |
| `0x00ec6ea0` | `PCDeviceManager::~PCDeviceManager` | `PCDeviceManager::~PCDeviceManager(void)` | Decompiled & Verified |
| `0x00ec6350` | `PCDeviceManager::SetVideoConfig` | `bool PCDeviceManager::SetVideoConfig(HWND, const PCVideoConfig*)` | Decompiled & Verified |
| `0x00ec5f80` | `PCDeviceManager::CreateDevice` | `bool PCDeviceManager::CreateDevice(const PCVideoConfig*)` | Decompiled & Verified |
| `0x00ec6230` | `PCDeviceManager::ReleaseDevice` | `void PCDeviceManager::ReleaseDevice(uint32_t)` | Decompiled & Verified |
| `0x00ec62f0` | `PCDeviceManager::UpdateDeviceStatus`| `void PCDeviceManager::UpdateDeviceStatus(void)` | Decompiled & Verified |

### `src/game/archive/ArchiveFileSystem.cpp` & `src/game/archive/ArchiveFileSystem.h` (Original: `f:\tr7\game\archive\ArchiveFileSystem.cpp`)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x0045c2a0` | `?InitFS@@YAXXZ` | `void InitFS(void)` | Decompiled & Verified |
| `0x0045c300` | `?InitArchive@@YAXXZ` | `void InitArchive(void)` | Decompiled & Verified |
| `0x0045c360` | `?ReleaseFS@@YAXXZ` | `void ReleaseFS(void)` | Decompiled & Verified |
| `0x0047a990` | `ArchiveFileSystem::Open` | `bool ArchiveFileSystem::Open(const char*)` | Decompiled & Verified |
| `0x0047ac70` | `ArchiveFileSystem::ArchiveFileSystem` | `ArchiveFileSystem::ArchiveFileSystem(FileSystem*)` | Decompiled & Verified |

### `src/game/pc/pclocals.cpp` & `src/game/pc/pclocals.h` (Original: `f:\tr7\game\pc\pclocals.cpp`)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x0045c0e0` | `?SetupBuildDir@@YAXPBD@Z` | `void SetupBuildDir(const char*)` | Decompiled & Verified |
| `0x004e3fa0` | `localstr_verifyexistence` | `bool localstr_verifyexistence(void)` | Decompiled & Verified |
| `0x004e4010` | `localstr_set_language` | `void localstr_set_language(int)` | Decompiled & Verified |
| `0x004e4050` | `localstr_get` / `PCLOCALS_GetFixedIDLocal` | `const char *localstr_get(int)` | Decompiled & Verified |

### `src/game/pc/D3D/rnd_d3d.cpp` & `src/game/pc/D3D/rnd_d3d.h` (Original: `f:\tr7\game\pc\d3d\rnd_d3d.cpp`)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x00413880` | `D3D_InitManager` | `bool D3D_InitManager(void)` | Decompiled & Verified |
| `0x004139a0` | `D3D_InitDevice` | `bool D3D_InitDevice(bool)` | Decompiled & Verified |
| `0x00413930` | `D3D_ShutdownDevice` | `void D3D_ShutdownDevice(void)` | Decompiled & Verified |
| `0x00413a20` | `RND_Shutdown` | `void RND_Shutdown(void)` | Decompiled & Verified |

### `src/game/pc/gamewindow.cpp` & `src/game/pc/gamewindow.h` (Original: `f:\tr7\game\pc\gamewindow.cpp`)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x00403fd0` | `GAMEWINDOW_Init` | `void GAMEWINDOW_Init(void)` | Decompiled & Verified |
| `0x004040c0` | `GAMEWINDOW_Deinit` | `void GAMEWINDOW_Deinit(void)` | Decompiled & Verified |
| `0x004040e0` | `GAMEWINDOW_Poll` | `int GAMEWINDOW_Poll(void)` | Decompiled & Verified |
| `0x00403c00` | `RegularWndProc` | `LRESULT CALLBACK RegularWndProc(HWND, UINT, WPARAM, LPARAM)` | Decompiled & Verified |

### `src/game/pc/timer.cpp` & `src/game/pc/timer.h` (Original: `f:\tr7\game\pc\timer.c`)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x00402570` | `TIMER_Init` | `void TIMER_Init(void)` | Decompiled & Verified |
| `0x00402520` | `WIN_TimeUS` | `uint64_t WIN_TimeUS(void)` | Decompiled & Verified |
| `0x00402590` | `TIMER_GetTimeUS` | `uint64_t TIMER_GetTimeUS(void)` | Decompiled & Verified |
| `0x00402ba0` | `TIMER_GetTimeMS` | `uint32_t TIMER_GetTimeMS(void)` | Decompiled & Verified |
| `0x00402610` | `TIMER_TimeDiff` | `uint32_t TIMER_TimeDiff(uint32_t, uint32_t)` | Decompiled & Verified |

### `src/game/pc/setup.cpp` & `src/game/pc/setup.h` (Original: `f:\tr7\game\pc\setup.cpp`)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x00404a40` | `GAMEWINDOW_ReadSettings` | `int SETUP_LoadRegistrySettings(void)` | Decompiled & Verified |
| `0x004044c0` | `GAMEWINDOW_ReadUserPreferences` | `void SETUP_LoadUserPreferences(void)` | Decompiled & Verified |
| `0x00404e00` | `GAMEWINDOW_SaveSettings` | `void SETUP_SaveRegistrySettings(void)` | Decompiled & Verified |
| `0x004056c0` | `SETUP_DoSetupDialog` | `int SETUP_DoSetupDialog(int, int)` | Decompiled & Verified |
| `0x004057c0` | `SETUP_LocalizeSetupDialog` | `void SETUP_LocalizeSetupDialog(void)` | Decompiled & Verified |

### `src/game/pc/pcmouse.cpp` & `src/game/pc/pcmouse.h` (Original: `f:\tr7\game\pc\pcmouse.cpp`)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x004051b0` | `?PCMOUSE_SetCursorPos@@YAXMM@Z` | `void PCMOUSE_SetCursorPos(float, float)` | Decompiled & Verified |
| `0x00405280` | `?PCMOUSE_IsMouseActive@@YA_NXZ` | `bool PCMOUSE_IsMouseActive(void)` | Decompiled & Verified |
| `0x004052a0` | `?PCMOUSE_DeActivateCursor@@YAXXZ` | `void PCMOUSE_DeActivateCursor(void)` | Decompiled & Verified |
| `0x004052b0` | `?PCMOUSE_ActivateCursor@@YAXXZ` | `void PCMOUSE_ActivateCursor(void)` | Decompiled & Verified |
| `0x004052d0` | `?PCMOUSE_GetCursorPos@@YAIPAM0@Z` | `uint32_t PCMOUSE_GetCursorPos(float*, float*)` | Decompiled & Verified |
| `0x00405300` | `?PCMOUSE_UpdateDelta@@YAXMM@Z` | `void PCMOUSE_UpdateDelta(float, float)` | Decompiled & Verified |
| `0x004053f0` | `?PCMOUSE_UpdateButtonDown@@YAXH@Z` | `void PCMOUSE_UpdateButtonDown(int)` | Decompiled & Verified |
| `0x00405400` | `?PCMOUSE_UpdateButtonUp@@YAXH@Z` | `void PCMOUSE_UpdateButtonUp(int)` | Decompiled & Verified |
| `0x00405410` | `?PCMOUSE_ScrollMouseWheel@@YAXH@Z` | `void PCMOUSE_ScrollMouseWheel(int)` | Decompiled & Verified |
| `0x00405430` | `?PCMOUSE_GetMouseWheelPos@@YAHXZ` | `int PCMOUSE_GetMouseWheelPos(void)` | Decompiled & Verified |
| `0x00405440` | `?PCMOUSE_SetMouseWheelPos@@YAXH@Z` | `void PCMOUSE_SetMouseWheelPos(int)` | Decompiled & Verified |
| `0x00405450` | `?PCMOUSE_GetButtonState@@YAHH@Z` | `int PCMOUSE_GetButtonState(int)` | Decompiled & Verified |
| `0x00405470` | `?PCMOUSE_DisplayCursor@@YA_NXZ` | `bool PCMOUSE_DisplayCursor(void)` | Decompiled & Verified |

### `src/game/pc/pcinput.cpp` & `src/game/pc/pcinput.h` (Original: `f:\tr7\game\pc\pcinput.cpp`)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x00411850` | `?Create@InputDevice@@SAXXZ` | `void InputDevice::Create(void)` | Decompiled & Verified |
| `0x0040fec0` | `?Destroy@InputDevice@@SAXXZ` | `void InputDevice::Destroy(void)` | Decompiled & Verified |
| `0x0040fe30` | `?Update@InputDevice@@SAXXZ` | `void InputDevice::Update(void)` | Decompiled & Verified |
| `0x0040f800` | `?GetDevicePtr@InputDevice@@SAPAV1@H@Z` | `InputDevice* InputDevice::GetDevicePtr(int)` | Decompiled & Verified |
| `0x00411930` | `?EnableMouse@InputDevice@@SAX_N@Z` | `void InputDevice::EnableMouse(bool)` | Decompiled & Verified |
| `0x0040f750` | `?IsMouseEnabled@InputDevice@@SA_NXZ` | `bool InputDevice::IsMouseEnabled(void)` | Decompiled & Verified |
| `0x0040f760` | `?EnableMouseButtons@InputDevice@@SA_N_N@Z` | `bool InputDevice::EnableMouseButtons(bool)` | Decompiled & Verified |
| `0x00411490` | `PCKeyboardDevice::Poll` | `int PCKeyboardDevice::Poll(UpdateInfo*, int, bool)` | Decompiled & Verified |
| `0x004111d0` | `PCMouseDevice::Poll` | `int PCMouseDevice::Poll(UpdateInfo*, int, bool)` | Decompiled & Verified |
| `0x00410a30` | `PCGamepadDevice::Poll` | `int PCGamepadDevice::Poll(UpdateInfo*, int, bool)` | Decompiled & Verified |
| `0x0040fd20` | `PCCombinationInputDevice::Poll` | `int PCCombinationInputDevice::Poll(UpdateInfo*, int, bool)` | Decompiled & Verified |

### `src/game/sound/sound.cpp` & `src/game/sound/sound.h` (Original: `f:\tr7\game\sound\sound.cpp` / FMOD)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x005d4b00` | `?SOUND_Init@@YAXXZ` | `void SOUND_Init(void)` | Decompiled & Verified |
| `0x005cc070` | `?SOUND_Free@@YAXXZ` | `void SOUND_Free(void)` | Decompiled & Verified |
| `0x005d3e00` | `?SOUND_LoadBank@@YAHPBDH@Z` | `int SOUND_LoadBank(const char*, int)` | Decompiled & Verified |
| `0x00462220` | `?OBTABLE_GetObjectID@@YAHPAD@Z` | `int OBTABLE_GetObjectID(const char*)` | Decompiled & Verified |

### `src/game/Stream/stream.cpp` & `src/game/Stream/stream.h` (Original: `f:\tr7\game\Stream\stream.cpp`)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x005d3c40` | `?STREAM_Init@@YAXXZ` | `void STREAM_Init(void)` | Decompiled & Verified |
| `0x005d3bc0` | `?STREAM_AbortAreaLoad@@YAXXZ` | `void STREAM_AbortAreaLoad(void)` | Decompiled & Verified |
| `0x005d4400` | `?STREAM_GetStreamUnitWithID@@YAPAUStreamUnit@@H@Z` | `StreamUnit* STREAM_GetStreamUnitWithID(int)` | Decompiled & Verified |
| `0x005d4440` | `?STREAM_GetStreamUnitWithName@@YAPAUStreamUnit@@PBD@Z` | `StreamUnit* STREAM_GetStreamUnitWithName(const char*)` | Decompiled & Verified |
| `0x005d43c0` | `?STREAM_GetLevelWithID@@YAPAULevel@@H@Z` | `Level* STREAM_GetLevelWithID(int)` | Decompiled & Verified |
| `0x005d48f0` | `?STREAM_MarkUnitNeeded@@YAXH@Z` | `void STREAM_MarkUnitNeeded(int)` | Decompiled & Verified |
| `0x005d4930` | `?STREAM_DumpUnit@@YAXPAUStreamUnit@@H@Z` | `void STREAM_DumpUnit(StreamUnit*, int)` | Decompiled & Verified |
| `0x005d4ac0` | `?STREAM_DumpAllUnitsNotNeeded@@YAXXZ` | `void STREAM_DumpAllUnitsNotNeeded(void)` | Decompiled & Verified |
| `0x005d4b00` | `?STREAM_DumpAllLevels@@YAXHH@Z` | `void STREAM_DumpAllLevels(int, int)` | Decompiled & Verified |
| `0x005d4d50` | `?STREAM_PollLoadQueue@@YAXXZ` | `int STREAM_PollLoadQueue(void)` | Decompiled & Verified |
| `0x005d5720` | `?STREAM_LoadLevel@@YAPAUStreamUnit@@PBDPAUStreamPortal@@H@Z` | `StreamUnit* STREAM_LoadLevel(const char*, void*, int)` | Decompiled & Verified |
| `0x005d5940` | `?STREAM_WaitForUnit@@YAXPAUStreamUnit@@@Z` | `void STREAM_WaitForUnit(StreamUnit*)` | Decompiled & Verified |

### `src/game/obtable.cpp` & `src/game/obtable.h` (Original: `f:\tr7\game\obtable.c`)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x00461c60` | `?OBTABLE_InstanceInit@@YAXPAUInstance@@@Z` | `void OBTABLE_InstanceInit(Instance*)` | Decompiled & Verified |
| `0x00461d00` | `?OBTABLE_GetInstanceCollideFunc@@YAXPAUInstance@@@Z` | `void OBTABLE_GetInstanceCollideFunc(Instance*)` | Decompiled & Verified |
| `0x00461d30` | `?OBTABLE_GetInstanceAdditionalCollideFunc@@YAXPAUInstance@@@Z` | `void OBTABLE_GetInstanceAdditionalCollideFunc(Instance*)` | Decompiled & Verified |
| `0x00461d60` | `?OBTABLE_GetInstanceProcessFunc@@YAXPAUInstance@@@Z` | `void OBTABLE_GetInstanceProcessFunc(Instance*)` | Decompiled & Verified |
| `0x00461d90` | `?OBTABLE_GetInstanceQueryFunc@@YAXPAUInstance@@@Z` | `void OBTABLE_GetInstanceQueryFunc(Instance*)` | Decompiled & Verified |
| `0x00461dc0` | `?OBTABLE_GetInstanceMessageFunc@@YAXPAUInstance@@@Z` | `void OBTABLE_GetInstanceMessageFunc(Instance*)` | Decompiled & Verified |
| `0x00461df0` | `?OBTABLE_InitObjectWithID@@YAXPAUObject@@@Z` | `void OBTABLE_InitObjectWithID(Object*)` | Decompiled & Verified |
| `0x00461eb0` | `?OBTABLE_SetObjectAccess@@YAXPAUObject@@@Z` | `void OBTABLE_SetObjectAccess(Object*)` | Decompiled & Verified |
| `0x00461ef0` | `?OBTABLE_ClearObjectReferences@@YAXXZ` | `void OBTABLE_ClearObjectReferences(void)` | Decompiled & Verified |
| `0x00461f10` | `?OBTABLE_RemoveObjectEntry@@YAXPAUObject@@@Z` | `void OBTABLE_RemoveObjectEntry(Object*)` | Decompiled & Verified |
| `0x00461f40` | `?OBTABLE_FindObject@@YAPAUObject@@H@Z` | `Object* OBTABLE_FindObject(int)` | Decompiled & Verified |
| `0x00461f70` | `?OBTABLE_InitAnimPointers@@YAXPAUObjectTracker@@@Z` | `void OBTABLE_InitAnimPointers(ObjectTracker*)` | Decompiled & Verified |
| `0x00461f90` | `?OBTABLE_Init@@YAXXZ` | `void OBTABLE_Init(void)` | Decompiled & Verified |
| `0x00462220` | `?OBTABLE_GetObjectID@@YAHPAD@Z` | `int OBTABLE_GetObjectID(char*)` | Decompiled & Verified |

### `src/game/instance.cpp` & `src/game/instance.h` (Original: `f:\tr7\game\instance.c`)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x004545b0` | `?INSTANCE_InitInstanceList@@YAXXZ` | `void INSTANCE_InitInstanceList(void)` | Decompiled & Verified |
| `0x00454670` | `?INSTANCE_NewInstance@@YAPAUInstance@@XZ` | `Instance* INSTANCE_NewInstance(void)` | Decompiled & Verified |
| `0x004546d0` | `?INSTANCE_InsertInstanceGroup@@YAXPAUInstance@@@Z` | `void INSTANCE_InsertInstanceGroup(Instance*)` | Decompiled & Verified |
| `0x004553f0` | `?INSTANCE_Find@@YAPAUInstance@@H@Z` | `Instance* INSTANCE_Find(int)` | Decompiled & Verified |
| `0x00455bb0` | `?INSTANCE_SetModel@@YAXPAUInstance@@H@Z` | `void INSTANCE_SetModel(Instance*, int)` | Decompiled & Verified |
| `0x00455db0` | `?INSTANCE_SetCurrentStreamUnitID@@YAXPAUInstance@@H@Z` | `void INSTANCE_SetCurrentStreamUnitID(Instance*, int)` | Decompiled & Verified |
| `0x00456770` | `?INSTANCE_PlainDeath@@YAXPAUInstance@@@Z` | `void INSTANCE_PlainDeath(Instance*)` | Decompiled & Verified |
| `0x004567c0` | `?INSTANCE_KillInstance@@YAXPAUInstance@@@Z` | `void INSTANCE_KillInstance(Instance*)` | Decompiled & Verified |
| `0x00457a20` | `?INSTANCE_BirthCommon@@YAXPAUInstance@@PAUObject@@HH@Z` | `void INSTANCE_BirthCommon(Instance*, Object*, int, int)` | Decompiled & Verified |
| `0x004582a0` | `?INSTANCE_IntroduceInstance@@YAPAUInstance@@PAUIntro@@F_N@Z` | `Instance* INSTANCE_IntroduceInstance(Intro*, short, bool)` | Decompiled & Verified |
| `0x00458600` | `?INSTANCE_BirthObjectNoParent@@YAPAUInstance@@IPAVVector3@cdc@@PAVEuler@3@PAUIntroData@@PAUObject@@HH@Z` | `Instance* INSTANCE_BirthObjectNoParent(unsigned int, cdc::Vector3*, cdc::Euler*, IntroData*, Object*, int, int)` | Decompiled & Verified |
| `0x00458750` | `?INSTANCE_BirthObject@@YAPAUInstance@@PAU1@PAUObject@@HH@Z` | `Instance* INSTANCE_BirthObject(Instance*, Object*, int, int)` | Decompiled & Verified |
| `0x00458840` | `?INSTANCE_IntroduceSavedInstance@@YAPAUInstance@@PAUSavedIntro@@PAUStreamUnit@@PAH@Z` | `Instance* INSTANCE_IntroduceSavedInstance(SavedIntro*, StreamUnit*, int*)` | Decompiled & Verified |

### `src/game/gameloop.cpp` & `src/game/gameloop.h` (Original: `f:\tr7\game\gameloop.cpp`)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x0044f5d0` | `?GAMELOOP_AllocStaticMemory@@YAXXZ` | `void GAMELOOP_AllocStaticMemory(void)` | Decompiled & Verified |
| `0x0044f630` | `?GAMELOOP_InitGameTracker@@YAXXZ` | `void GAMELOOP_InitGameTracker(void)` | Decompiled & Verified |
| `0x0044f6d0` | `?GAMELOOP_SystemInit@@YAXPAUGameTracker@@@Z` | `void GAMELOOP_SystemInit(GameTracker*)` | Decompiled & Verified |
| `0x0044f710` | `?GAMELOOP_ResetGameStates@@YAXXZ` | `void GAMELOOP_ResetGameStates(void)` | Decompiled & Verified |
| `0x0044f740` | `?GAMELOOP_ClearGameTracker@@YAXXZ` | `void GAMELOOP_ClearGameTracker(void)` | Decompiled & Verified |
| `0x0044f7e0` | `?GAMELOOP_IntroduceInstances@@YAXPAUGameTracker@@@Z` | `void GAMELOOP_IntroduceInstances(GameTracker*)` | Decompiled & Verified |
| `0x0044f820` | `?GAMELOOP_LoadingDisplayProcess@@YAXPAUInstance@@PAUGameTracker@@@Z` | `void GAMELOOP_LoadingDisplayProcess(void*, GameTracker*)` | Decompiled & Verified |
| `0x0044f830` | `?GAMELOOP_InitRunning@@YAXPAD0@Z` | `void GAMELOOP_InitRunning(char*, char*)` | Decompiled & Verified |
| `0x0044fa30` | `?GAMELOOP_CloseRunning@@YAXXZ` | `void GAMELOOP_CloseRunning(void)` | Decompiled & Verified |
| `0x0044faa0` | `?GAMELOOP_DisplayRunning@@YAXPAUStreamUnit@@@Z` | `void GAMELOOP_DisplayRunning(StreamUnit*)` | Decompiled & Verified |
| `0x0044fba0` | `?GAMELOOP_InitStaticDefault@@YAXPAD0@Z` | `void GAMELOOP_InitStaticDefault(char*, char*)` | Decompiled & Verified |
| `0x0044fc50` | `?GAMELOOP_InitStaticChapter@@YAXPAD0H@Z` | `void GAMELOOP_InitStaticChapter(char*, char*, int)` | Decompiled & Verified |
| `0x0044fd40` | `?GAMELOOP_InitStaticDeath@@YAXPAD0@Z` | `void GAMELOOP_InitStaticDeath(char*, char*)` | Decompiled & Verified |
| `0x0044fd90` | `?GAMELOOP_DisplayStatic@@YAXXZ` | `void GAMELOOP_DisplayStatic(void)` | Decompiled & Verified |
| `0x0044fdc0` | `?GAMELOOP_CloseStatic@@YAXXZ` | `void GAMELOOP_CloseStatic(void)` | Decompiled & Verified |
| `0x0044fde0` | `?GAMELOOP_SetLoadingDisplay@@YAXH@Z` | `void GAMELOOP_SetLoadingDisplay(int)` | Decompiled & Verified |
| `0x0044fdf0` | `?GAMELOOP_InitStandardObjects@@YAXXZ` | `void GAMELOOP_InitStandardObjects(void)` | Decompiled & Verified |
| `0x0044fe20` | `?GAMELOOP_IntroducePlayerInstance@@YAPAUInstance@@HMMMH@Z` | `Instance* GAMELOOP_IntroducePlayerInstance(int, float, float, float, int)` | Decompiled & Verified |
| `0x0044ff60` | `?GAMELOOP_ResetAllScreenWipes@@YAXXZ` | `void GAMELOOP_ResetAllScreenWipes(void)` | Decompiled & Verified |
| `0x0044ff80` | `?GAMELOOP_IsWipeDone@@YAHH@Z` | `int GAMELOOP_IsWipeDone(int)` | Decompiled & Verified |
| `0x0044ffb0` | `?GAMELOOP_IsWipeStartingOrON@@YAHH@Z` | `int GAMELOOP_IsWipeStartingOrON(int)` | Decompiled & Verified |
| `0x00450010` | `?GAMELOOP_AreAnyScreenFadesOnOrStarting@@YAHXZ` | `int GAMELOOP_AreAnyScreenFadesOnOrStarting(void)` | Decompiled & Verified |
| `0x00450040` | `?GAMELOOP_SetScreenWipe@@YAXHHH@Z` | `void GAMELOOP_SetScreenWipe(int, int, int)` | Decompiled & Verified |
| `0x004500a0` | `?GAMELOOP_HandleScreenWipes@@YAXXZ` | `void GAMELOOP_HandleScreenWipes(void)` | Decompiled & Verified |
| `0x004506f0` | `?GAMELOOP_GetMainRenderUnit@@YAPAUStreamUnit@@XZ` | `StreamUnit* GAMELOOP_GetMainRenderUnit(void)` | Decompiled & Verified |
| `0x00450770` | `?GAMELOOP_DisplayGameElements@@YA_NPAUGameTracker@@@Z` | `bool GAMELOOP_DisplayGameElements(GameTracker*)` | Decompiled & Verified |
| `0x00450a30` | `?GAMELOOP_DisplayFrame@@YAXPAUGameTracker@@@Z` | `void GAMELOOP_DisplayFrame(GameTracker*)` | Decompiled & Verified |
| `0x00450a90` | `TimeWarpRamp` | `float TimeWarpRamp(float, float, float)` | Decompiled & Verified |
| `0x00450ad0` | `?GAMELOOP_DoTimeProcess@@YAXXZ` | `void GAMELOOP_DoTimeProcess(void)` | Decompiled & Verified |
| `0x004511c0` | `?GAMELOOP_ModeStartRunning@@YAXH@Z` | `void GAMELOOP_ModeStartRunning(int)` | Decompiled & Verified |
| `0x00451270` | `?GAMELOOP_ModeStartPause@@YAX_N@Z` | `void GAMELOOP_ModeStartPause(bool)` | Decompiled & Verified |
| `0x00451360` | `?GAMELOOP_ChangeMode@@YAXH@Z` | `void GAMELOOP_ChangeMode(int)` | Decompiled & Verified |
| `0x00451680` | `?GAMELOOP_ExitGame@@YAXH@Z` | `void GAMELOOP_ExitGame(int)` | Decompiled & Verified |
| `0x00451780` | `?GAMELOOP_RequestLevelChangeByName@@YAXPADPAUGameTracker@@H@Z` | `void GAMELOOP_RequestLevelChangeByName(char*, GameTracker*, int)` | Decompiled & Verified |
| `0x004517e0` | `?GAMELOOP_RequestLevelChange@@YAXPADHPAUGameTracker@@@Z` | `void GAMELOOP_RequestLevelChange(char*, int, GameTracker*)` | Decompiled & Verified |
| `0x00451820` | `?GAMELOOP_LoadingDisplay@@YAXXZ` | `void GAMELOOP_LoadingDisplay(void)` | Decompiled & Verified |
| `0x00451910` | `?GAMELOOP_LevelLoadAndInit@@YAHPADPAUGameTracker@@@Z` | `int GAMELOOP_LevelLoadAndInit(char*, GameTracker*)` | Decompiled & Verified |
| `0x00451f90` | `?GAMELOOP_Process@@YAXPAUGameTracker@@@Z` | `void GAMELOOP_Process(GameTracker*)` | Decompiled & Verified |
| `0x00452360` | `?GAMELOOP_MainLoop@@YAXPAUGameTracker@@@Z` | `void GAMELOOP_MainLoop(GameTracker*)` | Decompiled & Verified |

### `src/cdc/cdcSys/UTF8String.cpp` & `src/cdc/cdcSys/UTF8String.h`
- String helper type `cdc::UTF8String` (constructors, destructor, `ConvertToWCHAR`, memory management).

### `src/game/player/player.cpp` & `src/game/player/player.h` (Original: `f:\tr7\game\player\player.c`)
- Size assertions for `MessageQueue` (0x08), `State` (0x34), `CharacterState` (0x38), `HealthSystem` (0x0c), `SplashSystem` (0x14), `AlgoSystem` (0x10), `PlayerHoldInfo` (0x30), `GeneralAttributes` (0x28), `OldRazielData` (0xc68), `PlayerProp` (0xc98), `Player` (0x1000), `PlayerData` (0x1070), `PlayerSM` (0x510).
- Subsystem globals: `gPlayer` (0x0111707c), `gPlayerSM` (0x01117494), `playerData` (0x01116f90), `s_playerInstance` (0x010e52bc), `s_timeScale` (0x010e53d8).
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x0059d5b0` | `RazielInit` | `void RazielInit(Instance*, GameTracker*)` | Decompiled & Verified |
| `0x0059aa90` | `RazielProcess` | `void RazielProcess(Instance*, GameTracker*)` | Decompiled & Verified |
| `0x00565ce0` | `RazielCollide` | `void RazielCollide(Instance*, GameTracker*)` | Decompiled & Verified |
| `0x0058a3e0` | `RazielQuery` | `unsigned int RazielQuery(Instance*, unsigned int)` | Decompiled & Verified |
| `0x00595060` | `RazielPost` | `void RazielPost(Instance*, unsigned int, unsigned int)` | Decompiled & Verified |
| `0x0058b060` | `RazielAdditionalCollide` | `void RazielAdditionalCollide(Instance*, GameTracker*)` | Decompiled & Verified |
| `0x00565cd0` | `RazielClearData` | `void RazielClearData(Instance*)` | Decompiled & Verified |
| `0x00565ed0` | `RAZIEL_SetLookAround` | `void RAZIEL_SetLookAround(Instance*)` | Decompiled & Verified |
| `0x00565f40` | `RAZIEL_ResetLookAround` | `void RAZIEL_ResetLookAround(Instance*)` | Decompiled & Verified |
| `0x00565fba` | `RAZIEL_OkToLookAround` | `bool RAZIEL_OkToLookAround(Instance*)` | Decompiled & Verified |
| `0x00579140` | `RAZIEL_DebugHealthFillUp` | `void RAZIEL_DebugHealthFillUp(void)` | Decompiled & Verified |
| `0x005aa7b0` | `RAZIEL_DebugHurtRaziel` | `void RAZIEL_DebugHurtRaziel(void)` | Decompiled & Verified |
| `0x0057d930` | `RAZIEL_TurnHead` | `void RAZIEL_TurnHead(Instance*)` | Decompiled & Verified |

### `src/game/player/control.cpp` & `src/game/player/control.h` (Original: `f:\tr7\game\player\control.c`)
- Physics and external forces subsystem for player character.
- Struct `Force` (0x20).
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x005b2db0` | `SetFallMaxZSpeed` | `void SetFallMaxZSpeed(Instance*, float)` | Decompiled & Verified |
| `0x005b2dc0` | `SetFallMaxSideSpeed` | `void SetFallMaxSideSpeed(Instance*, float)` | Decompiled & Verified |
| `0x005b2dd0` | `SetImpulsePhysics` | `void SetImpulsePhysics(Instance*, Player*)` | Decompiled & Verified |
| `0x005b2e80` | `InitExternalForces` | `void InitExternalForces(Force*, int)` | Decompiled & Verified |
| `0x005b2ec0` | `SetExternalForce` | `void SetExternalForce(Force*, float, float, float, unsigned char, float)` | Decompiled & Verified |
| `0x005b2ef0` | `SetPhysics` | `void SetPhysics(Instance*, float, float, float, float)` | Decompiled & Verified |
| `0x005b5850` | `ResetExternalForces` | `void ResetExternalForces(Instance*)` | Decompiled & Verified |
| `0x005b5910` | `ResetPhysics` | `void ResetPhysics(Instance*, float)` | Decompiled & Verified |
| `0x005b5980` | `SetDropPhysics` | `void SetDropPhysics(Instance*, Player*)` | Decompiled & Verified |
| `0x005b5b60` | `ApplyExternalLocalForces` | `void ApplyExternalLocalForces(Player*, Instance*, Force*, int, cdc::Vector3*)` | Decompiled & Verified |
| `0x005b5d00` | `ProcessPhysics` | `void ProcessPhysics(Player*, CharacterState*, int, int)` | Decompiled & Verified |

### `src/game/playercntrl.cpp` & `src/game/playercntrl.h` (Original: `f:\tr7\game\playercntrl.c`)
- Global player control toggle (`gPlayerControl_Enabled` at `0x00f159f4`).
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x00463b40` | `?PLAYERCNTRL_Enable@@YAXXZ` | `void PLAYERCNTRL_Enable(void)` | Decompiled & Verified |
| `0x00463b50` | `?PLAYERCNTRL_Disable@@YAXXZ` | `void PLAYERCNTRL_Disable(void)` | Decompiled & Verified |

### `src/game/camera/camera.cpp` & `src/game/camera/camera.h` (Original: `f:\tr7\game\camera\camera.c`)
- Size assertions for `CameraCore` (0x300), `CameraKey` (0x18), `FollowTransitionData` (0x24), `FollowSplinecamData` (0x50), `SplinecamData` (0x50), `SplinecamParams` (0x7c), `SavedCinematic` (0x230), `NextCamData` (0x140), `CameraDataUnion` (0x160), and `Camera` (0x1320).
- Subsystem globals: `theCamera` (0x010fc5a0 / PDB 0x010fd7a0), `g_cameraAxisFlags` (0x01075ac8), `camera_shakeOffset` (0x010fc360).
- Input axis scale routines: `INPUT_SetAxisScale` (0x00452570), `INPUT_GetAxisScale` (0x004525b0).
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x0047c030` | `CAMERA_Initialize` | `void CAMERA_Initialize(Camera*)` | Decompiled & Verified |
| `0x0047c570` | `CAMERA_Lock` | `void CAMERA_Lock(Camera*, int)` | Decompiled & Verified |
| `0x0047c590` | `CAMERA_Unlock` | `void CAMERA_Unlock(Camera*, int)` | Decompiled & Verified |
| `0x0047c5b0` | `CAMERA_SetSmoothValue` | `void CAMERA_SetSmoothValue(Camera*, int)` | Decompiled & Verified |
| `0x0047c610` | `CAMERA_Adjust_tilt_stay_in_place` | `void CAMERA_Adjust_tilt_stay_in_place(Camera*, int)` | Decompiled & Verified |
| `0x0047c6a0` | `CAMERA_Adjust_tilt` | `void CAMERA_Adjust_tilt(Camera*, int)` | Decompiled & Verified |
| `0x0047c720` | `CAMERA_Adjust_distance` | `void CAMERA_Adjust_distance(Camera*, int)` | Decompiled & Verified |
| `0x0047c770` | `CAMERA_Adjust_ZOffset` | `void CAMERA_Adjust_ZOffset(Camera*, int)` | Decompiled & Verified |
| `0x0047c780` | `CAMERA_Adjust_rotation` | `void CAMERA_Adjust_rotation(Camera*, int)` | Decompiled & Verified |
| `0x0047c820` | `CAMERA_Adjust_roll` | `void CAMERA_Adjust_roll(int, int)` | Decompiled & Verified |
| `0x0047c990` | `CAMERA_ChangeToUnderWater` | `void CAMERA_ChangeToUnderWater(Camera*, Instance*)` | Decompiled & Verified |
| `0x0047cab0` | `CAMERA_ChangeOutOfWade` | `void CAMERA_ChangeOutOfWade(Camera*, Instance*)` | Decompiled & Verified |
| `0x0047acc0` | `CAMERA_SaveMode` | `void CAMERA_SaveMode(Camera*, int)` | Decompiled & Verified |
| `0x0047b240` | `CAMERA_SetMode` | `void CAMERA_SetMode(Camera*, int)` | Decompiled & Verified |
| `0x0047d760` | `CAMERA_SetVertigo` | `void CAMERA_SetVertigo(int, int, int)` | Decompiled & Verified |
| `0x0047d860` | `CAMERA_SetValueByEvent` | `void CAMERA_SetValueByEvent(Camera*, int, int)` | Decompiled & Verified |
| `0x0047dc00` | `CAMERA_SetAxisInversionX` | `void CAMERA_SetAxisInversionX(bool)` | Decompiled & Verified |
| `0x0047dc30` | `CAMERA_SetAxisInversionY` | `void CAMERA_SetAxisInversionY(bool)` | Decompiled & Verified |
| `0x0047dc80` | `CAMERA_GetAxisInversionX` | `bool CAMERA_GetAxisInversionX(void)` | Decompiled & Verified |
| `0x0047dca0` | `CAMERA_GetAxisInversionY` | `bool CAMERA_GetAxisInversionY(void)` | Decompiled & Verified |
| `0x0047dcc0` | `CAMERA_SetAimAxisInversionX` | `void CAMERA_SetAimAxisInversionX(bool)` | Decompiled & Verified |
| `0x0047dcf0` | `CAMERA_SetAimAxisInversionY` | `void CAMERA_SetAimAxisInversionY(bool)` | Decompiled & Verified |
| `0x0047dd20` | `CAMERA_GetAimAxisInversionX` | `bool CAMERA_GetAimAxisInversionX(void)` | Decompiled & Verified |
| `0x0047dd40` | `CAMERA_GetAimAxisInversionY` | `bool CAMERA_GetAimAxisInversionY(void)` | Decompiled & Verified |
| `0x0047dd60` | `CAMERA_Restore` | `void CAMERA_Restore(Camera*, int)` | Decompiled & Verified |
| `0x0047de30` | `CAMERA_QueryMode` | `int CAMERA_QueryMode(Camera*)` | Decompiled & Verified |
| `0x0047e0e0` | `CAMERA_SetShakeNoPadshock` | `void CAMERA_SetShakeNoPadshock(Camera*, int, int)` | Decompiled & Verified |
| `0x0047e180` | `CAMERA_SetShake` | `void CAMERA_SetShake(Camera*, int, int)` | Decompiled & Verified |
| `0x0047e1e0` | `CAMERA_Relocate` | `void CAMERA_Relocate(Camera*, cdc::Vector3*, int)` | Decompiled & Verified |
| `0x0047e860` | `CAMERA_SetShakeRamp` | `void CAMERA_SetShakeRamp(int, int, int, int)` | Decompiled & Verified |
| `0x0047ff50` | `CAMERA_RestoreMode` | `void CAMERA_RestoreMode(Camera*)` | Decompiled & Verified |
| `0x004803e0` | `CAMERA_ChangeToOutOfWater` | `void CAMERA_ChangeToOutOfWater(Camera*, Instance*)` | Decompiled & Verified |

### Camera Support Subsystem (`src/game/camera/camerasupport.cpp`, `src/game/camera/camerasupport.h`)
Original Module: `f:\tr7\game\camera\cameraSupport.c` (PDB -> Retail shift: `-0x1b10`)

| Address (Retail) | Function / Symbol | Signature / Purpose | Status |
| :--- | :--- | :--- | :--- |
| `0x0048c6d0` | `CriticalDampValue` | `void CriticalDampValue(int, float*, float, float*, float*, float)` | Decompiled & Verified |
| `0x0048c840` | `CriticalDampValueControl` | `void CriticalDampValueControl(float*, float, float*, float*, float, float)` | Decompiled & Verified |
| `0x0048c8d0` | `CAMERA_CalcRotation` | `void CAMERA_CalcRotation(cdc::Euler*, const cdc::Vector3*, const cdc::Vector3*)` | Decompiled & Verified |
| `0x0048ca00` | `CAMERA_CircleAnglef` | `float CAMERA_CircleAnglef(float)` | Decompiled & Verified |
| `0x0048ca60` | `CAMERA_SetZRotation` | `void CAMERA_SetZRotation(Camera*, float)` | Decompiled & Verified |
| `0x0048ca90` | `CAMERA_SignedAngleDifference` | `float CAMERA_SignedAngleDifference(float, float)` | Decompiled & Verified |
| `0x0048caf0` | `CAMERA_FocusInstanceMoved` | `bool CAMERA_FocusInstanceMoved(Camera*)` | Decompiled & Verified |
| `0x0048cb80` | `CAMERA_AngleMoveTowardf` | `void CAMERA_AngleMoveTowardf(float*, float, float)` | Decompiled & Verified |
| `0x0048cc50` | `AngleMoveToward` | `void AngleMoveToward(float*, float, float)` | Decompiled & Verified |
| `0x0048cc70` | `Decouple_AngleMoveToward` | `void Decouple_AngleMoveToward(float*, float, float)` | Decompiled & Verified |
| `0x0048ccc0` | `CriticalDampPosition` | `void CriticalDampPosition(int, cdc::Vector3*, const cdc::Vector3*, cdc::Vector3*, cdc::Vector3*, float)` | Decompiled & Verified |
| `0x0048d100` | `CriticalDampPositionControl` | `void CriticalDampPositionControl(cdc::Vector3*, const cdc::Vector3*, cdc::Vector3*, cdc::Vector3*, float, float)` | Decompiled & Verified |
| `0x0048d200` | `CriticalDampAngle` | `void CriticalDampAngle(int, float*, float, float*, float*, float)` | Decompiled & Verified |
| `0x0048d2a0` | `CriticalDampAngleControl` | `void CriticalDampAngleControl(float*, float, float*, float*, float, float)` | Decompiled & Verified |
| `0x0048d340` | `CAMERA_CalcZRotation` | `float CAMERA_CalcZRotation(const cdc::Vector3*, const cdc::Vector3*)` | Decompiled & Verified |
| `0x0048d380` | `CAMERA_AngleDifference` | `float CAMERA_AngleDifference(float, float)` | Decompiled & Verified |
| `0x0048d410` | `CAMERA_LimitRotation` | `float CAMERA_LimitRotation(Camera*, float)` | Decompiled & Verified |
| `0x0048d580` | `CAMERA_CalcPosition` | `void CAMERA_CalcPosition(cdc::Vector3*, const cdc::Vector3*, const cdc::Euler*, float)` | Decompiled & Verified |

### Camera View Volume Subsystem (`src/game/camera/cameravv.cpp`, `src/game/camera/cameravv.h`)
Original Module: `f:\tr7\game\camera\cameraVV.c` (PDB -> Retail shift: `-0x1b10`)

| Address (Retail) | Function / Symbol | Signature / Purpose | Status |
| :--- | :--- | :--- | :--- |
| `0x0048d650` | `CAMERA_CalculateViewVolumeNormals` | `void CAMERA_CalculateViewVolumeNormals(CameraCore*)` | Decompiled & Verified |
| `0x0048d970` | `CAMERA_AspectAdjustMatrixf` | `void CAMERA_AspectAdjustMatrixf(cdc::Matrix*)` | Decompiled & Verified |
| `0x0048da40` | `CAMERA_SetNewProjDistance` | `void CAMERA_SetNewProjDistance(CameraCore*, float, float)` | Decompiled & Verified |
| `0x0048da60` | `CAMERA_SphereIntersectsViewVolume` | `int CAMERA_SphereIntersectsViewVolume(cdc::Vector3*, CameraCore*)` | Decompiled & Verified |
| `0x0048db70` | `CAMERA_CalcVVClipInfo` | `void CAMERA_CalcVVClipInfo(CameraCore*)` | Decompiled & Verified |
| `0x0048ddd0` | `CAMERA_CalculateWCTransform` | `void CAMERA_CalculateWCTransform(CameraCore*)` | Decompiled & Verified |
| `0x0048e100` | `CAMERA_SetViewVolume` | `void CAMERA_SetViewVolume(CameraCore*)` | Decompiled & Verified |
| `0x0048e120` | `CAMERA_SetDefaultViewVolume` | `void CAMERA_SetDefaultViewVolume(CameraCore*)` | Decompiled & Verified |
| `0x0048e150` | `CAMERA_SetProjDistance2` | `void CAMERA_SetProjDistance2(CameraCore*, float)` | Decompiled & Verified |
| `0x0048e210` | `CAMERA_SetProjDistance` | `void CAMERA_SetProjDistance(CameraCore*, float)` | Decompiled & Verified |

### Camera Focus Subsystem (`src/game/camera/camerafocus.cpp`, `src/game/camera/camerafocus.h`, `src/game/globalinfo.h`)
Original Module: `f:\tr7\game\camera\cameraFocus.c` (PDB -> Retail shift: `-0x1a30` to `-0x1a40`)

| Address (Retail) | Function / Symbol | Signature / Purpose | Status |
| :--- | :--- | :--- | :--- |
| `0x00485c50` | `CAMERA_CreateNewFocuspoint` | `void CAMERA_CreateNewFocuspoint(Camera*)` | Decompiled & Verified |
| `0x00485cc0` | `CAMERA_SetMaxVel` | `void CAMERA_SetMaxVel(Camera*)` | Decompiled & Verified |
| `0x00485cd0` | `CAMERA_FocusHeight` | `float CAMERA_FocusHeight(Camera*)` | Decompiled & Verified |
| `0x00485d50` | `CAMERA_SetInstanceFocus` | `void CAMERA_SetInstanceFocus(Camera*, Instance*)` | Decompiled & Verified |
| `0x00485df0` | `CAMERA_GetInterestInstFocus` | `static cdc::Vector3 CAMERA_GetInterestInstFocus(Camera*)` | Decompiled & Verified |
| `0x00486030` | `CAMERA_CalcFocusOffset` | `static cdc::Vector3 CAMERA_CalcFocusOffset(Camera*)` | Decompiled & Verified |
| `0x004860e0` | `CAMERA_SetFocus` | `void CAMERA_SetFocus(Camera*, cdc::Vector3*)` | Decompiled & Verified |
| `0x010c0998` | `globalInfo` | `GlobalInfo*` (`CameraDefaults` + global info layout) | Documented |

### Camera Behind Subsystem (`src/game/camera/camerabehind.cpp`, `src/game/camera/camerabehind.h`, `src/game/player/player.h`)
Original Module: `f:\tr7\game\camera\cameraBehind.c` (PDB -> Retail shift: `-0x19a0`)

| Address (Retail) | Function / Symbol | Signature / Purpose | Status |
| :--- | :--- | :--- | :--- |
| `0x00480470` | `CAMERA_CalcAutoCenterData` | `void CAMERA_CalcAutoCenterData(Camera*, float*, float*)` | Decompiled & Verified |
| `0x00480610` | `CAMERA_CalcFollowPosition` | `static void CAMERA_CalcFollowPosition(Camera*, cdc::Euler*)` | Decompiled & Verified |
| `0x00480a40` | `CAMERA_UpdateFocusDistance` | `static void CAMERA_UpdateFocusDistance(Camera*)` | Decompiled & Verified |
| `0x00480b80` | `CAMERA_UpdateFocusTilt` | `static void CAMERA_UpdateFocusTilt(Camera*)` | Decompiled & Verified |
| `0x00480c70` | `CAMERA_UpdateFocusRotate` | `static void CAMERA_UpdateFocusRotate(Camera*)` | Decompiled & Verified |
| `0x00480dc0` | `CameraGetShapeValue` | `float CameraGetShapeValue(shapeValueParams*)` | Decompiled & Verified |
| `0x00480e90` | `CAMERA_ResetFollowTransition` | `void CAMERA_ResetFollowTransition(Camera*)` | Decompiled & Verified |
| `0x00480ef0` | `CAMERA_CalcFocusOffsetForSwim` | `static cdc::Vector3 CAMERA_CalcFocusOffsetForSwim(Camera*)` | Decompiled & Verified |
| `0x00480f50` | `CAMERA_FollowPlayerSwimTilt` | `static void CAMERA_FollowPlayerSwimTilt(Camera*)` | Decompiled & Verified |
| `0x00481100` | `CAMERA_DesiredCombatCamAngle` | `static float CAMERA_DesiredCombatCamAngle(Camera*, const cdc::Vector3*)` | Decompiled & Verified |
| `0x00481180` | `CAMERA_FollowGoBehindPlayer` | `static void CAMERA_FollowGoBehindPlayer(Camera*, float)` | Decompiled & Verified |
| `0x00481550` | `CAMERA_OffsetRecentZPos` | `void CAMERA_OffsetRecentZPos(const cdc::Vector3&)` | Decompiled & Verified |
| `0x00481580` | `CAMERA_UpdateFocusRotationX` | `static void CAMERA_UpdateFocusRotationX(Camera*, Instance*)` | Decompiled & Verified |
| `0x004824b0` | `CAMERA_GenericCameraProcess` | `void CAMERA_GenericCameraProcess(Camera*)` | Decompiled & Verified |
| `EngagedInstance` | `struct EngagedInstance` | Player sensory engaged-instance list entry (`0x60` bytes) | Documented |

### Camera Look Subsystem (`src/game/camera/cameralook.cpp`, `src/game/camera/cameralook.h`, `src/game/player/player.h`)
Original Module: `f:\tr7\game\camera\cameraLook.c` (PDB -> Retail shift: `-0x1a40`)

| Address (Retail) | Function / Symbol | Signature / Purpose | Status |
| :--- | :--- | :--- | :--- |
| `0x00486450` | `CAMERA_SetStickyLookMode` | `void CAMERA_SetStickyLookMode(Camera*)` | Decompiled & Verified |
| `0x00486460` | `CAMERA_SetAccurateAimMode` | `void CAMERA_SetAccurateAimMode(Camera*)` | Decompiled & Verified |
| `0x00486470` | `CAMERA_SetLookFocusAndBase` | `void CAMERA_SetLookFocusAndBase(Camera*)` | Decompiled & Verified |
| `0x00486510` | `CAMERA_EndLook` | `void CAMERA_EndLook(Camera*)` | Decompiled & Verified |
| `0x00486730` | `CAMERA_BezierLine` | `static float CAMERA_BezierLine(float, float, float)` | Decompiled & Verified |
| `0x00486780` | `CAMERA_AccurateAimInterpTime` | `float CAMERA_AccurateAimInterpTime(Camera*)` | Decompiled & Verified |
| `0x004867a0` | `CAMERA_InitAimAssist` | `void CAMERA_InitAimAssist()` | Decompiled & Verified |
| `0x00486820` | `CAMERA_AimAssist` | `void CAMERA_AimAssist(Camera*, cdc::Vector3*)` | Decompiled & Verified |
| `0x00486c90` | `CAMERA_ResetStickyLookMode` | `void CAMERA_ResetStickyLookMode(Camera*)` | Decompiled & Verified |
| `0x00486cb0` | `CAMERA_ResetAccurateAimMode` | `void CAMERA_ResetAccurateAimMode(Camera*)` | Decompiled & Verified |
| `0x00486cd0` | `CAMERA_StartLookaroundMode` | `void CAMERA_StartLookaroundMode(Camera*)` | Decompiled & Verified |
| `0x00486df0` | `CAMERA_StartAccurateAimMode` | `void CAMERA_StartAccurateAimMode(Camera*)` | Decompiled & Verified |
| `0x00486fb0` | `CAMERA_ForceEndLookaroundMode` | `void CAMERA_ForceEndLookaroundMode(Camera*)` | Decompiled & Verified |
| `0x00486fd0` | `CAMERA_LookProcess` | `void CAMERA_LookProcess(Camera*)` | Decompiled & Verified |
| `0x00487540` | `CAMERA_AccurateAimProcessInputs` | `void CAMERA_AccurateAimProcessInputs(Camera*, float*, float*)` | Decompiled & Verified |
| `0x004877d0` | `CAMERA_AccurateAimProcess` | `void CAMERA_AccurateAimProcess(Camera*)` | Decompiled & Verified |
| `0x00487c30` | `CAMERA_TurretProcess` | `void CAMERA_TurretProcess(Camera*)` | Decompiled & Verified |
| `0x010fc57c` | `cameraTurretInterp` | `float cameraTurretInterp` | Documented |
| `TurretProp` / `TurretData` / `TurretState` | Turret camera data structs (`0x144` / `0x160` / `0x54` bytes) | Documented |
| `StateObj` / `StateMachine` | Player state machine layout (`0x14` / `0x4c` bytes) | Documented |

---

### Camera Spline Subsystem (`src/game/camera/cameraspline.h`, `src/game/camera/cameraspline.cpp`, `src/game/spline.h`)
Original Module: `f:\tr7\game\camera\cameraSpline.c` (`0x00488130`–`0x0048a17c` retail)
- Also updated `cdc::Quat` / `cdc::Dist2` (`0x004880e0`) in [src/cdc/runtime/cdcMath/Math.h](src/cdc/runtime/cdcMath/Math.h), `Level` (`0x130` bytes) in [src/game/Stream/stream.h](src/game/Stream/stream.h), `INSTANCE_FindIntroUniqueID` (`0x00455370`) in [src/game/instance.h](src/game/instance.h), and `CriticalDampPositionControl` 7-arg `transition` param in [src/game/camera/camerasupport.h](src/game/camera/camerasupport.h) / [src/game/camera/camerasupport.cpp](src/game/camera/camerasupport.cpp).

| Address | Symbol | Signature / Type | Status |
| :--- | :--- | :--- | :--- |
| `0x004880e0` | `cdc::Dist2` | `float Dist2(const Vector&, const Vector&)` | Decompiled & Verified |
| `0x00488130` | `CAMERA_ClearSplineKeyValues` | `void CAMERA_ClearSplineKeyValues(MultiSpline*)` | Decompiled & Verified |
| `0x00488140` | `CAMERA_ResetSplineCombatSettings` | `void CAMERA_ResetSplineCombatSettings(Camera*)` | Decompiled & Verified |
| `0x00488190` | `CAMERA_Lock_Control_Angle` | `void CAMERA_Lock_Control_Angle(Camera*)` | Decompiled & Verified |
| `0x004882a0` | `CAMERA_Unlock_Control_Angle` | `void CAMERA_Unlock_Control_Angle(Camera*)` | Decompiled & Verified |
| `0x004882b0` | `CAMERA_SetSpline0` | `void CAMERA_SetSpline0(Camera*, Intro*, int, int)` | Decompiled & Verified |
| `0x00488350` | `CAMERA_SetSpline1` | `void CAMERA_SetSpline1(Camera*, Intro*, int, int)` | Decompiled & Verified |
| `0x004883f0` | `CAMERA_SetSpline2` | `void CAMERA_SetSpline2(Camera*, Intro*, int, int)` | Decompiled & Verified |
| `0x00488490` | `CAMERA_SetSplineWidths` | `void CAMERA_SetSplineWidths(int, int, int, int)` | Decompiled & Verified |
| `0x004884c0` | `CAMERA_SourceNumber` | `int CAMERA_SourceNumber(Camera*)` | Decompiled & Verified |
| `0x00488500` | `CAMERA_ProcessSplineCamLevelData` | `int CAMERA_ProcessSplineCamLevelData(Camera*, SplineCameraLevelData*)` | Decompiled & Verified |
| `0x00488b50` | `CAMERA_SetSplineCameraFromLevelData` | `void CAMERA_SetSplineCameraFromLevelData(Camera*, Level*, int)` | Decompiled & Verified |
| `0x00488b70` | `CAMERA_NearestPointOnLine` | `float CAMERA_NearestPointOnLine(cdc::Vector3*, cdc::Vector3*, cdc::Vector3*, cdc::Vector3*)` | Decompiled & Verified |
| `0x00488c60` | `CAMERA_DoWideSplineCalc` | `void CAMERA_DoWideSplineCalc(Camera*, cdc::Vector3*, Spline*, int, float, cdc::Vector3*, float, float)` | Decompiled & Verified |
| `0x00488f70` | `CAMERA_DoWideSplineCalcZ` | `void CAMERA_DoWideSplineCalcZ(Camera*, cdc::Vector3*, Spline*, cdc::Vector3*, float, float)` | Decompiled & Verified |
| `0x004890e0` | `CAMERA_SplineGetNearestPointTarget` | `float CAMERA_SplineGetNearestPointTarget(Camera*, Spline*, cdc::Vector3*, int*, cdc::Vector3*, float*)` | Decompiled & Verified |
| `0x004893c0` | `CAMERA_CalcSplinecamPos` | `static void CAMERA_CalcSplinecamPos(Camera*, SplinecamData*, cdc::Vector3*, cdc::Vector3*)` | Decompiled & Verified |
| `0x004895a0` | `CAMERA_SplineProcess` | `void CAMERA_SplineProcess(Camera*)` | Decompiled & Verified |
| `0x00489ff0` | `CAMERA_CalcFollowSplinecamPos` | `void CAMERA_CalcFollowSplinecamPos(Camera*, cdc::Vector3*, cdc::Vector3*)` | Decompiled & Verified |
| `SplineKey` / `RSplineKey` / `SplineState` / `Spline` / `RSpline` / `MultiSpline` | Spline data structs (`0x40` / `0x20` / `0x08` / `0x50` / `0x30` / `0x70` bytes) | Documented |
| `SplineCameraLevelData` / `Level` | Level spline camera data & Level structs (`0x58` / `0x130` bytes) | Documented |

### Camera Cinematic & Support (`f:\tr7\game\camera\cameracinematic.cpp`, `src/game/camera/camerasupport.cpp`)

| Address | Symbol | Signature / Type | Status |
| :--- | :--- | :--- | :--- |
| `0x00483420` | `CAMERA_CinematicDone` | `int CAMERA_CinematicDone(Camera*)` | Decompiled & Verified |
| `0x00483440` | `CAMERA_CinematicProcess` | `void CAMERA_CinematicProcess(Camera*)` | Decompiled & Verified |
| `0x0048c980` | `CAMERA_SetTarget` | `void CAMERA_SetTarget(Camera*, const cdc::Vector3*)` | Decompiled & Verified |
| `0x010fd960` | `Cinematic_lastPosition` | `cdc::Vector3` (last cinematic camera position) | Documented |

---

## Build Verification
- Cross-compiler target: `i686-w64-mingw32-g++` (32-bit Windows PE)
- CMake library target: `libtrl_decomp_core.a` (clean compile and link)

---

### Camera Stack (`src/game/camera/camerastack.h`, `src/game/camera/camerastack.cpp`, `src/game/player/markup.h`)
| Function | Address (Retail / PDB) | Status | Source File |
|---|---|---|---|
| `CameraStackCircleAnglef` | `0x0048a180` / `0x0048bc40` | [x] Done | `camerastack.cpp` |
| `CameraStackCombineData` | `0x0048a1e0` / `0x0048bca0` | [x] Done | `camerastack.cpp` |
| `CameraStackUpdateCameraSystem` | `0x0048b240` / `0x0048cd00` | [x] Done | `camerastack.cpp` |
| `CameraStackRefreshCameraSystem` | `0x0048bc40` / `0x0048d700` | [x] Done | `camerastack.cpp` |
| `CameraStackRefreshCameraSystemResetTilt` | `0x0048bc50` / `0x0048d710` | [x] Done | `camerastack.cpp` |
| `CameraStackInit` | `0x0048bc60` / `0x0048d720` | [x] Done | `camerastack.cpp` |
| `CameraStackInitDTPID` | `0x0048bd10` / `0x0048d7d0` | [x] Done | `camerastack.cpp` |
| `CameraStackClearMarkupCam` | `0x0048bd40` / `0x0048d800` | [x] Done | `camerastack.cpp` |
| `CameraStackCombineAll` | `0x0048bd70` / `0x0048d830` | [x] Done | `camerastack.cpp` |
| `CameraStackClear` | `0x0048be70` / `0x0048d930` | [x] Done | `camerastack.cpp` |
| `CameraStackSetBase` | `0x0048bed0` / `0x0048d990` | [x] Done | `camerastack.cpp` |
| `CameraStackSetBaseDTPID` | `0x0048bf20` / `0x0048d9e0` | [x] Done | `camerastack.cpp` |
| `CameraStackSetBaseDTPIDRecheck` | `0x0048bf70` / `0x0048da30` | [x] Done | `camerastack.cpp` |
| `CameraStackAddCamera` | `0x0048bf90` / `0x0048da50` | [x] Done | `camerastack.cpp` |
| `CameraStackRemoveCamera` | `0x0048c020` / `0x0048db10` | [x] Done | `camerastack.cpp` |
| `CameraStackAddCameraDTPID` | `0x0048c0c0` / `0x0048dbd0` | [x] Done | `camerastack.cpp` |
| `CameraStackRemoveCameraDTPID` | `0x0048c0f0` / `0x0048dc00` | [x] Done | `camerastack.cpp` |
| `CameraStackProcessPlayerState` | `0x0048c110` / `0x0048dc20` | [x] Done | `camerastack.cpp` |
| `CameraStackMain` | `0x0048c6a0` / `0x0048e1b0` | [x] Done | `camerastack.cpp` |
- **Globals & Structs Added**: `theCameraStack` (`0x010fda98`), `safeGrabTimer` (`0x010fdd28`); `CameraDTPData` (`0x74`), `CameraStackInfo` (`0x08`), `movementCameraOverrides` (`0x10`), `CameraStack` (`0x290`) in `src/game/camera/camerastack.h`; `MarkUp` (`0x30`), `MarkUpBox` (`0x40`), `MarkupDTPCam` (`0x0c`), `PlayerMarkupCamInfo` (`0x10`) in `src/game/player/markup.h`; `GenericIntroData` (`0x18`), `IntroData` (`0x04`) in `src/game/instance.h`; markup/state fields and helper declarations in `src/game/player/player.h`.

---

### Camera Collide (`src/game/camera/cameracollide.h`, `src/game/camera/cameracollide.cpp`)
| Function | Address (Retail / PDB) | Status | Source File |
|---|---|---|---|
| `CAMERA_update_dist_debounced` | `0x00483c50` / `0x00485680` | [x] Done | `cameracollide.cpp` |
| `CAMERA_CalcIntersectAngle` | `0x00483c80` / `0x004856b0` | [x] Done | `cameracollide.cpp` |
| `CAMERA_SetupColInfo` | `0x00483d40` / `0x00485770` | [x] Done | `cameracollide.cpp` |
| `CAMERA_dampgetline` | `0x00483ec0` / `0x004858f0` | [x] Done | `cameracollide.cpp` |
| `CAMERA_GetLineAngle` | `0x00483f90` / `0x004859c0` | [x] Done | `cameracollide.cpp` |
| `CAMERA_ACNoForcedMovement` | `0x00484200` / `0x00485c30` | [x] Done | `cameracollide.cpp` |
| `CAMERA_AbsoluteCollision` | `0x00484830` / `0x00486260` | [x] Done | `cameracollide.cpp` |
| `CAMERA_SphereToSphereWithLines` | `0x00484910` / `0x00486340` | [x] Done | `cameracollide.cpp` |
| `CAMERA_DoPanicCheck` | `0x00485570` / `0x00486fa0` | [x] Done | `cameracollide.cpp` |
| `CAMERA_Panic` | `0x00485600` / `0x00487030` | [x] Done | `cameracollide.cpp` |
| `CAMERA_DoCameraCollision` | `0x004857b0` / `0x004871e0` | [x] Done | `cameracollide.cpp` |
- **Globals & Structs Added**: `previousCollision` (`0x010fd970`), `CameraLine_startPt[5]` (`0x010fd980`), `CameraLine_endPt[5]` (`0x010fd9d0`); `IndexedFace` (`0x0a`), `MTriangle` (`0x30`), `Mesh` (`0x50`), `TerrainGroup` (`0xb0`), `PCollideInfo` (`0x60`), `CameraCollisionInfo` (`0x90`) in `src/game/camera/cameracollide.h`; replaced manual `CAMERA_DoCameraCollision` externs in `cameralook.cpp` and `camerabehind.cpp` with `#include "game/camera/cameracollide.h"`.

---

### Animation Tracker (`src/game/anim/anitracker.h`, `src/game/anim/anitracker.cpp`)
| Function | Address (Retail / PDB) | Status | Source File |
|---|---|---|---|
| `ANITRACKER_AllocateAnimation` (`static`) | `0x00477a50` / `0x00479390` | [x] Done | `anitracker.cpp` |
| `ANITRACKER_FreeAnimation` | `0x00477ab0` / `0x004793f0` | [x] Done | `anitracker.cpp` |
| `ANITRACKER_FindAnimation` | `0x00477af0` / `0x00479430` | [x] Done | `anitracker.cpp` |
| `ANITRACKER_TranslateAnimationList` | `0x00477b30` / `0x00479470` | [x] Done | `anitracker.cpp` |
| `ANITRACKER_Init` | `0x00477b80` / `0x004794c0` | [x] Done | `anitracker.cpp` |
| `ANITRACKER_ResolveAnimation` | `0x00477ba0` / `0x004794e0` | [x] Done | `anitracker.cpp` |
- **Globals & Structs Added**: `gFirstEmptyAnimation` (`0x010fa350`), `gNumLoadedAnimations` (`0x010fa354`), `aniTracker[1024]` (`0x010fa358`); `AnimListEntry` (`0x04`), `_AniTracker` (`0x08`) in `src/game/anim/anitracker.h`.

---

### Math3D (`src/game/math3d.h`, `src/game/math3d.cpp`)
| Function | Address (Retail / PDB) | Status | Source File |
|---|---|---|---|
| `cdc::Vector::Len4` | `0x0045f5f0` / `0x00460da0` | [x] Done | `cdcMath/Math.h` |
| `MATH3D_QuatLogToMatrix` | `0x0045f690` / `0x00460e40` | [x] Done | `math3d.cpp` |
| `MATH3D_AngleFromVector` | `0x0045f800` / `0x00460fb0` | [x] Done | `math3d.cpp` |
| `MATH3D_AngleFromPosToPosf` | `0x0045f840` / `0x00460ff0` | [x] Done | `math3d.cpp` |
| `MATH3D_ElevationFromPosToPosf` | `0x0045f880` / `0x00461030` | [x] Done | `math3d.cpp` |
| `MATH3D_ElevationFromVector` | `0x0045f900` / `0x004610b0` | [x] Done | `math3d.cpp` |
| `MATH3D_RotationFromPosToPosf` | `0x0045f970` / `0x00461120` | [x] Done | `math3d.cpp` |
| `MATH3D_ZYXtoXYZf` | `0x0045f9d0` / `0x00461180` | [x] Done | `math3d.cpp` |
| `MATH3D_AngleDifff` | `0x0045fa20` / `0x004611d0` | [x] Done | `math3d.cpp` |
| `MATH3D_CheckAngleInRange` | `0x0045fa60` / `0x00461210` | [x] Done | `math3d.cpp` |
| `MATH3D_NearestPointOnPlane` | `0x0045fb40` / `0x004612f0` | [x] Done | `math3d.cpp` |
| `MATH3D_IntersectLineAndPlane` | `0x0045fba0` / `0x00461350` | [x] Done | `math3d.cpp` |
| `MATH3D_NearestPointOnLine` | `0x0045fc90` / `0x00461440` | [x] Done | `math3d.cpp` |
| `MATH3D_DistanceFromPointToLineSegment` | `0x0045fdb0` / `0x00461560` | [x] Done | `math3d.cpp` |
| `MATH3D_AngleBetweenVectorsf` | `0x0045fe20` / `0x004615d0` | [x] Done | `math3d.cpp` |
| `MATH3D_ConvertAngleToVector` | `0x0045feb0` / `0x00461660` | [x] Done | `math3d.cpp` |
| `MATH3D_EulerFromPosToPos` | `0x0045ff00` / `0x004616b0` | [x] Done | `math3d.cpp` |
| `MATH3D_ClampAngleInRange` | `0x0045ffc0` / `0x00461770` | [x] Done | `math3d.cpp` |
| `MATH3D_AngleMoveTowardf` | `0x00460060` / `0x00461810` | [x] Done | `math3d.cpp` |
| `MATH3D_RelativePositionf` | `0x00460120` / `0x004618d0` | [x] Done | `math3d.cpp` |
| `MATH3D_ConeDetectf` | `0x00460190` / `0x00461940` | [x] Done | `math3d.cpp` |
| `MATH3D_Solve2BoneIK` | `0x00460270` / `0x00461a20` | [x] Done | `math3d.cpp` |
| `MATH3D_CheckMatrixAxisAlignment` | `0x00460500` / `0x00461cb0` | [x] Done | `math3d.cpp` |
- **Notes**: Added `cdc::Vector::Len4()` in `src/cdc/runtime/cdcMath/Math.h`; replaced manual `MATH3D_IntersectLineAndPlane` extern in `cameracollide.cpp` with `#include "game/math3d.h"`.

---

### FootLockController / HandLockController (`src/game/anim/footlockcontroller.h`, `src/game/anim/footlockcontroller.cpp`)
| Function | Address (Retail / PDB) | Status | Source File |
|---|---|---|---|
| `FootLockController::Deactivate` | `0x00477bf0` / `0x00479530` | [x] Done | `footlockcontroller.cpp` |
| `FootLockController::IsActive` | `0x00477c00` / `0x00479540` | [x] Done | `footlockcontroller.cpp` |
| `FootLockController::SetSlideLock` | `0x00477c10` / `0x00479580` | [x] Done | `footlockcontroller.cpp` |
| `FootLockController::Reposition` | `0x00477c30` / `0x004795a0` | [x] Done | `footlockcontroller.cpp` |
| `FootLockController::SetHipLimits` | `0x00477d80` / `0x004796f0` | [x] Done | `footlockcontroller.cpp` |
| `HandLockController::SetLockPointCallback` | `0x00477d80` / `0x004796f0` | [x] Done | `footlockcontroller.cpp` |
| `FootLockController::DeactivateWithoutInterp` | `0x00477da0` / `0x00479710` | [x] Done | `footlockcontroller.cpp` |
| `HandLockController::Deactivate` | `0x00477dc0` / `0x00479730` | [x] Done | `footlockcontroller.cpp` |
| `HandLockController::IsActive` | `0x00477de0` / `0x00479750` | [x] Done | `footlockcontroller.cpp` |
| `HandLockController::SetLockPoint` | `0x00477df0` / `0x00479760` | [x] Done | `footlockcontroller.cpp` |
| `FootLockController::SkipSecondaryIkThisFrame` | `0x00477e10` / `0x00479780` | [x] Done | `footlockcontroller.cpp` |
| `FootLockController::RelocateLockPoint` | `0x00477e20` / `0x00479790` | [x] Done | `footlockcontroller.cpp` |
| `FootLockController::AdvanceInterp` | `0x00477e40` / `0x004797b0` | [x] Done | `footlockcontroller.cpp` |
| `FootLockController::FootLockController` | `0x00477fd0` / `0x00479940` | [x] Done | `footlockcontroller.cpp` |
| `FootLockController::FindLockPoint` | `0x004780f0` / `0x00479a60` | [x] Done | `footlockcontroller.cpp` |
| `FootLockController::Activate` | `0x004783f0` / `0x00479d60` | [x] Done | `footlockcontroller.cpp` |
| `HandLockController::HandLockController` | `0x00478450` / `0x00479dc0` | [x] Done | `footlockcontroller.cpp` |
| `HandLockController::CallbackImpl` | `0x00478540` / `0x00479eb0` | [x] Done | `footlockcontroller.cpp` |
| `HandLockController::HelperCallbackImpl` | `0x00478a00` / `0x0047a370` | [x] Done | `footlockcontroller.cpp` |
| `FootLockController::SolveIK` | `0x00478ab0` / `0x0047a420` | [x] Done | `footlockcontroller.cpp` |
| `FootLockController::SetBones` | `0x00479250` / `0x0047abc0` | [x] Done | `footlockcontroller.cpp` |
| `HandLockController::Callback` | `0x00479810` / `0x0047b180` | [x] Done | `footlockcontroller.cpp` |
| `HandLockController::HelperCallback` | `0x00479830` / `0x0047b1a0` | [x] Done | `footlockcontroller.cpp` |
| `FootLockController::Apply` | `0x00479850` / `0x0047b1c0` | [x] Done | `footlockcontroller.cpp` |
| `HandLockController::Activate` | `0x004798b0` / `0x0047b220` | [x] Done | `footlockcontroller.cpp` |
- **Globals & Structs Added**: `g_useSecondaryIK` (`0x010fc358`), `g_useToePitch` (`0x010fc359`); `LimbLockController` (`0x04`), `FootLockController` (`0x110`), `HandLockController` (`0x100`) in `src/game/anim/footlockcontroller.h`; `G2AnimSegValue` (`0x40`), `AnimController` (`0x70`), `AnimProcessor` (`0x5f0`), `AnimComponent` (`0x14`) in `src/game/anim/anim.h`; `cdc::` math methods (`Quat`, `Matrix`, `Vector3`, `Slerp`, `OrthonormalInverse3x4`) in `src/cdc/runtime/cdcMath/Math.h`; exposed `Player` controller pointers (`0xd0c`–`0xd1c`) and `isStuckToGround` (`0xfa0`) in `src/game/player/player.h`.

---

### RNodeController (`src/game/anim/rnodecontroller.h`, `src/game/anim/rnodecontroller.cpp`)
| Function | Address (Retail / PDB) | Status | Source File |
|---|---|---|---|
| `cdc::Quat::Build(Vector3, float)` | `0x005df360` / `0x005e1d90` | [x] Done | `cdcMath/Math.h` |
| `RNodeController::RNodeController` | `0x004799d0` / `0x0047b340` | [x] Done | `rnodecontroller.cpp` |
| `RNodeController::SetSlewTarget` | `0x00479a80` / `0x0047b3f0` | [x] Done | `rnodecontroller.cpp` |
| `RNodeController::DeactivateSlewing` | `0x00479ab0` / `0x0047b420` | [x] Done | `rnodecontroller.cpp` |
| `RNodeController::Deactivate` | `0x00479af0` / `0x0047b460` | [x] Done | `rnodecontroller.cpp` |
| `RNodeController::Relocate` | `0x00479b10` / `0x0047b480` | [x] Done | `rnodecontroller.cpp` |
| `RNodeController::CallbackImpl` | `0x00479b40` / `0x0047b4b0` | [x] Done | `rnodecontroller.cpp` |
| `RNodeController::Callback` | `0x0047a360` / `0x0047bcd0` | [x] Done | `rnodecontroller.cpp` |
| `RNodeController::Activate` | `0x0047a380` / `0x0047bcf0` | [x] Done | `rnodecontroller.cpp` |
| `RNodeController::SetLeaning` | `0x0047a460` / `0x0047bdd0` | [x] Done | `rnodecontroller.cpp` |
| `RNodeController::ActivateSlewing` | `0x0047a4b0` / `0x0047be20` | [x] Done | `rnodecontroller.cpp` |
- **Globals & Structs Added**: `RNodeController` (`0xd0`) in `src/game/anim/rnodecontroller.h`; exposed `InterpoBlendSlewAdjustRate` (`0x4b0`), `InterpoBlendSlewRadius` (`0x4b4`), `InterpoBlendSlewExaggeration` (`0x4b8`) on `OldRazielData` and typed `Player` controller pointers (`0xd0c`–`0xd1c`) in `src/game/player/player.h`.

---

### AnimPatternVM (`src/game/anim/animpatternvm.h`, `src/game/anim/animpatternvm.cpp`)
| Function | Address (Retail / PDB) | Status | Source File |
|---|---|---|---|
| `VirtualMachine::VirtualMachine` | `0x0046f570` / `0x00470ee0` | [x] Done | `animpatternvm.cpp` |
| `VirtualMachine::PushMethodContext` | `0x0046f5f0` / `0x00470f60` | [x] Done | `animpatternvm.cpp` |
| `VirtualMachine::Execute` | `0x0046f650` / `0x00470fc0` | [x] Done | `animpatternvm.cpp` |
| `VirtualMachine::PopFloat` | `0x0046f9e0` / `0x00471350` | [x] Done | `animpatternvm.cpp` |
| `VirtualMachine::PopInt` | `0x0046f9f0` / `0x00471360` | [x] Done | `animpatternvm.cpp` |
| `VirtualMachine::PushFloat` | `0x0046fa00` / `0x00471370` | [x] Done | `animpatternvm.cpp` |
| `VirtualMachine::PushInt` | `0x0046fa20` / `0x00471390` | [x] Done | `animpatternvm.cpp` |
| `VirtualMachine::PushNative` | `0x0046fa20` / `0x00471390` | [x] Done | `animpatternvm.cpp` |
| `VirtualMachine::PushBool` | `0x0046fa40` / `0x004713b0` | [x] Done | `animpatternvm.cpp` |
| `VirtualMachine::~VirtualMachine` | `0x0046fa60` / `0x004713e0` | [x] Done | `animpatternvm.cpp` |
| `VirtualMachine::CallVirtualMethod` | `0x0046fab0` / `0x00471430` | [x] Done | `animpatternvm.cpp` |
- **Globals & Structs Added**: `AnimScriptObject` (`0x04`), `AnimDataValue` (`0x04`), `NativePlayer` (`0x1c`), `VirtualMachine::MethodContext` (`0x0c`), `VirtualMachine` (`0x28`), `gNativeClassTable` (`0x00f15c60`) in `src/game/anim/animpatternvm.h`.

---

### AnimProcessor (`src/game/anim/anim.h`, `src/game/anim/animprocessor.cpp`)
| Component / Range | Address (Retail / PDB) | Status | Source File |
|---|---|---|---|
| `AnimProcessor`, `AnimFragmentProcessor`, `AnimController`, `AnimTweaker`, `G2AnimAlphaTable`, & module helpers (131 functions) | `0x0046fad0`–`0x004779bf` / `0x00471450`–`0x00479340` | [x] Done | `animprocessor.cpp` |
| Static initializers `$E7` / `$E9` / `$E11` (`sUserTransformRotation`) | `0x00efa9a0` / `0x00efa9f0` / `0x00efaa40` | [x] Done | `animprocessor.cpp` |
- **Globals & Structs Added**: `_interpStateBlockPool` (`0x010fa300`), `_controllerPool` (`0x010fa310`), `gAnimMorphVelocityDir` (`0x010fa320`), `AnimProcessor::sUserTransformTranslation` (`0x010fa330`), `AnimProcessor::sUserTransformRotation` (`0x010fa340`), `_segValues` (`0x00f1658c`), `_segValuesAnim` (`0x010fe8a0`), `_segValuesStatic` (`0x010fe8c0`), `AnimProcessor::sNativePattern` (`0x00f15b40`), `allBones` (`0x00f013fc`); complete definitions for `AnimFragment`, `Anim`, `AnimBlend`, `AnimSequence`, `AnimTweaker`, `AnimController`, `AnimFragmentProcessor`, `AnimProcessor`, `G2AnimAlphaTable`, `G2AnimQuatInfo`, `G2AnimInterpStateBlock`, `G2AnimInterpInfo`, `G2AnimBitStream`, `Model`, `Segment`, `BoneSet` in `src/game/anim/anim.h`; `ObjectBase` (`0x08`) in `src/game/obtable.h`; `InterpoBlend` and `Player::interpoBlend` (`0xd08`) in `src/game/player/player.h`; `cdc::Vector::Len3` and `cdc::OrthonormalInverse3x3` in `src/cdc/runtime/cdcMath/Math.h`.

---

### AnimTarget (`src/game/anim/anim.h`, `src/game/anim/animtarget.cpp`)
| Function | Address (Retail / PDB) | Status | Source File |
|---|---|---|---|
| `cdc::Array::reserve` (`ArrayImpl::reserve`) | `0x00415e90` | [x] Done | `cdcSys/Array.h` |
| `AnimTarget::PickSequence` | `0x00613790` / `0x00450370` | [x] Done | `animtarget.cpp` |
| `AnimTarget::~AnimTarget` | `0x004779c0` / `0x00479340` | [x] Done | `animtarget.cpp` |
| `AnimTarget::AddSequence` | `0x00477a10` / `0x005e7d10` | [x] Done | `animtarget.cpp` |
- **Structs Added**: `cdc::Array<T>` (`0x0c`) in `src/cdc/cdcSys/Array.h`; `AnimTarget` (`0x10`) in `src/game/anim/anim.h`.

---

### Animation (`src/game/anim/anim.h`, `src/game/animation.cpp`)
| Function | Address (Retail / PDB) | Status | Source File |
|---|---|---|---|
| `cdc::Vector::operator*=` | `0x0041af40` / `0x00419f10` | [x] Done | `cdcMath/Math.h` |
| `cdc::Vector::IsZero3` | `0x005dcea0` / `0x005df8d0` | [x] Done | `cdcMath/Math.h` |
| `cdc::Euler::Euler(const Matrix&)` | `0x005dd2a0` / `0x005dfcd0` | [x] Done | `cdcMath/Math.h` |
| `cdc::Euler::SetZero` | `0x00419dd0` (PDB) | [x] Done | `cdcMath/Math.h` |
| `cdc::operator/(Vector, float)` | `0x0041a740` / `0x004196a0` | [x] Done | `cdcMath/Math.h` |
| `cdc::operator!=(Quat, Quat)` | `0x0041ae50` / `0x00419e20` | [x] Done | `cdcMath/Math.h` |
| `cdc::operator~(Quat)` | `0x0041aed0` / `0x00419ea0` | [x] Done | `cdcMath/Math.h` |
| `cdc::QuatLog` / `cdc::QuatExp` | `0x005de6b0` / `0x005e10e0`, `0x005de7c0` / `0x005e11f0` | [x] Done | `cdcMath/Math.h` |
| `cdc::Transpose` / `cdc::operator*(Matrix, Matrix)` | `0x005dd3e0` / `0x005dfe10`, `0x004125a0` | [x] Done | `cdcMath/Math.h` |
| `AnimFragment::GetTrans` | `0x0041a770` / `0x004196d0` | [x] Done | `animation.cpp` |
| `AnimFragment::GetRot` | `0x0041a7c0` / `0x00419720` | [x] Done | `animation.cpp` |
| `AnimProcessor::SetMirrorMode` | `0x0041a810` / `0x00419770` | [x] Done | `animation.cpp` |
| `AnimSequence::AnimSequence` | `0x0041bb90` / `0x0041ab60` | [x] Done | `animation.cpp` |
| `AnimSequence::operator<<` | `0x0041a830` / `0x00419790` | [x] Done | `animation.cpp` |
| `AnimSequence::GetAnim` | `0x0041a860` / `0x004197c0` | [x] Done | `animation.cpp` |
| `AnimSequence::Size` | `0x0041a880` / `0x004197e0` | [x] Done | `animation.cpp` |
| `AnimSequence::SolveTranslationRotation` | `0x0041bc30` / `0x0041ac00` | [x] Done | `animation.cpp` |
| `AnimSequenceList::AnimSequenceList` | `0x0041a890` / `0x004197f0` | [x] Done | `animation.cpp` |
| `AnimSequenceList::Add` | `0x0041a8a0` / `0x00419800` | [x] Done | `animation.cpp` |
| `AnimSequenceList::Size` | `0x00419810` (PDB) | [x] Done | `animation.cpp` |
| `ANIM_MinimumErrorSequence` | `0x00419a80` (PDB) | [x] Done | `animation.cpp` |
| `GetNextThreeBits` | `0x0041ab10` / `0x00419a90` | [x] Done | `animation.cpp` |
| `ANIM_GetRotationAtTime` | `0x0041ab70` / `0x00419af0` | [x] Done | `animation.cpp` |
| `ANIM_HasRootMotion` | `0x0041abc0` / `0x00419b40` | [x] Done | `animation.cpp` |
| `ANIM_AnimBlendFromIndex` | `0x0041acf0` / `0x00419c70` | [x] Done | `animation.cpp` |
| `ANIM_GetTranslationFromBlendId` | `0x0041ad20` / `0x00419ca0` | [x] Done | `animation.cpp` |
| `ANIM_GetRotationFromBlendId` | `0x0041adb0` / `0x00419d30` | [x] Done | `animation.cpp` |
| `ANIM_GetTranslationAtTime` | `0x0041b090` / `0x0041a060` | [x] Done | `animation.cpp` |
| `ANIM_GetTransformAtTime` | `0x0041b350` / `0x0041a320` | [x] Done | `animation.cpp` |
| `ANIM_GetAnimBlendTransformAtTime` | `0x0041b780` / `0x0041a750` | [x] Done | `animation.cpp` |
| `AnimComponent::AnimComponent` | `0x0041af70` / `0x00419f40` | [x] Done | `animation.cpp` |
| `AnimComponent::~AnimComponent` | `0x0041b020` / `0x00419ff0` | [x] Done | `animation.cpp` |
| `AnimComponent::PlayAnimation` | `0x0041a8b0` / `0x00419820` | [x] Done | `animation.cpp` |
| `AnimComponent::PlayHostedAnimation` | `0x0041a8e0` / `0x00419850` | [x] Done | `animation.cpp` |
| `AnimComponent::Stop` | `0x0041a910` / `0x00419880` | [x] Done | `animation.cpp` |
| `AnimComponent::IsAnimationPlaying` | `0x0041a940` / `0x004198b0` | [x] Done | `animation.cpp` |
| `BlendProcessor::UpdateStoredFrame` | `0x0041a9c0` / `0x00419930` | [x] Done | `animation.cpp` |
| `BlendProcessor::NextKeyframe` | `0x0041aa10` / `0x00419980` | [x] Done | `animation.cpp` |
| `BlendProcessor::NextKeyframeTimeDelta` | `0x0041aa90` / `0x00419a00` | [x] Done | `animation.cpp` |
| `BlendProcessor::ApplyRagdoll` | `0x0041c230` / `0x0041b200` | [x] Done | `animation.cpp` |
| `BlendProcessor::BuildTransforms` | `0x0041cad0` / `0x0041baa0` | [x] Done | `animation.cpp` |
- **Globals & Structs Added**: `AnimSequenceList` (`0x14`), `BlendProcessor` (`0x04`) in `src/game/anim/anim.h`; `Body`, `RagdollBone` (`0xc0`), `Ragdoll` (`0xb70`), `RagdollClientBone` (`0x20`), `RagdollClient` (`0x130`) in `src/game/ragdoll.h`; resized `_segValuesStatic` to 180 entries in `src/game/anim/animprocessor.cpp`.

---

### AnimationPatterns (`src/game/anim/anim.h`, `src/game/animationpatterns.cpp`)
| Function | Address (Retail / PDB) | Status | Source File |
|---|---|---|---|
| `AnimPattern::Process` | `0x0049a3a0` (PDB) | [x] Done | `anim.h` |
| `AnimPattern::Exit` | `0x0049a3a0` (PDB) | [x] Done | `anim.h` |
| `AnimPattern_RunAndJump::RunToEdge` | `0x0041cdd0` / `0x0041bda0` | [x] Done | `animationpatterns.cpp` |
| `AnimPattern_RunAndJump::JumpToEdge` | `0x0041cf60` / `0x0041bf30` | [x] Done | `animationpatterns.cpp` |
| `AnimPattern_RunAndJump::Entry` | `0x0041d020` / `0x0041bff0` | [x] Done | `animationpatterns.cpp` |
| `AnimPattern_RunToSlide::Entry` | `0x0041d0c0` / `0x0041c090` | [x] Done | `animationpatterns.cpp` |
| `AnimPattern_SlopeJumpToLedge::Entry` | `0x0041d2f0` / `0x0041c2c0` | [x] Done | `animationpatterns.cpp` |
| `AnimPattern_SlopeBackJumpToLedge::Entry` | `0x0041d470` / `0x0041c440` | [x] Done | `animationpatterns.cpp` |
- **Globals & Structs Added**: `AnimPattern_RunAndJump`, `AnimPattern_RunToSlide`, `AnimPattern_SlopeJumpToLedge`, `AnimPattern_SlopeBackJumpToLedge`, `gAnimPattern_RunAndJump` (`0x00f127e8`), `gAnimPattern_RunToSlide` (`0x00f127ec`), `gAnimPattern_SlopeJumpToLedge` (`0x00f127f0`), `gAnimPattern_SlopeBackJumpToLedge` (`0x00f127f4`); wired `AnimProcessor::sNativePattern` (`0x00f15b40`) in `src/game/anim/animprocessor.cpp`; `AnimFragmentTable` (`0x24`) and `OldRazielData::animFragmentTable` (`0xbac`) in `src/game/player/player.h`.

---

### G2 Subsystem (`src/game/g2/poolmmg2.h`, `poolmmg2.cpp`, `quatg2.h`, `quatg2.cpp`, `timerg2.h`, `timerg2.cpp`, `instncg2.h`, `instncg2.cpp`, `emulateg2.h`, `emulateg2.cpp`)
| Function | Address (Retail / PDB) | Status | Source File |
|---|---|---|---|
| `G2PoolMem_ResetPool` | `0x004dffd0` / `0x004e1b00` | [x] Done | `poolmmg2.cpp` |
| `G2PoolMem_Allocate` | `0x004e0000` / `0x004e1b30` | [x] Done | `poolmmg2.cpp` |
| `G2PoolMem_Free` | `0x004e0030` / `0x004e1b60` | [x] Done | `poolmmg2.cpp` |
| `G2PoolMem_InitPool` | `0x004e0060` / `0x004e1b90` | [x] Done | `poolmmg2.cpp` |
| `G2Quat_ToMatrixf` | `0x004e00f0` / `0x004e1c20` | [x] Done | `quatg2.cpp` |
| `G2EulerAngles_FromMatrixf` | `0x004e0120` / `0x004e1c50` | [x] Done | `quatg2.cpp` |
| `G2Quat_ToEuler` | `0x004e01e0` / `0x004e1d10` | [x] Done | `quatg2.cpp` |
| `G2Quat_FromEulerf` | `0x004e0220` / `0x004e1d50` | [x] Done | `quatg2.cpp` |
| `G2Quat_FromMatrixf` | `0x004e0280` / `0x004e1db0` | [x] Done | `quatg2.cpp` |
| `G2Quat_Slerp` | `0x004e02b0` / `0x004e1de0` | [x] Done | `quatg2.cpp` |
| `G2Timer_GetFrameTime` | `0x004e02e0` / `0x004e1e10` | [x] Done | `timerg2.cpp` |
| `G2EmulationInit` | `0x004de560` / `0x004e0090` | [x] Done | `emulateg2.cpp` |
| `G2EmulationInstanceToInstanceSwitchAnimation` | `0x004de670` / `0x004e01b0` | [x] Done | `emulateg2.cpp` |
| `G2EmulationInstanceSwitchAnimation` | `0x004de730` / `0x004e0270` | [x] Done | `emulateg2.cpp` |
| `G2EmulationInstanceToInstanceSwitchAnimationCharacter` | `0x004de7e0` / `0x004e0320` | [x] Done | `emulateg2.cpp` |
| `G2EmulationInstanceSetAnimation` | `0x004de8b0` / `0x004e03f0` | [x] Done | `emulateg2.cpp` |
| `G2EmulationInstanceSetAnimationLastFrame` | `0x004de960` / `0x004e04a0` | [x] Done | `emulateg2.cpp` |
| `G2EmulationSetAnimation` | `0x004de9d0` / `0x004e0510` | [x] Done | `emulateg2.cpp` |
| `G2EmulationInstanceSetMode` | `0x004dea10` / `0x004e0550` | [x] Done | `emulateg2.cpp` |
| `G2EmulationInstanceSetAnimSpeed` | `0x004dea60` / `0x004e05a0` | [x] Done | `emulateg2.cpp` |
| `G2EmulationSetControllerAlpha` | `0x004dea90` / `0x004e05d0` | [x] Done | `emulateg2.cpp` |
| `G2EmulationInstanceQueryAnimation` | `0x004dead0` / `0x004e0610` | [x] Done | `emulateg2.cpp` |
| `G2EmulationInstanceQueryAnimLength` | `0x004deaf0` / `0x004e0630` | [x] Done | `emulateg2.cpp` |
| `G2EmulationInstanceEventAnimPlaying` | `0x004deb30` / `0x004e0670` | [x] Done | `emulateg2.cpp` |
| `G2EmulationInstanceSetEventAnimPlaying` | `0x004deb60` / `0x004e06a0` | [x] Done | `emulateg2.cpp` |
| `G2EmulationInstanceQueryMode` | `0x004deb90` / `0x004e06d0` | [x] Done | `emulateg2.cpp` |
| `G2EmulationInstanceSetStartAndEndSegment` | `0x004debd0` / `0x004e0710` | [x] Done | `emulateg2.cpp` |
| `G2EmulationInstanceSetTotalSections` | `0x004dec00` / `0x004e0740` | [x] Done | `emulateg2.cpp` |
| `G2EmulationInstanceInitSection` | `0x004dec50` / `0x004e0790` | [x] Done | `emulateg2.cpp` |
| `G2EmulationSetInterpController_Vector` | `0x004dec80` / `0x004e07c0` | [x] Done | `emulateg2.cpp` |
| `G2EmulationSetMode` | `0x004decd0` / `0x004e0810` | [x] Done | `emulateg2.cpp` |
| `G2EmulationQueryAnimation` | `0x004ded20` / `0x004e0860` | [x] Done | `emulateg2.cpp` |
| `G2EmulationSetStartAndEndSegment` | `0x004ded50` / `0x004e0890` | [x] Done | `emulateg2.cpp` |
| `G2EmulationSetTotalSections` | `0x004ded90` / `0x004e08d0` | [x] Done | `emulateg2.cpp` |
| `G2EmulationQueryInstanceAnimation` | `0x004deda0` / `0x004e08e0` | [x] Done | `emulateg2.cpp` |
| `G2EmulationInstancePlayAnimation` | `0x004dedc0` / `0x004e0900` | [x] Done | `emulateg2.cpp` |
| `G2EmulatePlayAnimation` | `0x004dede0` / `0x004e0920` | [x] Done | `emulateg2.cpp` |
| `G2EmulationInstanceQueryFrame` | `0x004dee00` / `0x004e0940` | [x] Done | `emulateg2.cpp` |
| `G2EmulationQueryFrame` | `0x004dee30` / `0x004e0970` | [x] Done | `emulateg2.cpp` |
| `G2Instance_Reset` | `0x004702f0` | [x] Done | `instncg2.cpp` |
| `G2Instance_GetKeylist` | `0x004dee70` / `0x004e09a0` | [x] Done | `instncg2.cpp` |
| `G2Instance_GetKeylistFromObject` | `0x004deea0` / `0x004e09d0` | [x] Done | `instncg2.cpp` |
| `G2Instance_BuildClothTransforms` | `0x004deed0` / `0x004e0a00` | [x] Done | `instncg2.cpp` |
| `G2Instance_BuildVirtualTransforms` | `0x004deef0` / `0x004e0a20` | [x] Done | `instncg2.cpp` |
| `G2Instance_NextKeyframe` | `0x004defd0` / `0x004e0b00` | [x] Done | `instncg2.cpp` |
| `G2Instance_GetNumSections` | `0x004df020` / `0x004e0b50` | [x] Done | `instncg2.cpp` |
| `_G2Instance_RebuildAnimatedTransforms` | `0x004df060` / `0x004e0b90` | [x] Done | `instncg2.cpp` |
| `G2Instance_SetTransformsToIdentity` | `0x004df4b0` / `0x004e0fe0` | [x] Done | `instncg2.cpp` |
| `_G2Instance_BuildFacadeTransforms` | `0x004df760` / `0x004e1290` | [x] Done | `instncg2.cpp` |
| `_G2Instance_RebuildNonAnimatedTransforms` | `0x004dfa40` / `0x004e1570` | [x] Done | `instncg2.cpp` |
| `G2Instance_BuildTransforms` | `0x004dfc60` / `0x004e1790` | [x] Done | `instncg2.cpp` |
| `G2Instance_RebuildTransforms` | `0x004dff40` / `0x004e1a70` | [x] Done | `instncg2.cpp` |
| `G2Instance_BuildTransformsForList` | `0x004dff60` / `0x004e1a90` | [x] Done | `instncg2.cpp` |
- **Structs, Globals & Math Helpers Added**: `G2PoolMemPool` (`0x10`) in `src/game/g2/poolmmg2.h`; unified `_G2EulerAngles_Type` (`0x10`) in `src/game/g2/quatg2.h` across `anim.h`, `animprocessor.cpp`, `cameracinematic.cpp`, and `math3d.cpp`; added `cdc::Quat::Quat(const cdc::Euler&)` (`0x005de3d0`) and `cdc::Matrix::RotX`/`RotY`/`RotZ` (`0x005dd8d0`/`0x005dd980`/`0x005dda20`) in `src/cdc/runtime/cdcMath/Math.h`; `G2AlphaTables[8]` (`0x010fe7c0`), `VirtSegment` (`0x40`) in `src/game/anim/anim.h`; `int16_t` `Object::sectionA/B/C` (`src/game/obtable.h`), `int16_t` `Instance::ParentLinkNode/linkMarkerIndex` (`src/game/instance.h`), `Player::currentRope` (`0xf5c`, `src/game/player/player.h`), and `GameTracker::activeCinematic` (`0x42c`, `src/game/main.h`).

---

### AnimationSection (`src/game/resolve/resolvesection.h`, `src/game/resolve/animationsection.h`, `src/game/resolve/animationsection.cpp`)
| Function | Address (Retail / PDB) | Status | Source File |
|---|---|---|---|
| `AnimationSection::StartResource` | `0x005b97a0` / `0x005bc110` | [x] Done | `animationsection.cpp` |
| `AnimationSection::HandleResourceData` | `0x005b97d0` / `0x005bc140` | [x] Done | `animationsection.cpp` |
| `AnimationSection::ReleaseResource` | `0x005b9800` / `0x005bc170` | [x] Done | `animationsection.cpp` |
| `AnimationSection::HandleResourceCancel` | `0x005b9800` / `0x005bc170` | [x] Done | `animationsection.cpp` |
| `AnimationSection::HandleResourceEnd` | `0x004693a0` / `0x0055d060` | [x] Done | `animationsection.cpp` |
| `AnimationSection::GetBasePointer` | `0x005b9810` / `0x005bc180` | [x] Done | `animationsection.cpp` |
| `AnimationSection::GetSize` | `0x005b9820` / `0x005bc190` | [x] Done | `animationsection.cpp` |
| `AnimationSection::GetRefCount` | `0x005b9840` / `0x005bc1b0` | [x] Done | `animationsection.cpp` |
| `AnimationSection::FindResourceA` | `0x005b9850` / `0x005bc1c0` | [x] Done | `animationsection.cpp` |
- **Structs Added**: `SectionType`, `SectionInfo` (`0x14`), `ResolveSection` (`0x04`) in `src/game/resolve/resolvesection.h`; `AnimationSection` (`0x04`, vtable `0x00f0614c`) in `src/game/resolve/animationsection.h`.

---

## Next Targets in Execution Queue
1. **Collision / Physics Subsystem (`src/game/collide/`)**:
   - Instance physical query and collision resolution
