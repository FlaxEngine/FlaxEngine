// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_WIN32

#include "Win32Thread.h"
#include "Engine/Core/Log.h"
#include "Engine/Threading/IRunnable.h"
#include "Engine/Threading/ThreadRegistry.h"
#include "IncludeWindowsHeaders.h"

#if PLATFORM_WINDOWS
#define WINDOWS_ENABLE_THREAD_NAMING 1
#endif

#if WINDOWS_ENABLE_THREAD_NAMING

// Source: https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void SetThreadName(DWORD dwThreadID, const char* threadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
#pragma warning(pop)
}

#endif

Win32Thread::Win32Thread(IRunnable* runnable, const String& name, ThreadPriority priority)
    : ThreadBase(runnable, name, priority)
    , _thread(nullptr)
{
}

Win32Thread::~Win32Thread()
{
    ASSERT(_thread == nullptr);
}

Win32Thread* Win32Thread::Create(IRunnable* runnable, const String& name, ThreadPriority priority, uint32 stackSize)
{
    auto result = New<Win32Thread>(runnable, name, priority);
    if (result->Start(stackSize))
    {
        SAFE_DELETE(result);
    }

    return result;
}

bool Win32Thread::Start(uint32 stackSize)
{
    // Spawn thread
    DWORD id;
    _thread = (void*)CreateThread(nullptr, stackSize, (LPTHREAD_START_ROUTINE)ThreadProc, this, STACK_SIZE_PARAM_IS_A_RESERVATION, &id);
    if (_thread == nullptr)
        return true;

    // Initialize priority
    SetPriorityInternal(_priority);

#if WINDOWS_ENABLE_THREAD_NAMING
    SetThreadName(id, _name.ToStringAnsi().Get());
#endif

    return false;
}

unsigned long Win32Thread::ThreadProc(void* pThis)
{
    auto thread = (Win32Thread*)pThis;
#if PLATFORM_WINDOWS
    __try
#endif
    {
        const int32 exitCode = thread->Run();
        return static_cast<unsigned long>(exitCode);
    }
#if PLATFORM_WINDOWS
    __except (Platform::SehExceptionHandler(GetExceptionInformation()))
    {
        return -1;
    }
#endif
}

void Win32Thread::Join()
{
    WaitForSingleObject((HANDLE)_thread, INFINITE);
    ClearHandleInternal();
}

void Win32Thread::ClearHandleInternal()
{
    _thread = nullptr;
}

void Win32Thread::SetPriorityInternal(ThreadPriority priority)
{
    int32 winPriority;
    switch (priority)
    {
    case ThreadPriority::Lowest:
        winPriority = THREAD_PRIORITY_LOWEST;
        break;
    case ThreadPriority::BelowNormal:
        winPriority = THREAD_PRIORITY_BELOW_NORMAL;
        break;
    case ThreadPriority::Normal:
        winPriority = THREAD_PRIORITY_NORMAL;
        break;
    case ThreadPriority::AboveNormal:
        winPriority = THREAD_PRIORITY_ABOVE_NORMAL;
        break;
    case ThreadPriority::Highest:
        winPriority = THREAD_PRIORITY_HIGHEST;
        break;
    default:
        winPriority = THREAD_PRIORITY_NORMAL;
        break;
    }
    ::SetThreadPriority((HANDLE)_thread, winPriority);
}

void Win32Thread::KillInternal(bool waitForJoin)
{
    if (waitForJoin)
    {
        WaitForSingleObject((HANDLE)_thread, INFINITE);
    }

    CloseHandle((HANDLE)_thread);
}

#endif
