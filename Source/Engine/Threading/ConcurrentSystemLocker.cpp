// Copyright (c) Wojciech Figat. All rights reserved.

#include "ConcurrentSystemLocker.h"
#include "Engine/Platform/Platform.h"
#if !BUILD_RELEASE
#include "Engine/Core/Log.h"
#endif

ConcurrentSystemLocker::ConcurrentSystemLocker()
{
    _counters[0] = _counters[1] = 0;
}

void ConcurrentSystemLocker::Begin(bool write, bool exclusively)
{
    volatile int64* thisCounter = &_counters[write];
    volatile int64* otherCounter = &_counters[!write];

#if !BUILD_RELEASE
    int32 retries = 0;
    double startTime = Platform::GetTimeSeconds();
#endif
RETRY:
#if !BUILD_RELEASE
    retries++;
    if (retries > 1000)
    {
        double endTime = Platform::GetTimeSeconds();
        if (endTime - startTime > 0.5f)
        {
            LOG(Error, "Deadlock detected in ConcurrentSystemLocker! Thread 0x{0:x} waits for {1} ms...", Platform::GetCurrentThreadID(), (int32)((endTime - startTime) * 1000.0));
            retries = 0;
        }
    }
#endif

    // Check if we can enter (cannot read while someone else is writing and vice versa)
    if (Platform::AtomicRead(otherCounter) != 0)
    {
        // Someone else is doing opposite operation so wait for it's end
        // TODO: use ConditionVariable+CriticalSection to prevent active-waiting
        Platform::Yield();
        goto RETRY;
    }

    // Writers might want to check themselves for a single writer at the same time - just like a mutex
    if (exclusively && Platform::AtomicRead(thisCounter) != 0)
    {
        // Someone else is doing opposite operation so wait for it's end
        Platform::Yield();
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

bool ConcurrentSystemLocker::HasLock(bool write) const
{
    return Platform::AtomicRead(&_counters[write]) != 0;
}
