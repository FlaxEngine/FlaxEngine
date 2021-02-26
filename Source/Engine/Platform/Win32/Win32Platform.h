// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WIN32

#include "Engine/Platform/Base/PlatformBase.h"

/// <summary>
/// The Win32 platform implementation and application management utilities.
/// </summary>
class FLAXENGINE_API Win32Platform : public PlatformBase
{
public:

    // [PlatformBase]
    static bool Init();
    static void Exit();
    static void MemoryBarrier();
    static int64 InterlockedExchange(int64 volatile* dst, int64 exchange);
    static int32 InterlockedCompareExchange(int32 volatile* dst, int32 exchange, int32 comperand);
    static int64 InterlockedCompareExchange(int64 volatile* dst, int64 exchange, int64 comperand);
    static int64 InterlockedIncrement(int64 volatile* dst);
    static int64 InterlockedDecrement(int64 volatile* dst);
    static int64 InterlockedAdd(int64 volatile* dst, int64 value);
    static int32 AtomicRead(int32 volatile* dst);
    static int64 AtomicRead(int64 volatile* dst);
    static void AtomicStore(int32 volatile* dst, int32 value);
    static void AtomicStore(int64 volatile* dst, int64 value);
    static void Prefetch(void const* ptr);
    static void* Allocate(uint64 size, uint64 alignment);
    static void Free(void* ptr);
    static void* AllocatePages(uint64 numPages, uint64 pageSize);
    static void FreePages(void* ptr);
    static bool Is64BitPlatform();
    static CPUInfo GetCPUInfo();
    static int32 GetCacheLineSize();
    static MemoryStats GetMemoryStats();
    static ProcessMemoryStats GetProcessMemoryStats();
    static uint64 GetCurrentProcessId();
    static uint64 GetCurrentThreadID();
    static void SetThreadPriority(ThreadPriority priority);
    static void SetThreadAffinityMask(uint64 affinityMask);
    static void Sleep(int32 milliseconds);
    static double GetTimeSeconds();
    static uint64 GetTimeCycles();
    static uint64 GetClockFrequency();
    static void GetSystemTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond);
    static void GetUTCTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond);
    static void CreateGuid(Guid& result);
    static String GetMainDirectory();
    static String GetExecutableFilePath();
    static struct Guid GetUniqueDeviceId();
    static String GetWorkingDirectory();
    static bool SetWorkingDirectory(const String& path);
    static void FreeLibrary(void* handle);
    static void* GetProcAddress(void* handle, const char* symbol);
};

#endif
