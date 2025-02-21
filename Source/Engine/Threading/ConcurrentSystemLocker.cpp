// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ConcurrentSystemLocker.h"
#include "Engine/Platform/Platform.h"

ConcurrentSystemLocker::ConcurrentSystemLocker()
{
    _counters[0] = _counters[1] = 0;
}

void ConcurrentSystemLocker::Begin(bool write)
{
    volatile int64* thisCounter = &_counters[write];
    volatile int64* otherCounter = &_counters[!write];
RETRY:
    // Check if we can enter (cannot read while someone else is writing and vice versa)
    if (Platform::AtomicRead(otherCounter) != 0)
    {
        // Someone else is doing opposite operation so wait for it's end
        // TODO: use ConditionVariable+CriticalSection to prevent active-waiting
        Platform::Sleep(1);
        goto RETRY;
    }

    // Mark that we entered this section
    Platform::InterlockedIncrement(thisCounter);

    // Double-check if we're safe to go
    if (Platform::InterlockedCompareExchange(otherCounter, 0, 0))
    {
        // Someone else is doing opposite operation while this thread was doing counter increment so retry
        Platform::InterlockedDecrement(thisCounter);
        goto RETRY;
    }
}

void ConcurrentSystemLocker::End(bool write)
{
    // Mark that we left this section
    Platform::InterlockedDecrement(&_counters[write]);
}
