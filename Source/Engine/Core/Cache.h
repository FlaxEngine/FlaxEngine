// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Serialization/ISerializeModifier.h"
#include "Engine/Core/Collections/CollectionPoolCache.h"

/// <summary>
/// Acceleration structure used to improve operations performed by the Engine. Cache the data and allows to reuse memory container for less allocations at runtime. It's a thread-safe.
/// </summary>
class Cache
{
public:

    static void ISerializeModifierClearCallback(ISerializeModifier* obj);

public:

    /// <summary>
    /// Gets the ISerializeModifier lookup cache. Safe allocation, per thread, uses caching.
    /// </summary>
    static CollectionPoolCache<ISerializeModifier, ISerializeModifierClearCallback> ISerializeModifier;

public:

    /// <summary>
    /// Releases all the allocated resources (existing in the pool that are not during usage).
    /// </summary>
    static void Release();
};
