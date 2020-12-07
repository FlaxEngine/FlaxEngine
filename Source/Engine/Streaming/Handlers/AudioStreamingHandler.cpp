// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "AudioStreamingHandler.h"
#include "Engine/Audio/AudioClip.h"
#include "Engine/Audio/Audio.h"
#include "Engine/Audio/AudioSource.h"

StreamingQuality AudioStreamingHandler::CalculateTargetQuality(StreamableResource* resource, DateTime now)
{
    // Audio clips don't use quality but only residency
    return MAX_STREAMING_QUALITY;
}

int32 AudioStreamingHandler::CalculateResidency(StreamableResource* resource, StreamingQuality quality)
{
    ASSERT(resource);
    auto clip = static_cast<AudioClip*>(resource);
    const int32 chunksCount = clip->Buffers.Count();
    bool chunksMask[ASSET_FILE_DATA_CHUNKS];
    Platform::MemoryClear(chunksMask, sizeof(chunksMask));

    // Find audio chunks required for streaming
    clip->StreamingQueue.Clear();
    for (int32 sourceIndex = 0; sourceIndex < Audio::Sources.Count(); sourceIndex++)
    {
        // TODO: collect refs to audio clip from sources and use faster iteration (but do it thread-safe)

        const auto src = Audio::Sources[sourceIndex];
        if (src->Clip == clip && src->GetState() == AudioSource::States::Playing)
        {
            // Stream the current and the next chunk if could be used in a while
            const int32 chunk = src->_streamingFirstChunk;
            ASSERT(Math::IsInRange(chunk, 0, chunksCount));
            chunksMask[chunk] = true;

            const float StreamingDstSec = 2.0f;
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
