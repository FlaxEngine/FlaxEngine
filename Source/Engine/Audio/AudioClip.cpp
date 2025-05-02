// Copyright (c) Wojciech Figat. All rights reserved.

#include "AudioClip.h"
#include "Audio.h"
#include "AudioSource.h"
#include "AudioBackend.h"
#include "Engine/Core/Log.h"
#include "Engine/Content/Upgraders/AudioClipUpgrader.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Streaming/StreamingGroup.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Tools/AudioTool/OggVorbisDecoder.h"
#include "Engine/Tools/AudioTool/AudioTool.h"
#include "Engine/Threading/Threading.h"

REGISTER_BINARY_ASSET_WITH_UPGRADER(AudioClip, "FlaxEngine.AudioClip", AudioClipUpgrader, false);

bool AudioClip::StreamingTask::Run()
{
    AssetReference<AudioClip> ref = _asset.Get();
    if (ref == nullptr || AudioBackend::Instance == nullptr)
        return true;
    ScopeLock lock(ref->Locker);
    const auto& queue = ref->StreamingQueue;
    if (queue.Count() == 0)
        return false;
    auto clip = ref.Get();

    // Update the buffers
    for (int32 i = 0; i < queue.Count(); i++)
    {
        const auto idx = queue[i];
        uint32& bufferID = clip->Buffers[idx];
        if (bufferID == 0)
        {
            bufferID = AudioBackend::Buffer::Create();
        }
        else
        {
            // Release unused data
            AudioBackend::Buffer::Delete(bufferID);
            bufferID = 0;
        }
    }

    // Load missing buffers data (from asset chunks)
    for (int32 i = 0; i < queue.Count(); i++)
    {
        if (clip->WriteBuffer(queue[i]))
        {
            return true;
        }
    }

    // Update the sources
    for (int32 sourceIndex = 0; sourceIndex < Audio::Sources.Count(); sourceIndex++)
    {
        // TODO: collect refs to audio clip from sources and use faster iteration (but do it thread-safe)
        const auto src = Audio::Sources[sourceIndex];
        if (src->Clip == clip && src->GetState() == AudioSource::States::Playing)
        {
            src->RequestStreamingBuffersUpdate();
        }
    }

    return false;
}

void AudioClip::StreamingTask::OnEnd()
{
    // Unlink
    if (_asset)
    {
        ScopeLock lock(_asset->Locker);
        ASSERT(_asset->_streamingTask == this);
        _asset->_streamingTask = nullptr;
        _asset = nullptr;
    }
    _dataLock.Release();

    // Base
    ThreadPoolTask::OnEnd();
}

