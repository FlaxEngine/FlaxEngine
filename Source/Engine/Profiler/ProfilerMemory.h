// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

#if COMPILE_WITH_PROFILER

/// <summary>
/// Provides memory allocations measuring methods.
/// </summary>
class FLAXENGINE_API ProfilerMemory
{
public:

    /// <summary>
    /// Called on memory allocation.
    /// </summary>
    /// <param name="bytes">The allocated bytes count.</param>
    /// <param name="isGC">True if allocation comes from the Garbage Collector, otherwise false.</param>
    static void OnAllocation(int32 bytes, bool isGC);
};

#endif
