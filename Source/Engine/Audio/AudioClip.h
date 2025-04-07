// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/BinaryAsset.h"
#include "Engine/Streaming/StreamableResource.h"
#include "Engine/Content/WeakAssetReference.h"
#include "Engine/Threading/ThreadPoolTask.h"
#include "Types.h"
#include "Config.h"

class AudioSource;

/// <summary>
/// Audio clip stores audio data in a compressed or uncompressed format using a binary asset. Clips can be provided to audio sources or other audio methods to be played.
/// </summary>
/// <seealso cref="BinaryAsset" />
API_CLASS(NoSpawn) class FLAXENGINE_API AudioClip : public BinaryAsset, public StreamableResource
{
    DECLARE_BINARY_ASSET_HEADER(AudioClip, 2);

public:
    /// <summary>
    /// Audio Clip resource header structure, version 2. Added on 08.08.2019.
    /// </summary>
    struct Header
    {
        AudioFormat Format;
        AudioDataInfo Info;
        bool Is3D;
        bool Streamable;
        uint32 OriginalSize;
        uint32 ImportedSize;
        uint32 SamplesPerChunk[ASSET_FILE_DATA_CHUNKS]; // Amount of audio samples (for all channels) stored per asset chunk (uncompressed data samples count)
    };

    /// <summary>
    /// Audio clip streaming task
    /// </summary>
    class StreamingTask : public ThreadPoolTask
    {
    private:
        WeakAssetReference<AudioClip> _asset;
        FlaxStorage::LockData _dataLock;

    public:
        /// <summary>
        /// Init
        /// </summary>
        /// <param name="asset">Parent asset</param>
        StreamingTask(AudioClip* asset)
            : _asset(asset)
            , _dataLock(asset->Storage->Lock())
        {
        }

    public:
        // [ThreadPoolTask]
        bool HasReference(Object* resource) const override
        {
            return _asset == resource;
        }

    protected:
        // [ThreadPoolTask]
        bool Run() override;
        void OnEnd() override;
    };

private:
    int32 _totalChunks;
    int32 _totalChunksSize;
    StreamingTask* _streamingTask;
    float _buffersStartTimes[ASSET_FILE_DATA_CHUNKS + 1];

public:
    /// <summary>
    /// Finalizes an instance of the <see cref="AudioClip"/> class.
    /// </summary>
    ~AudioClip();

public:
    /// <summary>
    /// The audio clip header data.
    /// </summary>
    Header AudioHeader;

    /// <summary>
    /// The audio backend buffers (internal ids) collection used by this audio clip.
    /// </summary>
    Array<uint32, FixedAllocation<ASSET_FILE_DATA_CHUNKS>> Buffers;

    /// <summary>
    /// The streaming cache. Contains indices of chunks to stream. If empty no streaming required. Managed by AudioStreamingHandler and used by the Audio streaming tasks.
    /// </summary>
    Array<int32, FixedAllocation<ASSET_FILE_DATA_CHUNKS>> StreamingQueue;

public:
    /// <summary>
    /// Gets the audio data format.
    /// </summary>
    API_PROPERTY() FORCE_INLINE AudioFormat Format() const
    {
        return AudioHeader.Format;
    }

    /// <summary>
    /// Gets the audio data info metadata.
    /// </summary>
    API_PROPERTY() FORCE_INLINE const AudioDataInfo& Info() const
    {
        return AudioHeader.Info;
    }

    /// <summary>
    /// Returns true if the sound source is three dimensional (volume and pitch varies based on listener distance and velocity).
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool Is3D() const
    {
        return AudioHeader.Is3D;
    }

    /// <summary>
    /// Returns true if the sound is using data streaming.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsStreamable() const
    {
        return AudioHeader.Streamable;
    }

    /// <summary>
    /// Returns true if the sound data is during streaming by an async task.
    /// </summary>
    API_PROPERTY() FORCE_INLINE bool IsStreamingTaskActive() const
    {
        return _streamingTask != nullptr;
    }

    /// <summary>
    /// Gets the length of the audio clip (in seconds).
    /// </summary>
    API_PROPERTY() float GetLength() const
    {
        return AudioHeader.Info.GetLength();
    }

public:
    /// <summary>
    /// Gets the buffer start time (in seconds).
    /// </summary>
    /// <param name="bufferIndex">Index of the buffer.</param>
    /// <returns>The buffer start time.</returns>
    float GetBufferStartTime(int32 bufferIndex) const;

    /// <summary>
    /// Gets the index of the first buffer to bind for the audio source data streaming.
    /// </summary>
    /// <param name="time">The time (in seconds).</param>
    /// <param name="offset">The offset time from the chunk start time (in seconds).</param>
    /// <returns>The buffer index.</returns>
    int32 GetFirstBufferIndex(float time, float& offset) const;

public:
    /// <summary>
    /// Extracts the source audio data from the asset storage. Loads the whole asset. The result data is in an asset format.
    /// </summary>
    /// <param name="resultData">The result data.</param>
    /// <param name="resultDataInfo">The result data format header info.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool ExtractData(API_PARAM(Out) Array<byte>& resultData, API_PARAM(Out) AudioDataInfo& resultDataInfo);

    /// <summary>
    /// Extracts the raw audio data (PCM format) from the asset storage and converts it to the normalized float format (in range [-1;1]). Loads the whole asset.
    /// </summary>
    /// <param name="resultData">The result data.</param>
    /// <param name="resultDataInfo">The result data format header info. That output data has 32 bits float data not the signed PCM data.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool ExtractDataFloat(API_PARAM(Out) Array<float>& resultData, API_PARAM(Out) AudioDataInfo& resultDataInfo);

    /// <summary>
    /// Extracts the raw audio data (PCM format) from the asset storage. Loads the whole asset.
    /// </summary>
    /// <param name="resultData">The result data.</param>
    /// <param name="resultDataInfo">The result data format header info.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() bool ExtractDataRaw(API_PARAM(Out) Array<byte>& resultData, API_PARAM(Out) AudioDataInfo& resultDataInfo);

public:
    // [BinaryAsset]
    void CancelStreaming() override;

    // [StreamableResource]
    int32 GetMaxResidency() const override;
    int32 GetCurrentResidency() const override;
    int32 GetAllocatedResidency() const override;
    bool CanBeUpdated() const override;
    Task* UpdateAllocation(int32 residency) override;
    Task* CreateStreamingTask(int32 residency) override;
    void CancelStreamingTasks() override;

protected:
    // [BinaryAsset]
    bool init(AssetInitData& initData) override;
    LoadResult load() override;
    void unload(bool isReloading) override;

private:
    // Writes audio samples into Audio Backend buffer and handles automatic decompression or format conversion for runtime playback.
    bool WriteBuffer(int32 chunkIndex);
};
