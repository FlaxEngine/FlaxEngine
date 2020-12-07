// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Collections/SamplesBuffer.h"
#include "Config.h"

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
    StreamingQuality _streamingQuality;

    StreamableResource(StreamingGroup* group);
    ~StreamableResource();

public:

    /// <summary>
    /// Gets resource group
    /// </summary>
    /// <returns>Streaming Group</returns>
    FORCE_INLINE StreamingGroup* GetGroup() const
    {
        return _group;
    }

    /// <summary>
    /// Gets value indicating whenever resource can be used in dynamic streaming (otherwise use always the best quality)
    /// </summary>
    /// <returns>Is dynamic streamable</returns>
    FORCE_INLINE bool IsDynamic() const
    {
#if ENABLE_RESOURCES_DYNAMIC_STREAMING
        return _isDynamic;
#else
		return false;
#endif
    }

    /// <summary>
    /// Gets resource streaming quality level
    /// </summary>
    /// <returns>Streaming Quality level</returns>
    FORCE_INLINE StreamingQuality GetStreamingQuality() const
    {
        return _streamingQuality;
    }

    /// <summary>
    /// Gets resource maximum residency level.
    /// </summary>
    /// <returns>Residency</returns>
    virtual int32 GetMaxResidency() const = 0;

    /// <summary>
    /// Gets resource current residency level.
    /// </summary>
    /// <returns>Residency</returns>
    virtual int32 GetCurrentResidency() const = 0;

    /// <summary>
    /// Gets resource allocated residency level.
    /// </summary>
    /// <returns>Residency</returns>
    virtual int32 GetAllocatedResidency() const = 0;

    /// <summary>
    /// Gets resource target residency level.
    /// </summary>
    /// <returns>Residency</returns>
    int32 GetTargetResidency() const
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

    // Streaming Manager cached variables
    struct StreamingCache
    {
        /// <summary>
        /// The minimum usage distance since last update (eg. mesh draw distance from camera).
        /// Used to calculate resource quality.
        /// </summary>
        //float MinDstSinceLastUpdate;

        DateTime LastUpdate;
        int32 TargetResidency;
        DateTime TargetResidencyChange;
        SamplesBuffer<StreamingQuality, 5> QualitySamples;

        StreamingCache()
        //: MinDstSinceLastUpdate(MAX_float)
            : LastUpdate(0)
            , TargetResidency(0)
            , TargetResidencyChange(0)
        {
        }
    };

    StreamingCache Streaming;

    /// <summary>
    /// Requests the streaming update for this resource during next streaming manager update.
    /// </summary>
    void RequestStreamingUpdate()
    {
        Streaming.LastUpdate.Ticks = 0;
    }

protected:

    void startStreaming(bool isDynamic);
    void stopStreaming();
};