AudioClip::AudioClip(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
    , StreamableResource(StreamingGroups::Instance()->Audio())
    , _totalChunks(0)
    , _totalChunksSize(0)
    , _streamingTask(nullptr)
{
    Platform::MemoryClear(&AudioHeader, sizeof(AudioHeader));
    Platform::MemoryClear(&_buffersStartTimes, sizeof(_buffersStartTimes));
}

AudioClip::~AudioClip()
{
    ASSERT(_streamingTask == nullptr);
}

float AudioClip::GetBufferStartTime(int32 bufferIndex) const
{
    ASSERT(IsLoaded());
    return _buffersStartTimes[bufferIndex];
}

int32 AudioClip::GetFirstBufferIndex(float time, float& offset) const
{
    ASSERT(IsLoaded());
    time = Math::Clamp(time, 0.0f, GetLength());

    for (int32 i = 0; i < _totalChunks; i++)
    {
        if (_buffersStartTimes[i + 1] > time)
        {
            offset = time - _buffersStartTimes[i];
#if BUILD_DEBUG
            ASSERT(Math::Abs(GetBufferStartTime(i) + offset - time) < 0.001f);
#endif
            return i;
        }
    }
    /*
    const float samples = static_cast<float>(AudioHeader.Info.SampleRate * AudioHeader.Info.NumChannels * (AudioHeader.Info.BitDepth / 8));
    // TODO: use _buffersStartTimes
    int32 pos = static_cast<int32>(time * samples);
    for (int32 i = 0; i < _totalChunks; i++)
    {
        const uint32 size = _options.Chunks[i]->LocationInFile.Size;
        pos -= size;
        if (pos <= 0)
        {
            offset = (pos + size) / samples;
#if BUILD_DEBUG
            ASSERT(Math::Abs(GetBufferStartTime(i) + offset - time) < 0.001f);
#endif
            return i;
        }
    }
    */

    offset = 0.0f;
    return 0;
}

bool AudioClip::ExtractData(Array<byte>& resultData, AudioDataInfo& resultDataInfo)
{
    ASSERT(!IsVirtual());
    if (WaitForLoaded())
        return true;
    ScopeLock lock(Locker);
    auto storageLock = Storage->LockSafe();

    // Allocate memory
    ASSERT(_totalChunksSize > 0);
    resultData.Resize(_totalChunksSize);

    // Load and copy data
    if (LoadChunks(ALL_ASSET_CHUNKS))
        return true;
    int32 pos = 0;
    Storage->LockChunks();
    for (int32 i = 0; i < _totalChunks; i++)
    {
        const auto chunk = GetChunk(i);
        const uint32 size = chunk->Size();
        Platform::MemoryCopy(resultData.Get() + pos, chunk->Get(), size);
        pos += size;
    }
    Storage->UnlockChunks();
    ASSERT(pos == _totalChunksSize);

    // Copy header
    resultDataInfo = AudioHeader.Info;

    return false;
}

bool AudioClip::ExtractDataFloat(Array<float>& resultData, AudioDataInfo& resultDataInfo)
{
    // Extract PCM data
    Array<byte> data;
    if (ExtractDataRaw(data, resultDataInfo))
        return true;

    // Convert to float
    resultData.Resize(resultDataInfo.NumSamples);
    AudioTool::ConvertToFloat(data.Get(), resultDataInfo.BitDepth, resultData.Get(), resultDataInfo.NumSamples);
    resultDataInfo.BitDepth = 32;

    return false;
}

bool AudioClip::ExtractDataRaw(Array<byte>& resultData, AudioDataInfo& resultDataInfo)
{
    if (WaitForLoaded())
        return true;
    ScopeLock lock(Locker);
    switch (Format())
    {
    case AudioFormat::Raw:
    {
        return ExtractData(resultData, resultDataInfo);
    }
    case AudioFormat::Vorbis:
    {
#if COMPILE_WITH_OGG_VORBIS
        Array<byte> vorbisData;
        AudioDataInfo vorbisDataInfo;
        if (ExtractData(vorbisData, vorbisDataInfo))
            return true;

        OggVorbisDecoder decoder;
        MemoryReadStream stream(vorbisData.Get(), vorbisData.Count());
        return decoder.Convert(&stream, resultDataInfo, resultData);
#else
		LOG(Warning, "OggVorbisDecoder is disabled.");
		return true;
#endif
    }
    }

    return true;
}

void AudioClip::CancelStreaming()
{
    Asset::CancelStreaming();
    CancelStreamingTasks();
}

int32 AudioClip::GetMaxResidency() const
{
    return _totalChunks;
}

int32 AudioClip::GetCurrentResidency() const
{
    return Buffers.Count();
}

int32 AudioClip::GetAllocatedResidency() const
{
    return Buffers.Count();
}

bool AudioClip::CanBeUpdated() const
{
    // Check if is ready and has no streaming tasks running
    return _totalChunks != 0 && _streamingTask == nullptr;
}

Task* AudioClip::UpdateAllocation(int32 residency)
{
    // Audio clips are not using dynamic allocation feature
    return nullptr;
}

Task* AudioClip::CreateStreamingTask(int32 residency)
{
    ScopeLock lock(Locker);

    ASSERT(_totalChunks != 0 && Math::IsInRange(residency, 0, _totalChunks) && _streamingTask == nullptr);
    Task* result = nullptr;

    // Requests assets chunks data
    for (int32 i = 0; i < StreamingQueue.Count(); i++)
    {
        const int32 idx = StreamingQueue[i];
        if (Buffers[idx] == 0)
        {
            const auto task = (Task*)RequestChunkDataAsync(idx);
            if (task)
            {
                if (result)
                    result->ContinueWith(task);
                else
                    result = task;
            }
        }
    }

    // Add streaming task
    _streamingTask = New<StreamingTask>(this);
    if (result)
        result->ContinueWith(_streamingTask);
    else
        result = _streamingTask;

    return result;
}

void AudioClip::CancelStreamingTasks()
{
    ScopeLock lock(Locker);
    if (_streamingTask)
    {
        _streamingTask->Cancel();
        ASSERT_LOW_LAYER(_streamingTask == nullptr);
    }
}

bool AudioClip::init(AssetInitData& initData)
{
    // Validate
    if (initData.CustomData.Length() != sizeof(AudioHeader))
    {
        LOG(Warning, "Missing audio data.");
        return true;
    }

    // Load header
    Platform::MemoryCopy(&AudioHeader, initData.CustomData.Get(), sizeof(AudioHeader));

    return false;
}

Asset::LoadResult AudioClip::load()
{
#if !COMPILE_WITH_OGG_VORBIS
    if (AudioHeader.Format == AudioFormat::Vorbis)
    {
		LOG(Warning, "OggVorbisDecoder is disabled.");
		return LoadResult::Failed;
    }
#endif

    // Count chunks with an audio data
    _totalChunks = 0;
    while (_totalChunks < ASSET_FILE_DATA_CHUNKS && HasChunk(_totalChunks))
        _totalChunks++;

    // Fill buffers with empty ids
    for (int32 i = 0; i < _totalChunks; i++)
        Buffers.Add(0);

    // Setup the buffers start time (used by the streaming)
    const float scale = 1.0f / static_cast<float>(Math::Max(1U, AudioHeader.Info.SampleRate * AudioHeader.Info.NumChannels));
    _totalChunksSize = AudioHeader.ImportedSize;
    _buffersStartTimes[0] = 0.0f;
    for (int32 i = 0; i < _totalChunks; i++)
    {
        _buffersStartTimes[i + 1] = _buffersStartTimes[i] + AudioHeader.SamplesPerChunk[i] * scale;
    }

#if !BUILD_RELEASE
    // Validate buffer start times
    if (!Math::NearEqual(_buffersStartTimes[_totalChunks], GetLength(), 1.0f / 60.0f))
    {
        LOG(Warning, "Invalid audio buffers data size. Expected length: {0}s", GetLength());
        for (int32 i = 0; i < _totalChunks + 1; i++)
            LOG(Warning, "StartTime[{0}] = {1}s", i, _buffersStartTimes[i]);
        return LoadResult::InvalidData;
    }
#endif

    // Check if use audio streaming
    if (AudioHeader.Streamable)
    {
        // Do nothing because data streaming starts when any AudioSource requests the data
        StartStreaming(false);
        return LoadResult::Ok;
    }

    // Load the whole audio at once
    if (LoadChunk(0))
        return LoadResult::CannotLoadData;

    // Create single buffer
    Buffers[0] = AudioBackend::Buffer::Create();

    // Write data to audio buffer
    if (WriteBuffer(0))
        return LoadResult::Failed;

    return LoadResult::Ok;
}

void AudioClip::unload(bool isReloading)
{
    bool hasAnyBuffer = false;
    for (const uint32 bufferID : Buffers)
        hasAnyBuffer |= bufferID != 0;

    // Stop any audio sources that are using this clip right now
    // TODO: find better way to collect audio sources using audio clip and impl it for AudioStreamingHandler too
    for (int32 sourceIndex = 0; sourceIndex < Audio::Sources.Count(); sourceIndex++)
    {
        const auto src = Audio::Sources[sourceIndex];
        if (src->Clip == this)
            src->Stop();
    }

    StopStreaming();
    CancelStreamingTasks();
    StreamingQueue.Clear();
    if (hasAnyBuffer && AudioBackend::Instance)
    {
        for (uint32 bufferID : Buffers)
        {
            if (bufferID != 0)
                AudioBackend::Buffer::Delete(bufferID);
        }
    }
    Buffers.Clear();
    _totalChunks = 0;
    Platform::MemoryClear(&AudioHeader, sizeof(AudioHeader));
}

bool AudioClip::WriteBuffer(int32 chunkIndex)
{
    // Ignore if buffer is not created
    const uint32 bufferID = Buffers[chunkIndex];
    if (bufferID == 0)
        return false;

    // Ensure audio backend exists
    if (AudioBackend::Instance == nullptr)
        return true;

    const auto chunk = GetChunk(chunkIndex);
    if (chunk == nullptr || chunk->IsMissing())
    {
        LOG(Warning, "Missing audio data.");
        return true;
    }
    Span<byte> data;
    Array<byte> tmp1, tmp2;
    AudioDataInfo info = AudioHeader.Info;
    const uint32 bytesPerSample = info.BitDepth / 8;

    // Get raw data or decompress it
    switch (Format())
    {
    case AudioFormat::Vorbis:
    {
#if COMPILE_WITH_OGG_VORBIS
        OggVorbisDecoder decoder;
        MemoryReadStream stream(chunk->Get(), chunk->Size());
        AudioDataInfo tmpInfo;
        if (decoder.Convert(&stream, tmpInfo, tmp1))
        {
            LOG(Warning, "Audio data decode failed (OggVorbisDecoder).");
            return true;
        }
        // TODO: validate decompressed data header info?
        data = Span<byte>(tmp1.Get(), tmp1.Count());
#else
		LOG(Warning, "OggVorbisDecoder is disabled.");
		return true;
#endif
    }
    break;
    case AudioFormat::Raw:
        data = Span<byte>(chunk->Get(), chunk->Size());
        break;
    default:
        return true;
    }
    info.NumSamples = Math::AlignDown(data.Length() / bytesPerSample, info.NumChannels * bytesPerSample);

    // Convert to Mono if used as 3D source and backend doesn't support it
    if (Is3D() && info.NumChannels > 1 && EnumHasNoneFlags(AudioBackend::Features(), AudioBackend::FeatureFlags::SpatialMultiChannel))
    {
        const uint32 samplesPerChannel = info.NumSamples / info.NumChannels;
        const uint32 monoBufferSize = samplesPerChannel * bytesPerSample;
        tmp2.Resize(monoBufferSize);
        AudioTool::ConvertToMono(data.Get(), tmp2.Get(), info.BitDepth, samplesPerChannel, info.NumChannels);
        info.NumChannels = 1;
        info.NumSamples = samplesPerChannel;
        data = Span<byte>(tmp2.Get(), tmp2.Count());
    }

    // Write samples to the audio buffer
    AudioBackend::Buffer::Write(bufferID, data.Get(), info);
    return false;
}
