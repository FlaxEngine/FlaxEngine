// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Platform/CriticalSection.h"

class GPUResource;
class StringBuilder;

/// <summary>
/// GPU Resources collection container
/// </summary>
class GPUResourcesCollection
{
private:

    CriticalSection _locker;
    Array<GPUResource*> _collection;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResourcesCollection"/> class.
    /// </summary>
    GPUResourcesCollection()
        : _collection(1024)
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUResourcesCollection"/> class.
    /// </summary>
    ~GPUResourcesCollection()
    {
    }

public:

    /// <summary>
    /// Gets the total memory usage (in bytes).
    /// </summary>
    /// <returns>GPU memory usage (in bytes).</returns>
    uint64 GetMemoryUsage() const;

    /// <summary>
    /// Called when device is being disposed.
    /// </summary>
    void OnDeviceDispose();

    /// <summary>
    /// Dumps all resources information to the log.
    /// </summary>
    void DumpToLog() const;

    /// <summary>
    /// Dumps all resources information to the log.
    /// </summary>
    void DumpToLog(StringBuilder& output) const;

public:

    /// <summary>
    /// Adds the specified resource to the collection.
    /// </summary>
    /// <param name="resource">The resource.</param>
    void Add(GPUResource* resource);

    /// <summary>
    /// Removes the specified resource from the collection.
    /// </summary>
    /// <param name="resource">The resource.</param>
    void Remove(GPUResource* resource);
};
