// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "ModelsStreamingHandler.h"
#include "Engine/Content/Assets/Model.h"

StreamingQuality ModelsStreamingHandler::CalculateTargetQuality(StreamableResource* resource, DateTime now)
{
    ASSERT(resource);

    // TODO: calculate a proper quality levels for models based on render time and streaming enable/disable options

    return MAX_STREAMING_QUALITY;
}

int32 ModelsStreamingHandler::CalculateResidency(StreamableResource* resource, StreamingQuality quality)
{
    ASSERT(resource);
    auto& model = *(Model*)resource;
    auto lodCount = model.GetLODsCount();

    if (quality < ZeroTolerance)
        return 0;

    int32 lods = Math::CeilToInt(quality * (StreamingQuality)lodCount);

    ASSERT(model.IsValidLODIndex(lods - 1));
    return lods;
}

int32 ModelsStreamingHandler::CalculateRequestedResidency(StreamableResource* resource, int32 targetResidency)
{
    ASSERT(resource);
    auto& model = *(Model*)resource;

    // Always load only single LOD at once
    int32 residency = targetResidency;
    int32 currentResidency = model.GetCurrentResidency();
    if (currentResidency < targetResidency)
    {
        // Up
        residency = currentResidency + 1;
    }
    else if (currentResidency > targetResidency)
    {
        // Down
        residency = currentResidency - 1;
    }

    return residency;
}
