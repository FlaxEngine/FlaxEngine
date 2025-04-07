// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Object.h"

/// <summary>
/// Removing old objects service. Your friendly garbage collector!
/// </summary>
class FLAXENGINE_API ObjectsRemovalService
{
public:
    /// <summary>
    /// Determines whether object has been registered in the pool for the removing.
    /// </summary>
    /// <param name="obj">The object.</param>
    /// <returns>True if object has been registered in the pool for the removing, otherwise false.</returns>
    static bool IsInPool(Object* obj);

    /// <summary>
    /// Removes the specified object from the dead pool (clears the reference to it).
    /// </summary>
    /// <param name="obj">The object.</param>
    static void Dereference(Object* obj);

    /// <summary>
    /// Adds the specified object to the dead pool.
    /// </summary>
    /// <param name="obj">The object.</param>
    /// <param name="timeToLive">The time to live (in seconds).</param>
    /// <param name="useGameTime">True if unscaled game time for the object life timeout, otherwise false to use absolute time.</param>
    static void Add(Object* obj, float timeToLive = 1.0f, bool useGameTime = false);

    /// <summary>
    /// Flushes the objects pool removing objects marked to remove now (with negative or zero time to live).
    /// </summary>
    FORCE_INLINE static void Flush()
    {
        Flush(0, 0);
    }

    /// <summary>
    /// Flushes the objects pool.
    /// </summary>
    /// <param name="dt">The delta time (in seconds).</param>
    /// <param name="gameDelta">The game update delta time (in seconds).</param>
    static void Flush(float dt, float gameDelta);

    /// <summary>
    /// Forces the flush the all objects from the pool.
    /// </summary>
    FORCE_INLINE static void ForceFlush()
    {
        Flush(1000, 1000);
    }
};
