// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Platform/Thread.h"

/// <summary>
/// Holds all created threads (except the main thread)
/// </summary>
class FLAXENGINE_API ThreadRegistry
{
public:

    /// <summary>
    /// Gets thread with given ID
    /// </summary>
    /// <param name="id">Thread ID</param>
    /// <returns>Founded thread, or null if is missing</returns>
    static Thread* GetThread(uint64 id);

    /// <summary>
    /// Gets amount of threads in a registry
    /// </summary>
    /// <returns>The amount of threads used by the engine.</returns>
    static int32 Count();

    /// <summary>
    /// Attempts to kill all threads. Also starts playing Metallica album Kill'Em All. Hit the Lights...
    /// </summary>
    static void KillEmAll();

public:

    static void Add(Thread* thread);
    static void Remove(Thread* thread);
};
