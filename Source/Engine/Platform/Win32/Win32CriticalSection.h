// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WIN32

#include "WindowsMinimal.h"

class Win32ConditionVariable;

/// <summary>
/// Win32 implementation of a critical section. Shared between Windows and UWP platforms.
/// </summary>
class FLAXENGINE_API Win32CriticalSection
{
    friend Win32ConditionVariable;

private:
    mutable Windows::CRITICAL_SECTION _criticalSection;

private:
    Win32CriticalSection(const Win32CriticalSection&);
    Win32CriticalSection& operator=(const Win32CriticalSection&);

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="Win32CriticalSection"/> class.
    /// </summary>
    Win32CriticalSection()
    {
        Windows::InitializeCriticalSectionEx(&_criticalSection, 4000, 0x01000000);
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="Win32CriticalSection"/> class.
    /// </summary>
    ~Win32CriticalSection()
    {
        Windows::DeleteCriticalSection(&_criticalSection);
    }

public:
    /// <summary>
    /// Locks the critical section.
    /// </summary>
    void Lock() const
    {
        Windows::EnterCriticalSection(&_criticalSection);
    }

    /// <summary>
    /// Attempts to enter a critical section without blocking. If the call is successful, the calling thread takes ownership of the critical section.
    /// </summary>
    /// <returns>True if calling thread took ownership of the critical section.</returns>
    bool TryLock() const
    {
        return Windows::TryEnterCriticalSection(&_criticalSection) != 0;
    }

    /// <summary>
    /// Releases the lock on the critical section.
    /// </summary>
    void Unlock() const
    {
        Windows::LeaveCriticalSection(&_criticalSection);
    }
};

#endif
