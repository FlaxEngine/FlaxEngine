// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

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
