// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Engine/Globals.h"

/// <summary>
/// Checks if current execution in on the main thread.
/// </summary>
/// <returns>True if running on the main thread, otherwise false.</returns>
inline bool IsInMainThread()
{
    return Globals::MainThreadID == Platform::GetCurrentThreadID();
}

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
    /// Init, enters critical section
    /// </summary>
    /// <param name="section">The synchronization object to manage</param>
    ScopeLock(const CriticalSection* section)
        : _section(section)
    {
        ASSERT_LOW_LAYER(_section);
        _section->Lock();
    }

    /// <summary>
    /// Init, enters critical section
    /// </summary>
    /// <param name="section">The synchronization object to manage</param>
    ScopeLock(const CriticalSection& section)
        : _section(&section)
    {
        _section->Lock();
    }

    /// <summary>
    /// Destructor, releases critical section
    /// </summary>
    ~ScopeLock()
    {
        _section->Unlock();
    }
};
