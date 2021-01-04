// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Singleton.h"
#include "MTypes.h"

/// <summary>
/// Main handler for CLR Engine.
/// </summary>
class FLAXENGINE_API MCore : public Singleton<MCore>
{
    friend MDomain;

private:

    MDomain* _rootDomain;
    MDomain* _activeDomain;
    Array<MDomain*, InlinedAllocation<4>> _domains;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="MCore"/> class.
    /// </summary>
    MCore();

public:

    /// <summary>
    /// Creates an new empty domain.
    /// </summary>
    /// <param name="domainName">The domain name to create.</param>
    /// <returns>The domain object.</returns>
    MDomain* CreateDomain(const MString& domainName);

    /// <summary>
    /// Unloads the domain.
    /// </summary>
    /// <param name="domainName">The domain name to remove.</param>
    void UnloadDomain(const MString& domainName);

    /// <summary>
    /// Gets the root domain.
    /// </summary>
    /// <returns>The root domain.</returns>
    FORCE_INLINE MDomain* GetRootDomain() const
    {
        return _rootDomain;
    }

    /// <summary>
    /// Gets the currently active domain.
    /// </summary>
    /// <returns>The current domain.</returns>
    FORCE_INLINE MDomain* GetActiveDomain() const
    {
        return _activeDomain;
    }

public:

    /// <summary>
    /// Initialize CLR Engine
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    bool LoadEngine();

    /// <summary>
    /// Unload CLR Engine
    /// </summary>
    void UnloadEngine();

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
