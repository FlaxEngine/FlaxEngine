// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_UNIX

#include "Engine/Platform/Platform.h"
#include <pthread.h>

class UnixConditionVariable;

/// <summary>
/// Unix implementation of a critical section.
/// </summary>
class FLAXENGINE_API UnixCriticalSection
{
    friend UnixConditionVariable;
private:

    pthread_mutex_t _mutex;
    pthread_mutex_t* _mutexPtr;
#if BUILD_DEBUG
    pthread_t _owningThreadId;
#endif

    UnixCriticalSection(const UnixCriticalSection&);
    UnixCriticalSection& operator=(const UnixCriticalSection&);

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="UnixCriticalSection"/> class.
    /// </summary>
    UnixCriticalSection()
    {
        pthread_mutexattr_t attributes;
        pthread_mutexattr_init(&attributes);
        pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&_mutex, &attributes);
        pthread_mutexattr_destroy(&attributes);
        _mutexPtr = &_mutex;
#if BUILD_DEBUG
        _owningThreadId = 0;
#endif
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="UnixCriticalSection"/> class.
    /// </summary>
    ~UnixCriticalSection()
    {
        pthread_mutex_destroy(&_mutex);
    }

public:

    /// <summary>
    /// Locks the critical section.
    /// </summary>
    NO_SANITIZE_THREAD void Lock() const
    {
        pthread_mutex_lock(_mutexPtr);
#if BUILD_DEBUG
        ((UnixCriticalSection*)this)->_owningThreadId = pthread_self();
#endif
    }

    /// <summary>
    /// Attempts to enter a critical section without blocking. If the call is successful, the calling thread takes ownership of the critical section.
    /// </summary>
    /// <returns>True if calling thread took ownership of the critical section.</returns>
    NO_SANITIZE_THREAD bool TryLock() const
    {
        return pthread_mutex_trylock(_mutexPtr) == 0;
    }

    /// <summary>
    /// Releases the lock on the critical section.
    /// </summary>
    NO_SANITIZE_THREAD void Unlock() const
    {
#if BUILD_DEBUG
        ((UnixCriticalSection*)this)->_owningThreadId = 0;
#endif
        pthread_mutex_unlock(_mutexPtr);
    }
};

#endif
