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
| `0x0048ca00` | `CAMERA_CircleAnglef` | `float CAMERA_CircleAnglef(float)` | Decompiled & Verified |
| `0x0048ca90` | `CAMERA_AngleDifference` | `float CAMERA_AngleDifference(float, float)` | Decompiled & Verified |

---

## Build Verification
- Cross-compiler target: `i686-w64-mingw32-g++` (32-bit Windows PE)
- CMake library target: `libtrl_decomp_core.a` (clean compile and link)

---

## Next Targets in Execution Queue
1. **Camera Behind & Look Processors (`camerabehind.c`, `cameralook.c`)**:
   - `CAMERA_FollowGoBehindPlayer` (`0x00482b20` / retail `0x00481180`)
   - `CAMERA_LookProcess` (`0x00488a10` / retail `0x00487070`)
2. **Animation Subsystem (`src/game/anim/` / `f:\tr7\game\anim\`)**:
   - Animation blending and character motion systems
3. **Collision / Physics Subsystem (`src/game/collide/`)**:
   - Instance physical query and collision resolution
