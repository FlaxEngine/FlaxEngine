// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WIN32

// The following macros define the minimum required platform - Windows 7
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT WINVER
#endif
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS WINVER
#endif

// Override for Xbox
#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#endif

// Disable annoying warnings
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 1

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI
#define NODRAWTEXT
#define NOCTLMGR
#define NOFLATSBAPIS

// Include Windows headers
#include <Windows.h>

// Remove some Windows definitions
#undef MemoryBarrier
#undef DeleteFile
#undef MoveFile
#undef CopyFile
#undef CreateDirectory
#undef GetComputerName
#undef GetUserName
#undef MessageBox
#undef GetCommandLine
#undef CreateWindow
#undef CreateProcess
#undef SetWindowText
#undef DrawText
#undef CreateFont
#undef IsMinimized
#undef IsMaximized
#undef LoadIcon
#undef InterlockedOr
#undef InterlockedAnd
#undef InterlockedExchange
#undef InterlockedCompareExchange
#undef InterlockedIncrement
#undef InterlockedDecrement
#undef InterlockedAdd
#undef GetObject
#undef GetClassName
#undef GetMessage
#undef CreateMutex
#undef DrawState
#undef LoadLibrary
#undef GetEnvironmentVariable
#undef SetEnvironmentVariable

#define WINDOWS_GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define WINDOWS_GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

// Releases a COM object and nullifies pointer
template<typename T>
inline void SafeRelease(T** currentObject)
{
    if (*currentObject != nullptr)
    {
        (*currentObject)->Release();
        *currentObject = nullptr;
    }
}

// Acquires an additional reference, if non-null
template<typename T>
inline T* SafeAcquire(T* newObject)
{
    if (newObject != nullptr)
        newObject->AddRef();
    return newObject;
}

// Sets a new COM object, releasing the old one
template<typename T>
inline void SafeSet(T** currentObject, T* newObject)
{
    SafeAcquire(newObject);
    SafeRelease(currentObject);
    *currentObject = newObject;
}

// Releases a COM object and nullifies pointer
template<typename T>
inline T* SafeDetach(T** currentObject)
{
    T* oldObject = *currentObject;
    *currentObject = nullptr;
    return oldObject;
}

// Sets a new COM object, acquiring the reference
template<typename T>
inline void SafeAttach(T** currentObject, T* newObject)
{
    SafeRelease(currentObject);
    *currentObject = newObject;
}

#define LOG_WIN32_LAST_ERROR LOG(Warning, "Win32::GetLastError() = 0x{0:x}", GetLastError())

#endif
