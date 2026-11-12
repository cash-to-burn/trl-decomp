#pragma once

#ifndef COMMON_H
#define COMMON_H

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cstdint>
typedef void* HANDLE;
typedef void* HWND;
typedef void* HINSTANCE;
typedef void* HKEY;
typedef void* HICON;
typedef void* HCURSOR;
typedef void* HBRUSH;
typedef void* HMENU;
typedef void* HDC;
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef unsigned long long DWORD_PTR;
typedef int BOOL;
typedef long LONG;
typedef intptr_t LRESULT;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;
#define CALLBACK __stdcall
#define WINAPI __stdcall
#endif

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Common game constants
#define TRL_MUTEX_NAME "TRLMUTEX"
#define TRL_WINDOW_CLASS "TOMBRAIDER"
#define TRL_WINDOW_TITLE "Tomb Raider: Legend"

#endif // COMMON_H
