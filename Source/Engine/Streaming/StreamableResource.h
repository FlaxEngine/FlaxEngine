// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Collections/SamplesBuffer.h"

class StreamingGroup;
class Task;

/// <summary>
/// Base class for all resource types that can be dynamically streamed.
/// </summary>
class FLAXENGINE_API StreamableResource
{
protected:

    StreamingGroup* _group;
    bool _isDynamic, _isStreaming;
    float _streamingQuality;

    StreamableResource(StreamingGroup* group);
    ~StreamableResource();

public:

    /// <summary>
    /// Gets resource group.
    /// </summary>
    FORCE_INLINE StreamingGroup* GetGroup() const
    {
        return _group;
    }

    /// <summary>
    /// Gets value indicating whenever resource can be used in dynamic streaming (otherwise use always the best quality).
    /// </summary>
    FORCE_INLINE bool IsDynamic() const
    {
        return _isDynamic;
    }

    /// <summary>
    /// Gets resource streaming quality level
    /// </summary>
    FORCE_INLINE float GetStreamingQuality() const
    {
        return _streamingQuality;
    }

    /// <summary>
    /// Gets resource target residency level.
    /// </summary>
    FORCE_INLINE int32 GetTargetResidency() const
    {
        return Streaming.TargetResidency;
    }

    /// <summary>
    /// Gets a value indicating whether this resource has been allocated. 
    /// </summary>
    FORCE_INLINE bool IsAllocated() const
    {
        return GetAllocatedResidency() != 0;
    }

    /// <summary>
    /// Gets resource maximum residency level.
    /// </summary>
    virtual int32 GetMaxResidency() const = 0;

    /// <summary>
    /// Gets resource current residency level.
    /// </summary>
    virtual int32 GetCurrentResidency() const = 0;

    /// <summary>
    /// Gets resource allocated residency level.
    /// </summary>
    virtual int32 GetAllocatedResidency() const = 0;

public:

    /// <summary>
    /// Determines whether this instance can be updated. Which means: no async streaming, no pending action in background.
    /// </summary>
    /// <returns><c>true</c> if this instance can be updated; otherwise, <c>false</c>.</returns>
    virtual bool CanBeUpdated() const = 0;

    /// <summary>
    /// Updates the resource allocation to the given residency level. May not be updated now but in an async operation.
    /// </summary>
    /// <param name="residency">The target allocation residency.</param>
    /// <returns>Async task that updates resource allocation or null if already done it. Warning: need to call task start to perform allocation.</returns>
    virtual Task* UpdateAllocation(int32 residency) = 0;

    /// <summary>
    /// Creates streaming task (or tasks sequence) to perform resource streaming for the desire residency level.
    /// </summary>
    /// <param name="residency">The target residency.</param>
    /// <returns>Async task or tasks that update resource residency level. Must be preceded with UpdateAllocation call.</returns>
    virtual Task* CreateStreamingTask(int32 residency) = 0;

public:

    struct StreamingCache
    {
        int64 LastUpdate = 0;
        int32 TargetResidency = 0;
        int64 TargetResidencyChange = 0;
        SamplesBuffer<float, 5> QualitySamples;
    };

    StreamingCache Streaming;

    /// <summary>
    /// Requests the streaming update for this resource during next streaming manager update.
    /// </summary>
    void RequestStreamingUpdate()
    {
        Streaming.LastUpdate = 0;
    }

protected:

    void StartStreaming(bool isDynamic);
    void StopStreaming();
};
