# Tomb Raider: Legend (PC) Decompilation Progress

## Target Binary Information
- **Target Executable**: `trl.exe` (28,102,656 bytes, PE32)
- **Debug Symbols**: `tr7.pdb` (10,202,112 bytes, Microsoft C/C++ MSF v7.00 PDB)
- **Original Source Root**: `f:\tr7\`
- **Architecture**: x86 (32-bit Windows), MSVC 2003 (VC7.1)
- **Symbol Offset Note**: Retail `trl.exe` `.text` layout differs from internal `tr7.pdb` by an offset shift of `-0x4f0` in `mainvm.c` (`0x00402150` in PDB maps to `0x00401c60` in executable).

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
    ├── DuplicateHandle(hCurProcess, hCurThread, ..., &hMainThread)
    ├── SetThreadAffinityMask(GetCurrentThread(), 1) [Lock to CPU Core 0]
    │
    ├── Setup_LoadRegistrySettings() [0x00404a40]
    │   └── Reads "SOFTWARE\Crystal Dynamics\Tomb Raider: Legend" (Language, Audio, Gamepad)
    │
    ├── Setup_LoadUserPreferences() [0x004044c0]
    │
    ├── Command-Line Parsing:
    │   ├── "-setup"  -> forceOptions = 1
    │   └── "-config" -> forceOptions = 2
    │
    ├── TIMER_Init() [0x00402570 / PDB 0x00402af0]
    │   └── WIN_TimeUS() [0x00402520] (QueryPerformanceFrequency & Counter)
    │
    ├── CreateMutexA(NULL, FALSE, "TRLMUTEX") [0x00f10424]
    │   └── [If ERROR_ALREADY_EXISTS]:
    │       ├── EnumWindows(FindGameEnumWindowsProc, "TOMBRAIDER") [0x00401a10 / PDB 0x00401f00]
    │       │   └── Bring existing game window to foreground
    │       └── MessageBoxUTF8() [0x00401950 / PDB 0x00401940] (If window not found)
    │
    ├── OS Version Verification:
    │   └── GetVersionExA(&verInfo) (Requires Windows 2000 / XP, NT 5.0+)
    │
    ├── SETUP_DoSetupDialog(1, forceDialog) [0x004056c0]
    │
    └── Game Execution Loop:
        ├── SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, FALSE) [Disable screensaver]
        ├── GAMEWINDOW_Init() [0x00403fd0]
        │   ├── RegisterClassA("TOMBRAIDER", RegularWndProc) [0x00403c00]
        │   └── CreateWindowExA(WS_EX_TOPMOST, "TOMBRAIDER", "Tomb Raider: Legend", ...)
        │
        ├── D3D_InitDevice() [0x004139a0]
        │   └── SetThreadPriority(THREAD_PRIORITY_NORMAL) & SetPriorityClass(NORMAL_PRIORITY_CLASS)
        │
        ├── GAMELOOP_Execute(0) [0x0045cc10] (Core Gameplay & Engine Loop)
        │
        └── Shutdown & Cleanup:
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
1. **Direct3D Device Initialization & Shutdown**:
   - `D3D_InitManager` (`0x00413880`)
   - `D3D_InitDevice` (`0x004139a0`)
   - `D3D_ShutdownDevice` (`0x00413930`)
   - `src/game/pc/D3D/rnd_d3d.cpp`
2. **Bigfile / File System Initialization**:
   - `FileSystem_Init` (`0x0045c2a0`)
   - `FileSystem_InitBigfile` (`0x0045c300`)
   - `FileSystem_Shutdown` (`0x0045c360`)
   - `Archive_Open` (`0x0047a990`)
3. **PCLOCALS String Manager**:
   - `PCLOCALS_GetFixedIDLocal` (`0x004e4050`)
   - `src/game/pc/pclocals.cpp`
4. **GAMELOOP Subsystem Breakdown**:
   - `GAMELOOP_Execute` (`0x0045cc10`) state machine dispatch
