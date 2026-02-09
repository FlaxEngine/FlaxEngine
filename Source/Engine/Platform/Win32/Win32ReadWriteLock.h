// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WIN32

#include "WindowsMinimal.h"

/// <summary>
/// Win32 implementation of a read/write lock that allows for shared reading by multiple threads and exclusive writing by a single thread.
/// </summary>
class FLAXENGINE_API Win32ReadWriteLock
{
private:
    mutable Windows::SRWLOCK _lock;

private:
    NON_COPYABLE(Win32ReadWriteLock);

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="Win32ReadWriteLock"/> class.
    /// </summary>
    Win32ReadWriteLock()
    {
        Windows::InitializeSRWLock(&_lock);
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="Win32ReadWriteLock"/> class.
    /// </summary>
    ~Win32ReadWriteLock()
    {
    }

public:
    /// <summary>
    /// Locks for shared reading.
    /// </summary>
    __forceinline void ReadLock() const
    {
        Windows::AcquireSRWLockShared(&_lock);
    }

    /// <summary>
    /// Releases the lock after shared reading.
    /// </summary>
    __forceinline void ReadUnlock() const
    {
        Windows::ReleaseSRWLockShared(&_lock);
    }
    /// <summary>
    /// Locks for exclusive writing.
    /// </summary>
    __forceinline void WriteLock() const
    {
        Windows::AcquireSRWLockExclusive(&_lock);
    }

    /// <summary>
    /// Releases the lock after exclusive writing.
    /// </summary>
    __forceinline void WriteUnlock() const
    {
        Windows::ReleaseSRWLockExclusive(&_lock);
    }
};

#endif
