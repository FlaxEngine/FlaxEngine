// Copyright (c) Wojciech Figat. All rights reserved.

#include "StreamingHandlers.h"
#include "Streaming.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Graphics/Textures/StreamingTexture.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Content/Assets/Model.h"
#include "Engine/Content/Assets/SkinnedModel.h"
#include "Engine/Audio/AudioClip.h"
#include "Engine/Audio/Audio.h"
#include "Engine/Audio/AudioSource.h"

float TexturesStreamingHandler::CalculateTargetQuality(StreamableResource* resource, double currentTime)
{
    ASSERT(resource);
    auto& texture = *(StreamingTexture*)resource;
    const TextureHeader& header = *texture.GetHeader();
    float result = 1.0f;
    if (header.TextureGroup >= 0 && header.TextureGroup < Streaming::TextureGroups.Count())
    {
        // Quality based on texture group settings
        const TextureGroup& group = Streaming::TextureGroups[header.TextureGroup];
        result = group.Quality;

        // Drop quality if invisible
        const double lastRenderTime = texture.GetTexture()->LastRenderTime;
        if (lastRenderTime < 0 || group.TimeToInvisible <= (float)(currentTime - lastRenderTime))
        {
            result *= group.QualityIfInvisible;
        }
    }
    return result;
}

int32 TexturesStreamingHandler::CalculateResidency(StreamableResource* resource, float quality)
{
    if (quality < ZeroTolerance)
        return 0;
    ASSERT(resource);
    auto& texture = *(StreamingTexture*)resource;
    ASSERT(texture.IsInitialized());
    const TextureHeader& header = *texture.GetHeader();

    const int32 totalMipLevels = texture.TotalMipLevels();
    int32 mipLevels = Math::CeilToInt(quality * (float)totalMipLevels);

    if (header.TextureGroup >= 0 && header.TextureGroup < Streaming::TextureGroups.Count())
    {
        const TextureGroup& group = Streaming::TextureGroups[header.TextureGroup];
        mipLevels = Math::Clamp(mipLevels + group.MipLevelsBias, group.MipLevelsMin, group.MipLevelsMax);
#if USE_EDITOR
        // Simulate per-platform limit in Editor
        int32 max;
        if (group.MipLevelsMaxPerPlatform.TryGet(PLATFORM_TYPE, max))
            mipLevels = Math::Min(mipLevels, max);
#endif
    }

    if (mipLevels > 0 && mipLevels < texture._minMipCountBlockCompressed && texture._isBlockCompressed)
    {
        // Block compressed textures require minimum size of block size (eg. 4 for BC formats)
        mipLevels = texture._minMipCountBlockCompressed;
    }

    return Math::Clamp(mipLevels, 0, totalMipLevels);
}

int32 TexturesStreamingHandler::CalculateRequestedResidency(StreamableResource* resource, int32 targetResidency)
{
    ASSERT(resource);
    auto& texture = *(StreamingTexture*)resource;
    ASSERT(texture.IsInitialized());
    int32 residency = targetResidency;
    int32 currentResidency = texture.GetCurrentResidency();
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

float ModelsStreamingHandler::CalculateTargetQuality(StreamableResource* resource, double currentTime)
{
    // TODO: calculate a proper quality levels for models based on render time and streaming enable/disable options
    return 1.0f;
}

int32 ModelsStreamingHandler::CalculateResidency(StreamableResource* resource, float quality)
{
    if (quality < ZeroTolerance)
        return 0;
    ASSERT(resource);
    const auto& model = *(Model*)resource;
    const int32 lodCount = model.GetLODsCount();
    const int32 lods = Math::CeilToInt(quality * (float)lodCount);
    return lods;
}

int32 ModelsStreamingHandler::CalculateRequestedResidency(StreamableResource* resource, int32 targetResidency)
{
    ASSERT(resource);
    const auto& model = *(Model*)resource;

    // Always load only single LOD at once
    int32 residency = targetResidency;
    const int32 currentResidency = model.GetCurrentResidency();
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

float SkinnedModelsStreamingHandler::CalculateTargetQuality(StreamableResource* resource, double currentTime)
{
    // TODO: calculate a proper quality levels for models based on render time and streaming enable/disable options
    return 1.0f;
}

int32 SkinnedModelsStreamingHandler::CalculateResidency(StreamableResource* resource, float quality)
{
    if (quality < ZeroTolerance)
        return 0;
    ASSERT(resource);
    const auto& model = *(SkinnedModel*)resource;
    const int32 lodCount = model.GetLODsCount();
    const int32 lods = Math::CeilToInt(quality * (float)lodCount);
    return lods;
}

int32 SkinnedModelsStreamingHandler::CalculateRequestedResidency(StreamableResource* resource, int32 targetResidency)
{
    ASSERT(resource);
    const auto& model = *(SkinnedModel*)resource;

    // Always load only single LOD at once
    int32 residency = targetResidency;
    const int32 currentResidency = model.GetCurrentResidency();
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

float AudioStreamingHandler::CalculateTargetQuality(StreamableResource* resource, double currentTime)
{
    // Audio clips don't use quality but only residency
    return 1.0f;
}

int32 AudioStreamingHandler::CalculateResidency(StreamableResource* resource, float quality)
{
    ASSERT(resource);
    auto clip = static_cast<AudioClip*>(resource);
    const int32 chunksCount = clip->Buffers.Count();
    bool chunksMask[ASSET_FILE_DATA_CHUNKS]; // TODO: use single int as bit mask
    Platform::MemoryClear(chunksMask, sizeof(chunksMask));

    // Find audio chunks required for streaming
    clip->StreamingQueue.Clear();
    for (int32 sourceIndex = 0; sourceIndex < Audio::Sources.Count(); sourceIndex++)
    {
        // TODO: collect refs to audio clip from sources and use faster iteration (but do it thread-safe)

        const auto src = Audio::Sources[sourceIndex];
        if (src->Clip == clip && src->GetState() != AudioSource::States::Stopped)
        {
            // Stream the current and the next chunk if could be used in a while
            const int32 chunk = src->_streamingFirstChunk;
            ASSERT(Math::IsInRange(chunk, 0, chunksCount));
            chunksMask[chunk] = true;
            
            const float StreamingDstSec = 2.0f; // TODO: make it configurable via StreamingSettings
            if (chunk + 1 < chunksCount && src->GetTime() + StreamingDstSec >= clip->GetBufferStartTime(src->_streamingFirstChunk))
            {
                chunksMask[chunk + 1] = true;
            }
        }
    }

    // Try to enqueue chunks to modify (load or unload)
    for (int32 i = 0; i < chunksCount; i++)
    {
        if (chunksMask[i] != (clip->Buffers[i] != 0))
        {
            clip->StreamingQueue.Add(i);
        }
    }

    return clip->StreamingQueue.Count();
}

int32 AudioStreamingHandler::CalculateRequestedResidency(StreamableResource* resource, int32 targetResidency)
{
    // No smoothing or slowdown in residency change
    return targetResidency;
}

bool AudioStreamingHandler::RequiresStreaming(StreamableResource* resource, int32 currentResidency, int32 targetResidency)
{
    // Audio clips use streaming queue buffer to detect streaming request start
    const auto clip = static_cast<AudioClip*>(resource);
    return clip->StreamingQueue.HasItems();
}
