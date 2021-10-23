// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "MTypes.h"

/// <summary>
/// Main handler for CLR Engine.
/// </summary>
class FLAXENGINE_API MCore
{
public:

    /// <summary>
    /// Gets the root domain.
    /// </summary>
    static MDomain* GetRootDomain();

    /// <summary>
    /// Gets the currently active domain.
    /// </summary>
    static MDomain* GetActiveDomain();

    /// <summary>
    /// Creates an new empty domain.
    /// </summary>
    /// <param name="domainName">The domain name to create.</param>
    /// <returns>The domain object.</returns>
    static MDomain* CreateDomain(const MString& domainName);

    /// <summary>
    /// Unloads the domain.
    /// </summary>
    /// <param name="domainName">The domain name to remove.</param>
    static void UnloadDomain(const MString& domainName);

public:

    /// <summary>
    /// Initialize CLR Engine
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    static bool LoadEngine();

    /// <summary>
    /// Unload CLR Engine
    /// </summary>
    static void UnloadEngine();

    /// <summary>
    /// Attaches CLR runtime to the current thread. Use it to allow invoking managed runtime from native threads.
    /// </summary>
    static void AttachThread();

    /// <summary>
    /// Exits the managed runtime thread. Clears the thread data and sets its exit code to 0. Use it before ending the native thread that uses AttachThread.
    /// </summary>
    static void ExitThread();

public:

    /// <summary>
    /// Helper utilities for C# garbage collector.
    /// </summary>
    class GC
    {
    public:

        /// <summary>
        /// Forces an immediate garbage collection of all generations.
        /// </summary>
        static void Collect();

        /// <summary>
        /// Forces an immediate garbage collection of the given generation.
        /// </summary>
        /// <param name="generation">The target generation</param>
        static void Collect(int32 generation);

        /// <summary>
        /// Suspends the current thread until the thread that is processing the queue of finalizers has emptied that queue.
        /// </summary>
        static void WaitForPendingFinalizers();
    };
};
