// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WEB

#include "../Unix/UnixPlatform.h"
#ifdef __EMSCRIPTEN_PTHREADS__
#include <pthread.h>
#endif

// Platform memory profiler categories mapping
#define WEB_MEM_TAG_HEAP_SIZE CustomPlatform0
#define WEB_MEM_TAG_HEAP_MAX CustomPlatform1

/// <summary>
/// The Web platform implementation and application management utilities.
/// </summary>
class FLAXENGINE_API WebPlatform : public UnixPlatform
{
public:
    // [UnixPlatform]
    FORCE_INLINE static void MemoryBarrier()
    {
#ifdef __EMSCRIPTEN_PTHREADS__
        // Fake a fence with an arbitrary atomic operation (from emscripten_atomic_fence to avoid including it for less header bloat)
        uint8 temp = 0;
        __c11_atomic_fetch_or((_Atomic uint8*)&temp, 0, __ATOMIC_SEQ_CST);
#endif
    }
    FORCE_INLINE static void MemoryPrefetch(void const* ptr)
    {
        __builtin_prefetch(static_cast<char const*>(ptr));
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
#ifdef __EMSCRIPTEN_PTHREADS__
        int32 result;
        __atomic_load(dst, &result, __ATOMIC_SEQ_CST);
        return result;
#else
        return *dst;
#endif
    }
    FORCE_INLINE static int64 AtomicRead(int64 const volatile* dst)
    {
#ifdef __EMSCRIPTEN_PTHREADS__
        int64 result;
        __atomic_load(dst, &result, __ATOMIC_SEQ_CST);
        return result;
#else
        return *dst;
#endif
    }
    FORCE_INLINE static void AtomicStore(int32 volatile* dst, int32 value)
    {
#ifdef __EMSCRIPTEN_PTHREADS__
        __atomic_store(dst, &value, __ATOMIC_SEQ_CST);
#else
        *dst = value;
#endif
    }
    FORCE_INLINE static void AtomicStore(int64 volatile* dst, int64 value)
    {
#ifdef __EMSCRIPTEN_PTHREADS__
        __atomic_store(dst, &value, __ATOMIC_SEQ_CST);
#else
        *dst = value;
#endif
    }
#if 0
    static void* Allocate(uint64 size, uint64 alignment);
    static void Free(void* ptr);
#endif
    FORCE_INLINE static uint64 GetCurrentThreadID()
    {
#ifdef __EMSCRIPTEN_PTHREADS__
        return (uint64)pthread_self();
#else
        return 1;
#endif
    }
    static String GetSystemName();
    static Version GetSystemVersion();
    static CPUInfo GetCPUInfo();
    static MemoryStats GetMemoryStats();
    static ProcessMemoryStats GetProcessMemoryStats();
    static void SetThreadPriority(ThreadPriority priority);
    static void SetThreadAffinityMask(uint64 affinityMask);
    static void Sleep(int32 milliseconds);
    static void Yield();
    static double GetTimeSeconds();
    static uint64 GetTimeCycles();
    static void GetSystemTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond);
    static void GetUTCTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond);
    static void Log(const StringView& msg, int32 logType = 1);
#if !BUILD_RELEASE
    static bool IsDebuggerPresent()
    {
        return false;
    }
#endif
    static String GetComputerName();
    static bool GetHasFocus();
    static String GetMainDirectory();
    static String GetExecutableFilePath();
    static Guid GetUniqueDeviceId();
    static String GetWorkingDirectory();
    static bool SetWorkingDirectory(const String& path);
    static bool Init();
    static void LogInfo();
    static void Tick();
    static void Exit();
    static void GetEnvironmentVariables(Dictionary<String, String, HeapAllocation>& result);
    static bool GetEnvironmentVariable(const String& name, String& value);
    static bool SetEnvironmentVariable(const String& name, const String& value);
    static void* LoadLibrary(const Char* filename);
    static void FreeLibrary(void* handle);
    static void* GetProcAddress(void* handle, const char* symbol);
    static Array<StackFrame, HeapAllocation> GetStackFrames(int32 skipCount = 0, int32 maxDepth = 60, void* context = nullptr);
};

#endif
