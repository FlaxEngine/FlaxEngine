// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "TexturesStreamingHandler.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Graphics/Textures/StreamingTexture.h"

StreamingQuality TexturesStreamingHandler::CalculateTargetQuality(StreamableResource* resource, DateTime now)
{
    ASSERT(resource);
    auto& texture = *(StreamingTexture*)resource;

    // TODO: calculate a proper quality levels for the textures

    //return 0.5f;

    return MAX_STREAMING_QUALITY;
}

int32 TexturesStreamingHandler::CalculateResidency(StreamableResource* resource, StreamingQuality quality)
{
    ASSERT(resource);
    auto& texture = *(StreamingTexture*)resource;
    ASSERT(texture.IsInitialized());
    const auto totalMipLevels = texture.TotalMipLevels();

    if (quality < ZeroTolerance)
    {
        return 0;
    }

    int32 mipLevels = Math::CeilToInt(quality * totalMipLevels);
    if (mipLevels > 0 && mipLevels < 3 && totalMipLevels > 1 && texture.IsBlockCompressed())
    {
        // Block compressed textures require minimum size of 4 (3 mips or more)
        mipLevels = 3;
    }

    ASSERT(Math::IsInRange(mipLevels, 0, totalMipLevels));
    return mipLevels;
}

int32 TexturesStreamingHandler::CalculateRequestedResidency(StreamableResource* resource, int32 targetResidency)
{
    ASSERT(resource);
    auto& texture = *(StreamingTexture*)resource;
    ASSERT(texture.IsInitialized());

    int32 residency = targetResidency;
    int32 currentResidency = texture.GetCurrentResidency();

    // Check if go up or down
    if (currentResidency < targetResidency)
    {
        // Up
        residency = Math::Min(currentResidency + 2, targetResidency);

        // Stream first a few mips very fast (they are very small)
        const int32 QuickStartMipsCount = 6;
        if (currentResidency == 0)
        {
            residency = Math::Min(QuickStartMipsCount, targetResidency);
        }
    }
    else if (currentResidency > targetResidency)
    {
        // Down at once
        residency = targetResidency;
    }

    return residency;
}
