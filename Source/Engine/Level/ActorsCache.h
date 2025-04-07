// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Actor.h"
#include "Engine/Serialization/ISerializeModifier.h"
#include "Engine/Core/Collections/CollectionPoolCache.h"

/// <summary>
/// Acceleration structure used to improve operations performed on a set of actors. Cache the data and allows to reuse memory container for less allocations. It's a thread-safe.
/// </summary>
class ActorsCache
{
public:
    typedef ActorsLookup ActorsLookupType;
    typedef Array<Actor*> ActorsListType;
    typedef Array<SceneObject*> SceneObjectsListType;

public:
    /// <summary>
    /// Gets the actors lookup cached. Safe allocation, per thread, uses caching.
    /// </summary>
    static CollectionPoolCache<ActorsLookupType> ActorsLookupCache;

    /// <summary>
    /// Gets the actors lookup cached. Safe allocation, per thread, uses caching.
    /// </summary>
    static CollectionPoolCache<ActorsListType> ActorsListCache;

    /// <summary>
    /// Gets the scene objects lookup cached. Safe allocation, per thread, uses caching.
    /// </summary>
    static CollectionPoolCache<SceneObjectsListType> SceneObjectsListCache;
};
