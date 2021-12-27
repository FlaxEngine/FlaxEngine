// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_MAC

#include "MacCriticalSection.h"
#include <pthread.h>
#include <sys/time.h>

/// <summary>
/// Mac implementation of a condition variables. Condition variables are synchronization primitives that enable threads to wait until a particular condition occurs. Condition variables enable threads to atomically release a lock and enter the sleeping state.
/// </summary>
class FLAXENGINE_API MacConditionVariable
{
private:

    pthread_cond_t _cond;

private:

    MacConditionVariable(const MacConditionVariable&);
    MacConditionVariable& operator=(const MacConditionVariable&);

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="MacConditionVariable"/> class.
    /// </summary>
    MacConditionVariable()
    {
        pthread_cond_init(&_cond, nullptr);
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="MacConditionVariable"/> class.
    /// </summary>
    ~MacConditionVariable()
    {
        pthread_cond_destroy(&_cond);
    }

public:

    /// <summary>
    /// Blocks the current thread execution until the condition variable is woken up.
    /// </summary>
    /// <param name="lock">The critical section locked by the current thread.</param>
    void Wait(const MacCriticalSection& lock)
    {
        pthread_cond_wait(&_cond, lock._mutexPtr);
    }

    /// <summary>
    /// Blocks the current thread execution until the condition variable is woken up or after the specified timeout duration.
    /// </summary>
    /// <param name="lock">The critical section locked by the current thread.</param>
    /// <param name="timeout">The time-out interval, in milliseconds. If the time-out interval elapses, the function re-acquires the critical section and returns zero. If timeout is zero, the function tests the states of the specified objects and returns immediately. If timeout is INFINITE, the function's time-out interval never elapses.</param>
    /// <returns>If the function succeeds, the return value is true, otherwise, if the function fails or the time-out interval elapses, the return value is false.</returns>
    bool Wait(const MacCriticalSection& lock, const int32 timeout)
    {
        struct timeval tv;
        struct timespec ts;

        gettimeofday(&tv, NULL);
        ts.tv_sec = time(NULL) + timeout / 1000;
        ts.tv_nsec = tv.tv_usec * 1000 + 1000 * 1000 * (timeout % 1000);
        ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
        ts.tv_nsec %= (1000 * 1000 * 1000);

        return pthread_cond_timedwait(&_cond, lock._mutexPtr, &ts) == 0;
    }

    /// <summary>
    /// Notifies one waiting thread.
    /// </summary>
    void NotifyOne()
    {
        pthread_cond_signal(&_cond);
    }

    /// <summary>
    /// Notifies all waiting threads.
    /// </summary>
    void NotifyAll()
    {
        pthread_cond_broadcast(&_cond);
    }
};

#endif
