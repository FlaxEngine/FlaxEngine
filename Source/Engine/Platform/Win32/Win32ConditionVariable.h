// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WIN32

#include "Win32CriticalSection.h"
#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// Win32 implementation of a condition variables. Condition variables are synchronization primitives that enable threads to wait until a particular condition occurs. Condition variables enable threads to atomically release a lock and enter the sleeping state.
/// </summary>
class FLAXENGINE_API Win32ConditionVariable
{
private:

    Windows::CONDITION_VARIABLE _cond;

private:

    Win32ConditionVariable(const Win32ConditionVariable&);
    Win32ConditionVariable& operator=(const Win32ConditionVariable&);

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="Win32ConditionVariable"/> class.
    /// </summary>
    Win32ConditionVariable()
    {
        Windows::InitializeConditionVariable(&_cond);
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="Win32ConditionVariable"/> class.
    /// </summary>
    ~Win32ConditionVariable()
    {
    }

public:

    /// <summary>
    /// Blocks the current thread execution until the condition variable is woken up.
    /// </summary>
    /// <param name="lock">The critical section locked by the current thread.</param>
    void Wait(const Win32CriticalSection& lock)
    {
        Windows::SleepConditionVariableCS(&_cond, &lock._criticalSection, 0xFFFFFFFF);
    }

    /// <summary>
    /// Blocks the current thread execution until the condition variable is woken up or after the specified timeout duration.
    /// </summary>
    /// <param name="lock">The critical section locked by the current thread.</param>
    /// <param name="timeout">The time-out interval, in milliseconds. If the time-out interval elapses, the function re-acquires the critical section and returns zero. If timeout is zero, the function tests the states of the specified objects and returns immediately. If timeout is INFINITE, the function's time-out interval never elapses.</param>
    /// <returns>If the function succeeds, the return value is true, otherwise, if the function fails or the time-out interval elapses, the return value is false.</returns>
    bool Wait(const Win32CriticalSection& lock, const int32 timeout)
    {
        return !!Windows::SleepConditionVariableCS(&_cond, &lock._criticalSection, timeout);
    }

    /// <summary>
    /// Notifies one waiting thread.
    /// </summary>
    void NotifyOne()
    {
        Windows::WakeConditionVariable(&_cond);
    }

    /// <summary>
    /// Notifies all waiting threads.
    /// </summary>
    void NotifyAll()
    {
        Windows::WakeAllConditionVariable(&_cond);
    }
};

#endif
