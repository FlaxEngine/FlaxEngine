// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_MAC || PLATFORM_IOS

#include "../Unix/UnixPlatform.h"

/// <summary>
/// The Apple platform implementation and application management utilities.
/// </summary>
class FLAXENGINE_API ApplePlatform : public UnixPlatform
{
public:
    static float ScreenScale;

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
        return __atomic_load_n(dst, __ATOMIC_RELAXED);
    }
    FORCE_INLINE static int64 AtomicRead(int64 const volatile* dst)
    {
        return __atomic_load_n(dst, __ATOMIC_RELAXED);
    }
    FORCE_INLINE static void AtomicStore(int32 volatile* dst, int32 value)
    {
        __atomic_store_n((volatile int32*)dst, value, __ATOMIC_RELAXED);
    }
    FORCE_INLINE static void AtomicStore(int64 volatile* dst, int64 value)
    {
        __atomic_store_n((volatile int64*)dst, value, __ATOMIC_RELAXED);
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
    static uint64 GetCurrentThreadID();
    static void SetThreadPriority(ThreadPriority priority);
    static void SetThreadAffinityMask(uint64 affinityMask);
    static void Sleep(int32 milliseconds);
    static double GetTimeSeconds();
    static uint64 GetTimeCycles();
    static uint64 GetClockFrequency();
    static void GetSystemTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond);
    static void GetUTCTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond);
#if !BUILD_RELEASE
    static void Log(const StringView& msg);
    static bool IsDebuggerPresent();
#endif
    static bool Init();
    static void Tick();
    static void BeforeExit();
    static void Exit();
    static void SetHighDpiAwarenessEnabled(bool enable);
    static String GetUserLocaleName();
    static bool GetHasFocus();
    static void CreateGuid(Guid& result);
	static Float2 GetDesktopSize();
	static String GetMainDirectory();
	static String GetExecutableFilePath();
    static String GetWorkingDirectory();
	static bool SetWorkingDirectory(const String& path);
    static bool GetEnvironmentVariable(const String& name, String& value);
    static bool SetEnvironmentVariable(const String& name, const String& value);
    static void* LoadLibrary(const Char* filename);
    static void FreeLibrary(void* handle);
    static void* GetProcAddress(void* handle, const char* symbol);
    static Array<StackFrame, HeapAllocation> GetStackFrames(int32 skipCount = 0, int32 maxDepth = 60, void* context = nullptr);
};

#endif
