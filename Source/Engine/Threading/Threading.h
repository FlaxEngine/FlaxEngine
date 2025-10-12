// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/CriticalSection.h"
#include "Engine/Platform/ReadWriteLock.h"

/// <summary>
/// Checks if current execution in on the main thread.
/// </summary>
FLAXENGINE_API bool IsInMainThread();

/// <summary>
/// Scope lock for critical section (mutex). Ensures no other thread can enter scope.
/// </summary>
class ScopeLock
{
private:
    const CriticalSection* _section;
    ScopeLock() = delete;
    NON_COPYABLE(ScopeLock);

public:
    FORCE_INLINE ScopeLock(const CriticalSection& section)
        : _section(&section)
    {
        _section->Lock();
    }

    FORCE_INLINE ~ScopeLock()
    {
        _section->Unlock();
    }
};

/// <summary>
/// Scope lock for read/write lock that allows for shared reading by multiple threads (no writers allowed).
/// </summary>
class ScopeReadLock
{
private:
    const ReadWriteLock* _lock;
    ScopeReadLock() = delete;
    NON_COPYABLE(ScopeReadLock);

public:
    FORCE_INLINE ScopeReadLock(const ReadWriteLock& lock)
        : _lock(&lock)
    {
        _lock->ReadLock();
    }

    FORCE_INLINE ~ScopeReadLock()
    {
        _lock->ReadUnlock();
    }
};

/// <summary>
/// Scope lock for read/write lock that allows for exclusive writing by a single thread (no readers allowed).
/// </summary>
class ScopeWriteLock
{
private:
    const ReadWriteLock* _lock;
    ScopeWriteLock() = delete;
    NON_COPYABLE(ScopeWriteLock);

public:
    FORCE_INLINE ScopeWriteLock(const ReadWriteLock& lock)
        : _lock(&lock)
    {
        _lock->WriteLock();
    }

    FORCE_INLINE ~ScopeWriteLock()
    {
        _lock->WriteUnlock();
    }
};
