// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Config.h"
#include "Engine/Core/Types/DateTime.h"

class StreamingGroup;

/// <summary>
/// Base interface for all streamable resource handlers
/// </summary>
class FLAXENGINE_API IStreamingHandler
{
public:

    virtual ~IStreamingHandler() = default;

public:

    /// <summary>
    /// Calculates target quality level for the given resource.
    /// </summary>
    /// <param name="resource">The resource.</param>
    /// <param name="now">The current time.</param>
    /// <returns>Target Quality</returns>
    virtual StreamingQuality CalculateTargetQuality(StreamableResource* resource, DateTime now) = 0;

    /// <summary>
    /// Calculates the residency level for a given resource and quality level.
    /// </summary>
    /// <param name="resource">The resource.</param>
    /// <param name="quality">The quality level.</param>
    /// <returns>Residency level</returns>
    virtual int32 CalculateResidency(StreamableResource* resource, StreamingQuality quality) = 0;

    /// <summary>
    /// Calculates the residency level to stream for a given resource and target residency.
    /// </summary>
    /// <param name="resource">The resource.</param>
    /// <param name="targetResidency">The target residency level.</param>
    /// <returns>Residency level to stream</returns>
    virtual int32 CalculateRequestedResidency(StreamableResource* resource, int32 targetResidency) = 0;

    /// <summary>
    /// Determines if the specified resource requires the streaming.
    /// </summary>
    /// <param name="resource">The resource.</param>
    /// <param name="currentResidency">The current residency level.</param>
    /// <param name="targetResidency">The target residency level.</param>
    /// <returns>True if perform resource streaming, otherwise false.</returns>
    virtual bool RequiresStreaming(StreamableResource* resource, int32 currentResidency, int32 targetResidency)
    {
        return currentResidency != targetResidency;
    }
};
