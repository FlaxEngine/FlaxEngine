// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

// Enables pinning thread pool to the logical CPU cores with affinity mask
//#define THREAD_POOL_AFFINITY_MASK(thread) (1 << (thread + 1))

/// <summary>
/// Main engine thread pool for threaded tasks system.
/// </summary>
class ThreadPool
{
    friend class ThreadPoolTask;
    friend class ThreadPoolService;
private:

    static int32 ThreadProc();
};
