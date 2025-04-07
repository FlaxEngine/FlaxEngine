// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Streaming/IStreamingHandler.h"

/// <summary>
/// Implementation of IStreamingHandler for streamable textures.
/// </summary>
class FLAXENGINE_API TexturesStreamingHandler : public IStreamingHandler
{
public:
    // [IStreamingHandler]
    float CalculateTargetQuality(StreamableResource* resource, double currentTime) override;
    int32 CalculateResidency(StreamableResource* resource, float quality) override;
    int32 CalculateRequestedResidency(StreamableResource* resource, int32 targetResidency) override;
};

/// <summary>
/// Implementation of IStreamingHandler for streamable models.
/// </summary>
class FLAXENGINE_API ModelsStreamingHandler : public IStreamingHandler
{
public:
    // [IStreamingHandler]
    float CalculateTargetQuality(StreamableResource* resource, double currentTime) override;
    int32 CalculateResidency(StreamableResource* resource, float quality) override;
    int32 CalculateRequestedResidency(StreamableResource* resource, int32 targetResidency) override;
};

/// <summary>
/// Implementation of IStreamingHandler for streamable skinned models.
/// </summary>
class FLAXENGINE_API SkinnedModelsStreamingHandler : public IStreamingHandler
{
public:
    // [IStreamingHandler]
    float CalculateTargetQuality(StreamableResource* resource, double currentTime) override;
    int32 CalculateResidency(StreamableResource* resource, float quality) override;
    int32 CalculateRequestedResidency(StreamableResource* resource, int32 targetResidency) override;
};

/// <summary>
/// Implementation of IStreamingHandler for audio clips.
/// </summary>
class FLAXENGINE_API AudioStreamingHandler : public IStreamingHandler
{
public:
    // [IStreamingHandler]
    float CalculateTargetQuality(StreamableResource* resource, double currentTime) override;
    int32 CalculateResidency(StreamableResource* resource, float quality) override;
    int32 CalculateRequestedResidency(StreamableResource* resource, int32 targetResidency) override;
    bool RequiresStreaming(StreamableResource* resource, int32 currentResidency, int32 targetResidency) override;
};
