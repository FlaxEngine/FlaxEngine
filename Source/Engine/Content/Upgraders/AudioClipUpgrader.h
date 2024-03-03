// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if USE_EDITOR

#include "BinaryAssetUpgrader.h"
#include "Engine/Audio/AudioClip.h"
#include "Engine/Tools/AudioTool/OggVorbisDecoder.h"
#include "Engine/Serialization/MemoryReadStream.h"

/// <summary>
/// Audio Clip asset upgrader.
/// </summary>
/// <seealso cref="BinaryAssetUpgrader" />
class AudioClipUpgrader : public BinaryAssetUpgrader
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="AudioClipUpgrader"/> class.
    /// </summary>
    AudioClipUpgrader()
    {
        static const Upgrader upgraders[] =
        {
            { 1, 2, &Upgrade_1_To_2 },
        };
        setup(upgraders, ARRAY_COUNT(upgraders));
    }

private:
    // ============================================
    //                  Version 1:
    // Designed: 26.02.2018
    // Header version 1.
    // Chunks with audio data.
    // ============================================
    //                  Version 2:
    // Designed: 08.08.2019
    // Header version 2.
    // Chunks with audio data.
    // ============================================

    /// <summary>
    /// Audio Clip resource header structure, version 1. Added on 26.02.2018.
    /// [Deprecated on 08/08/2019, expires on 08/09/2020]
    /// </summary>
    struct Header1
    {
        AudioFormat Format;
        AudioDataInfo Info;
        bool Is3D;
        bool Streamable;
        uint32 OriginalSize;
        uint32 ImportedSize;
    };

    typedef AudioClip::Header Header2;

    static bool Upgrade_1_To_2(AssetMigrationContext& context)
    {
        ASSERT(context.Input.SerializedVersion == 1 && context.Output.SerializedVersion == 2);

        // Copy all chunks
        if (CopyChunks(context))
            return true;

        // Convert header
        if (context.Input.CustomData.IsInvalid())
            return true;
        auto& oldHeader = *(Header1*)context.Input.CustomData.Get();
        Header2 newHeader;
        newHeader.Format = oldHeader.Format;
        newHeader.Info = oldHeader.Info;
        newHeader.Is3D = oldHeader.Is3D;
        newHeader.Streamable = oldHeader.Streamable;
        newHeader.OriginalSize = oldHeader.OriginalSize;
        newHeader.ImportedSize = oldHeader.ImportedSize;
        Platform::MemoryClear(newHeader.SamplesPerChunk, sizeof(newHeader.SamplesPerChunk));
        for (int32 chunkIndex = 0; chunkIndex < ASSET_FILE_DATA_CHUNKS; chunkIndex++)
        {
            const auto chunk = context.Output.Header.Chunks[chunkIndex];
            if (!chunk)
                continue;

            int32 numSamples;
            switch (oldHeader.Format)
            {
            case AudioFormat::Raw:
            {
                numSamples = chunk->Size() / (oldHeader.Info.BitDepth / 8);
                break;
            }
            case AudioFormat::Vorbis:
            {
#if COMPILE_WITH_OGG_VORBIS
                OggVorbisDecoder decoder;
                MemoryReadStream stream(chunk->Get(), chunk->Size());
                AudioDataInfo outInfo;
                if (!decoder.Open(&stream, outInfo, 0))
                {
                    LOG(Warning, "Audio data open failed (OggVorbisDecoder).");
                    return true;
                }
                numSamples = outInfo.NumSamples;
#else
		            LOG(Warning, "OggVorbisDecoder is disabled.");
		            return true;
#endif
                break;
            }
            default:
                LOG(Warning, "Unknown audio data format.");
                return true;
            }
            newHeader.SamplesPerChunk[chunkIndex] = numSamples;
        }
        context.Output.CustomData.Copy(&newHeader);

        // Validate total amount of samples calculated from the asset chunks
        uint32 totalSamplesInChunks = 0;
        for (int32 chunkIndex = 0; chunkIndex < ASSET_FILE_DATA_CHUNKS; chunkIndex++)
            totalSamplesInChunks += newHeader.SamplesPerChunk[chunkIndex];
        if (totalSamplesInChunks != newHeader.Info.NumSamples)
        {
            LOG(Warning, "Invalid amount of audio data samples in data chunks {0}, audio clip has: {1}.", totalSamplesInChunks, newHeader.Info.NumSamples);
            return true;
        }

        return false;
    }
};

#endif
