// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/CriticalSection.h"

class StreamableResource;

/// <summary>
/// Container for collection of Streamable Resources.
/// </summary>
class StreamableResourcesCollection
{
private:

    CriticalSection _locker;
    Array<StreamableResource*> _resources;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="StreamableResourcesCollection"/> class.
    /// </summary>
    StreamableResourcesCollection()
        : _resources(4096)
    {
    }

public:

    /// <summary>
    /// Event called on new resource added to the collection.
    /// </summary>
    Delegate<StreamableResource*> Added;

    /// <summary>
    /// Event called on new resource added from the collection.
    /// </summary>
    Delegate<StreamableResource*> Removed;

public:

    /// <summary>
    /// Gets the amount of registered resources.
    /// </summary>
    /// <returns>The resources count.</returns>
    int32 ResourcesCount() const
    {
        _locker.Lock();
        const int32 result = _resources.Count();
        _locker.Unlock();
        return result;
    }

    /// <summary>
    /// Gets the resource at the given index.
    /// </summary>
    /// <param name="index">The index.</param>
    /// <returns>The resource at the given index.</returns>
    StreamableResource* operator[](int32 index) const
    {
        _locker.Lock();
        StreamableResource* result = _resources.At(index);
        _locker.Unlock();
        return result;
    }

public:

    /// <summary>
    /// Adds the resource to the collection.
    /// </summary>
    /// <param name="resource">The resource to add.</param>
    void Add(StreamableResource* resource)
    {
        ASSERT(resource);

        _locker.Lock();
        ASSERT(_resources.Contains(resource) == false);
        _resources.Add(resource);
        _locker.Unlock();

        Added(resource);
    }

    /// <summary>
    /// Removes resource from the collection.
    /// </summary>
    /// <param name="resource">The resource to remove.</param>
    void Remove(StreamableResource* resource)
    {
        ASSERT(resource);

        _locker.Lock();
        ASSERT(_resources.Contains(resource) == true);
        _resources.Remove(resource);
        _locker.Unlock();

        Removed(resource);
    }
};
