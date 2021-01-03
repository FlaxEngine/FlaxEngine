// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if COMPILE_WITH_PROFILER

#include "ProfilerMemory.h"
#include "ProfilerCPU.h"

void ProfilerMemory::OnAllocation(int32 bytes, bool isGC)
{
    // Register allocation during the current CPU event
    auto thread = ProfilerCPU::GetCurrentThread();
    if (thread != nullptr && thread->Buffer.GetCount() != 0)
    {
        auto& activeEvent = thread->Buffer.Last().Event();
        if (activeEvent.End < ZeroTolerance)
        {
            if (isGC)
                activeEvent.ManagedMemoryAllocation += bytes;
            else
                activeEvent.NativeMemoryAllocation += bytes;
        }
    }
}

#endif
