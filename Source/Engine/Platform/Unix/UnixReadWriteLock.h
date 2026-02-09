// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_UNIX

#include "Engine/Platform/Platform.h"
#include <pthread.h>

/// <summary>
/// Unix implementation of a read/write lock that allows for shared reading by multiple threads and exclusive writing by a single thread.
/// </summary>
class FLAXENGINE_API UnixReadWriteLock
{
private:
    mutable pthread_rwlock_t _lock;

private:
    NON_COPYABLE(UnixReadWriteLock);

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="UnixReadWriteLock"/> class.
    /// </summary>
    UnixReadWriteLock()
    {
        pthread_rwlock_init(&_lock, nullptr);
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="UnixReadWriteLock"/> class.
    /// </summary>
    ~UnixReadWriteLock()
    {
        pthread_rwlock_destroy(&_lock);
    }

public:
    /// <summary>
    /// Locks for shared reading.
    /// </summary>
    void ReadLock() const
    {
        pthread_rwlock_rdlock(&_lock);
    }

    /// <summary>
    /// Releases the lock after shared reading.
    /// </summary>
    void ReadUnlock() const
    {
        pthread_rwlock_unlock(&_lock);
    }
    /// <summary>
    /// Locks for exclusive writing.
    /// </summary>
    void WriteLock() const
    {
        pthread_rwlock_wrlock(&_lock);
    }

    /// <summary>
    /// Releases the lock after exclusive writing.
    /// </summary>
    void WriteUnlock() const
    {
        pthread_rwlock_unlock(&_lock);
    }
};

#endif
