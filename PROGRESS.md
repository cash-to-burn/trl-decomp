# Tomb Raider: Legend (PC) Decompilation Progress

## Target Binary Information
- **Target Executable**: `trl.exe` (28,102,656 bytes, PE32)
- **Debug Symbols**: `tr7.pdb` (10,202,112 bytes, Microsoft C/C++ MSF v7.00 PDB)
- **Original Source Root**: `f:\tr7\`
- **Architecture**: x86 (32-bit Windows), MSVC 2003 (VC7.1)
- **Symbol Offset Note**:
  - `mainvm.c`: Retail `.text` offset shift of `-0x4f0` relative to PDB (`0x00402150` in PDB maps to `0x00401c60` in executable).
  - `main.c` / `ArchiveFileSystem`: Retail `.text` offset shift of `-0x1790` relative to PDB (`0x0045e3a0` in PDB maps to `0x0045cc10` in executable).

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
    ├── D3D_InitManager() [0x00413880] (Loads d3d9.dll & initializes PCDeviceManager)
    │
    ├── OS Version Verification (Requires Windows 2000 / XP, NT 5.0+)
    │
    ├── SETUP_DoSetupDialog(1, showDialog) [0x004056c0]
    │
    └── Main Execution Cycle:
        ├── SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, FALSE) [Disable screensaver]
        ├── GAMEWINDOW_Init() [0x00403fd0]
        │   ├── RegisterClassA("TOMBRAIDER", RegularWndProc) [0x00403c00]
        │   └── CreateWindowExA(WS_EX_TOPMOST, "TOMBRAIDER", "Tomb Raider: Legend", ...)
        │
        ├── D3D_InitDevice(false) [0x004139a0]
        │   └── SetThreadPriority(THREAD_PRIORITY_NORMAL) & SetPriorityClass(NORMAL_PRIORITY_CLASS)
        │
        ├── MainG2(0) [0x0045cc10 / PDB 0x0045e3a0] (Core G2 Gameplay & Engine Loop)
        │   ├── ProcessArgs(baseAreaName, gameTracker, appData) [0x0045c530]
        │   ├── MAIN_DoMainInit() [0x0045c940] (Loads shaders.drm, sound banks, UI)
        │   └── MainG2_UpdateLoop() [0x00401ab0] (Engine frame tick & message dispatch)
        │
        └── Shutdown & Cleanup:
            ├── RND_Shutdown() [0x00413a20] (Flush caches)
            ├── ReleaseFS() [0x0045c360 / PDB 0x0045da10] (Release Bigfile & FileSystem)
            ├── D3D_ShutdownDevice() [0x00413930]
            ├── GAMEWINDOW_Deinit() [0x004040c0]
            ├── SETUP_SaveRegistrySettings() [0x00404e00]
            ├── SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, oldScreensaver) [Restore screensaver]
            └── CloseHandle(hMutexAppIsRunning)
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

### `src/game/main.cpp` & `src/game/main.h` (Original: `f:\tr7\game\main.c`)
| Address (VA) | PDB Symbol | Signature | Status |
|---|---|---|---|
| `0x0045cc10` | `?MainG2@@YAHPAX@Z` | `int MainG2(void *appData)` | Decompiled & Verified |
| `0x0045c940` | `?MAIN_DoMainInit@@YAXXZ` | `void MAIN_DoMainInit(void)` | Decompiled & Verified |
| `0x0045c530` | `?ProcessArgs@@YAHPADPAUGameTracker@@PAX@Z` | `int ProcessArgs(char*, void*, void*)` | Decompiled & Verified |

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

### `src/cdc/cdcSys/UTF8String.cpp` & `src/cdc/cdcSys/UTF8String.h`
- String helper type `cdc::UTF8String` (constructors, destructor, `ConvertToWCHAR`, memory management).

---

## Build Verification
- Cross-compiler target: `i686-w64-mingw32-g++` (32-bit Windows PE)
- CMake library target: `libtrl_decomp_core.a` (clean compile and link)

---

## Next Targets in Execution Queue
1. **`cdc::PCDeviceManager` Class Decompilation**:
   - `PCDeviceManager::PCDeviceManager` (`0x00ec6ff0`)
   - `PCDeviceManager::~PCDeviceManager` (`0x00ec6ea0`)
   - `PCDeviceManager::CreateInstance` (`0x00ec70f0`)
   - `PCDeviceManager::SetVideoConfig` (`0x00ec6350`)
   - `src/cdc/runtime/cdcRender/pc/shared/PCDeviceManager.cpp`
2. **`DTPDataSection` Resource Management**:
   - `DTPDataSection_Init` (`0x005bd910`)
   - `src/game/dtp.cpp`
3. **`MainG2` State Machine Breakdown**:
   - Mode dispatch (`GAMELOOP_ChangeMode`, `GAMELOOP_InitRunning`)
   - Level transition handler (`GAMELOOP_LevelLoadAndInit`)
