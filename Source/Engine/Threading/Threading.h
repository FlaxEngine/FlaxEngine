// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/CriticalSection.h"

/// <summary>
/// Checks if current execution in on the main thread.
/// </summary>
FLAXENGINE_API bool IsInMainThread();

/// <summary>
/// Scope locker for critical section.
/// </summary>
class ScopeLock
{
private:

    const CriticalSection* _section;

    ScopeLock() = default;
    ScopeLock(const ScopeLock&) = delete;
    ScopeLock& operator=(const ScopeLock&) = delete;

public:

    /// <summary>
    /// Init, enters critical section.
    /// </summary>
    /// <param name="section">The synchronization object to lock.</param>
    ScopeLock(const CriticalSection& section)
        : _section(&section)
    {
        _section->Lock();
    }

    /// <summary>
    /// Destructor, releases critical section.
    /// </summary>
    ~ScopeLock()
    {
        _section->Unlock();
    }
};
