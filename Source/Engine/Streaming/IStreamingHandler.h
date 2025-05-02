// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

class StreamingGroup;
class StreamableResource;

/// <summary>
/// Base interface for all streamable resource handlers that implement resource streaming policy.
/// </summary>
class FLAXENGINE_API IStreamingHandler
{
public:
    virtual ~IStreamingHandler() = default;

    /// <summary>
    /// Calculates target quality level (0-1) for the given resource.
    /// </summary>
    /// <param name="resource">The resource.</param>
    /// <param name="currentTime">The current platform time (seconds).</param>
    /// <returns>Target quality (0-1).</returns>
    virtual float CalculateTargetQuality(StreamableResource* resource, double currentTime) = 0;

    /// <summary>
    /// Calculates the residency level for a given resource and quality level.
    /// </summary>
    /// <param name="resource">The resource.</param>
    /// <param name="quality">The quality level (0-1).</param>
    /// <returns>Residency level</returns>
    virtual int32 CalculateResidency(StreamableResource* resource, float quality) = 0;

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
