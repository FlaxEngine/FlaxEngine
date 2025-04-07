// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_ANDROID

#include "Engine/Platform/Unix/UnixPlatform.h"
#include <pthread.h>

struct android_app;

/// <summary>
/// The Android platform implementation and application management utilities.
/// </summary>
class FLAXENGINE_API AndroidPlatform : public UnixPlatform
{
public:

    static android_app* GetApp();
    static String GetAppPackageName();
    static String GetDeviceManufacturer();
    static String GetDeviceModel();
    static String GetDeviceBuildNumber();
    static void PreInit(android_app* app);

public:

    // [UnixPlatform]
    FORCE_INLINE static void MemoryBarrier()
    {
        __sync_synchronize();
    }
    FORCE_INLINE static int64 InterlockedExchange(int64 volatile* dst, int64 exchange)
    {
        return __sync_lock_test_and_set(dst, exchange);
    }
    FORCE_INLINE static int32 InterlockedCompareExchange(int32 volatile* dst, int32 exchange, int32 comperand)
    {
        return __sync_val_compare_and_swap(dst, comperand, exchange);
    }
    FORCE_INLINE static int64 InterlockedCompareExchange(int64 volatile* dst, int64 exchange, int64 comperand)
    {
        return __sync_val_compare_and_swap(dst, comperand, exchange);
    }
    FORCE_INLINE static int64 InterlockedIncrement(int64 volatile* dst)
    {
        return __sync_add_and_fetch(dst, 1);
    }
    FORCE_INLINE static int64 InterlockedDecrement(int64 volatile* dst)
    {
        return __sync_sub_and_fetch(dst, 1);
    }
    FORCE_INLINE static int64 InterlockedAdd(int64 volatile* dst, int64 value)
    {
        return __sync_fetch_and_add(dst, value);
    }
    FORCE_INLINE static int32 AtomicRead(int32 const volatile* dst)
    {
        int32 result;
        __atomic_load(dst, &result, __ATOMIC_RELAXED);
        return result;
    }
    FORCE_INLINE static int64 AtomicRead(int64 const volatile* dst)
    {
        int64 result;
        __atomic_load(dst, &result, __ATOMIC_RELAXED);
        return result;
    }
    FORCE_INLINE static void AtomicStore(int32 volatile* dst, int32 value)
    {
        __atomic_store(dst, &value, __ATOMIC_RELAXED);
    }
    FORCE_INLINE static void AtomicStore(int64 volatile* dst, int64 value)
    {
        __atomic_store(dst, &value, __ATOMIC_RELAXED);
    }
    FORCE_INLINE static void Prefetch(void const* ptr)
    {
        __builtin_prefetch(static_cast<char const*>(ptr));
    }
    static bool Is64BitPlatform();
    static String GetSystemName();
    static Version GetSystemVersion();
    static CPUInfo GetCPUInfo();
    static MemoryStats GetMemoryStats();
    static ProcessMemoryStats GetProcessMemoryStats();
    static uint64 GetCurrentThreadID()
    {
        return static_cast<uint64>(pthread_self());
    }
    static void SetThreadPriority(ThreadPriority priority);
    static void SetThreadAffinityMask(uint64 affinityMask);
    static void Sleep(int32 milliseconds);
    static double GetTimeSeconds();
    static uint64 GetTimeCycles();
    FORCE_INLINE static uint64 GetClockFrequency()
    {
        // Dummy value
        return 1000000;
    }
    static void GetSystemTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond);
    static void GetUTCTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond);
    static bool Init();
    static void LogInfo();
    static void BeforeRun();
    static void Tick();
    static void BeforeExit();
    static void Exit();
#if !BUILD_RELEASE
    static void Log(const StringView& msg);
#endif
    static int32 GetDpi();
    static NetworkConnectionType GetNetworkConnectionType();
    static ScreenOrientationType GetScreenOrientationType();
    static String GetUserLocaleName();
    static String GetComputerName();
    static bool GetHasFocus();
    static bool GetIsPaused();
    static bool CanOpenUrl(const StringView& url);
    static void OpenUrl(const StringView& url);
    static Float2 GetMousePosition();
    static void SetMousePosition(const Float2& pos);
    static Float2 GetDesktopSize();
    static String GetMainDirectory();
    static String GetExecutableFilePath();
    static Guid GetUniqueDeviceId();
    static String GetWorkingDirectory();
    static bool SetWorkingDirectory(const String& path);
    static Window* CreateWindow(const CreateWindowSettings& settings);
    static bool GetEnvironmentVariable(const String& name, String& value);
    static bool SetEnvironmentVariable(const String& name, const String& value);
    static void* LoadLibrary(const Char* filename);
    static void FreeLibrary(void* handle);
    static void* GetProcAddress(void* handle, const char* symbol);
    static Array<StackFrame, HeapAllocation> GetStackFrames(int32 skipCount = 0, int32 maxDepth = 60, void* context = nullptr);
};

#endif
