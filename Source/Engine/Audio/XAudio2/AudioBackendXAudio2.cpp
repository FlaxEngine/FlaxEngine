// Copyright (c) Wojciech Figat. All rights reserved.

#if AUDIO_API_XAUDIO2

#include "AudioBackendXAudio2.h"
#include "Engine/Audio/AudioBackendTools.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/ChunkedArray.h"
#include "Engine/Core/Log.h"
#include "Engine/Audio/Audio.h"
#include "Engine/Threading/Threading.h"

#if PLATFORM_WINDOWS
// Tweak Win ver
#define _WIN32_WINNT 0x0602
//#include "Engine/Platform/Windows/IncludeWindowsHeaders.h"
#endif

// Include XAudio library
// Documentation: https://docs.microsoft.com/en-us/windows/desktop/xaudio2/xaudio2-apis-portal
#include <xaudio2.h>

// TODO: implement multi-channel support (eg. 5.1, 7.1)
#define MAX_INPUT_CHANNELS 6
#define MAX_OUTPUT_CHANNELS 2
#define MAX_CHANNELS_MATRIX_SIZE (MAX_INPUT_CHANNELS*MAX_OUTPUT_CHANNELS)
#if ENABLE_ASSERTION
#define XAUDIO2_CHECK_ERROR(method) \
    if (hr != 0) \
    { \
        LOG(Error, "XAudio2 method {0} failed with error 0x{1:X} (at line {2})", TEXT(#method), (uint32)hr, __LINE__ - 1); \
    }
#else
#define XAUDIO2_CHECK_ERROR(method)
#endif

namespace XAudio2
{
    struct Listener : AudioBackendTools::Listener
    {
    };

    class VoiceCallback : public IXAudio2VoiceCallback
    {
        COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE OnVoiceProcessingPassStart(THIS_ UINT32 BytesRequired) override
        {
        }

        COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE OnVoiceProcessingPassEnd(THIS) override
        {
        }

        COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE OnStreamEnd(THIS_) override
        {
        }

        COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE OnBufferStart(THIS_ void* pBufferContext) override
        {
            PeekSamples();
        }

        COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE OnBufferEnd(THIS_ void* pBufferContext) override;

        COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE OnLoopEnd(THIS_ void* pBufferContext) override
        {
        }

        COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE OnVoiceError(THIS_ void* pBufferContext, HRESULT Error) override
        {
#if ENABLE_ASSERTION
            LOG(Warning, "IXAudio2VoiceCallback::OnVoiceError! Error: 0x{0:x}", Error);
#endif
        }

    public:
        uint32 SourceID;

        void PeekSamples();
    };

    struct Source : AudioBackendTools::Source
    {
        IXAudio2SourceVoice* Voice;
        WAVEFORMATEX Format;
        AudioDataInfo Info;
        XAUDIO2_SEND_DESCRIPTOR Destination;
        float StartTimeForQueueBuffer;
        float LastBufferStartTime;
        uint64 LastBufferStartSamplesPlayed;
        int32 BuffersProcessed;
        int32 Channels;
        bool IsDirty;
        bool IsPlaying;
        bool IsLoop;
        uint32 LastBufferID;
        VoiceCallback Callback;

        Source()
        {
            Init();
        }

        void Init()
        {
            Voice = nullptr;
            Destination.Flags = 0;
            Destination.pOutputVoice = nullptr;
            Pitch = 1.0f;
            Pan = 0.0f;
            StartTimeForQueueBuffer = 0.0f;
            LastBufferStartTime = 0.0f;
            IsDirty = false;
            Is3D = false;
            IsPlaying = false;
            IsLoop = false;
            LastBufferID = 0;
            LastBufferStartSamplesPlayed = 0;
            BuffersProcessed = 0;
        }

        bool IsFree() const
        {
            return Voice == nullptr;
        }
    };

    struct Buffer
    {
        AudioDataInfo Info;
        Array<byte> Data;
    };

    class EngineCallback : public IXAudio2EngineCallback
    {
        COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE OnProcessingPassEnd() override
        {
        }

        COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE OnProcessingPassStart() override
        {
        }

        COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE OnCriticalError(HRESULT error) override
        {
            LOG(Warning, "IXAudio2EngineCallback::OnCriticalError! Error: 0x{0:x}", error);
        }
    };

    IXAudio2* Instance = nullptr;
    IXAudio2MasteringVoice* MasteringVoice = nullptr;
    int32 Channels;
    DWORD ChannelMask;
    bool ForceDirty = true;
    AudioBackendTools::Settings Settings;
    Listener Listener;
    CriticalSection Locker;
    ChunkedArray<Source, 32> Sources;
    ChunkedArray<Buffer*, 64> Buffers; // TODO: use ChunkedArray for better performance or use buffers pool?
    EngineCallback Callback;

    Source* GetSource(uint32 sourceID)
    {
        if (sourceID == 0)
            return nullptr;
        return &Sources[sourceID - 1]; // 0 is invalid ID so shift them
    }

    void MarkAllDirty()
    {
        ForceDirty = true;
    }

    void QueueBuffer(Source* aSource, const int32 bufferID, XAUDIO2_BUFFER& buffer)
    {
        Buffer* aBuffer = Buffers[bufferID - 1];
        buffer.pAudioData = aBuffer->Data.Get();
        buffer.AudioBytes = aBuffer->Data.Count();

        if (aSource->StartTimeForQueueBuffer > ZeroTolerance)
        {
            // Offset start position when playing buffer with a custom time offset
            const uint32 bytesPerSample = aBuffer->Info.BitDepth / 8 * aBuffer->Info.NumChannels;
            buffer.PlayBegin = (UINT32)(aSource->StartTimeForQueueBuffer * aBuffer->Info.SampleRate);
            buffer.PlayLength = (buffer.AudioBytes / bytesPerSample) - buffer.PlayBegin;
            aSource->LastBufferStartTime = aSource->StartTimeForQueueBuffer;
            aSource->StartTimeForQueueBuffer = 0;
        }

        const HRESULT hr = aSource->Voice->SubmitSourceBuffer(&buffer);
        XAUDIO2_CHECK_ERROR(SubmitSourceBuffer);
    }

    void VoiceCallback::OnBufferEnd(void* pBufferContext)
    {
        auto aSource = GetSource(SourceID);
        if (aSource->IsPlaying)
            aSource->BuffersProcessed++;
    }

    void VoiceCallback::PeekSamples()
    {
        auto aSource = GetSource(SourceID);
        XAUDIO2_VOICE_STATE state;
        aSource->Voice->GetState(&state);
        aSource->LastBufferStartSamplesPlayed = state.SamplesPlayed;
    }
}

void AudioBackendXAudio2::Listener_Reset()
{
    XAudio2::Listener.Reset();
    XAudio2::MarkAllDirty();
}

void AudioBackendXAudio2::Listener_VelocityChanged(const Vector3& velocity)
{
    XAudio2::Listener.Velocity = velocity;
    XAudio2::MarkAllDirty();
}

void AudioBackendXAudio2::Listener_TransformChanged(const Vector3& position, const Quaternion& orientation)
{
    XAudio2::Listener.Position = position;
    XAudio2::Listener.Orientation = orientation;
    XAudio2::MarkAllDirty();
}

void AudioBackendXAudio2::Listener_ReinitializeAll()
{
    // TODO: Implement XAudio2 reinitialization; read HRTF audio value from Audio class
}

uint32 AudioBackendXAudio2::Source_Add(const AudioDataInfo& format, const Vector3& position, const Quaternion& orientation, float volume, float pitch, float pan, bool loop, bool spatial, float attenuation, float minDistance, float doppler)
{
    ScopeLock lock(XAudio2::Locker);

    // Get first free source
    XAudio2::Source* aSource = nullptr;
    uint32 sourceID = 0;
    for (int32 i = 0; i < XAudio2::Sources.Count(); i++)
    {
        if (XAudio2::Sources[i].IsFree())
        {
            sourceID = i;
            aSource = &XAudio2::Sources[i];
            break;
        }
    }
    if (aSource == nullptr)
    {
        // Add new
        const XAudio2::Source src;
        sourceID = XAudio2::Sources.Count();
        XAudio2::Sources.Add(src);
        aSource = &XAudio2::Sources[sourceID];
    }
    sourceID++; // 0 is invalid ID so shift them

    // Initialize audio data format information (from clip)
    aSource->Info = format;
    auto& aFormat = aSource->Format;
    aFormat.wFormatTag = WAVE_FORMAT_PCM;
    aFormat.nChannels = spatial ? 1 : format.NumChannels; // 3d audio is always mono (AudioClip auto-converts before buffer write if FeatureFlags::SpatialMultiChannel is unset)
    aFormat.nSamplesPerSec = format.SampleRate;
    aFormat.wBitsPerSample = format.BitDepth;
    aFormat.nBlockAlign = (WORD)(aFormat.nChannels * (aFormat.wBitsPerSample / 8));
    aFormat.nAvgBytesPerSec = aFormat.nSamplesPerSec * aFormat.nBlockAlign;
    aFormat.cbSize = 0;

    // Setup dry effect
    aSource->Destination.pOutputVoice = XAudio2::MasteringVoice;

    // Create voice
    const XAUDIO2_VOICE_SENDS sendList = { 1, &aSource->Destination };
    HRESULT hr = XAudio2::Instance->CreateSourceVoice(&aSource->Voice, &aSource->Format, 0, 2.0f, &aSource->Callback, &sendList);
    XAUDIO2_CHECK_ERROR(CreateSourceVoice);
    if (FAILED(hr))
        return 0;

    // Prepare source state
    aSource->Callback.SourceID = sourceID;
    aSource->IsDirty = true;
    aSource->IsLoop = loop;
    aSource->Is3D = spatial;
    aSource->Pitch = pitch;
    aSource->Pan = pan;
    aSource->DopplerFactor = doppler;
    aSource->Volume = volume;
    aSource->MinDistance = minDistance;
    aSource->Attenuation = attenuation;
    aSource->Channels = aFormat.nChannels;
    aSource->Position = position;
    aSource->Orientation = orientation;
    aSource->Velocity = Vector3::Zero;
    hr = aSource->Voice->SetVolume(volume);
    XAUDIO2_CHECK_ERROR(SetVolume);

    return sourceID;
}

void AudioBackendXAudio2::Source_Remove(uint32 sourceID)
{
    ScopeLock lock(XAudio2::Locker);
    auto aSource = XAudio2::GetSource(sourceID);
    if (!aSource)
        return;

    // Free source
    if (aSource->Voice)
    {
        aSource->Voice->DestroyVoice();
    }
    aSource->Init();
}

void AudioBackendXAudio2::Source_VelocityChanged(uint32 sourceID, const Vector3& velocity)
{
    auto aSource = XAudio2::GetSource(sourceID);
    if (aSource)
    {
        aSource->Velocity = velocity;
        aSource->IsDirty = true;
    }
}

void AudioBackendXAudio2::Source_TransformChanged(uint32 sourceID, const Vector3& position, const Quaternion& orientation)
{
    auto aSource = XAudio2::GetSource(sourceID);
    if (aSource)
    {
        aSource->Position = position;
        aSource->Orientation = orientation;
        aSource->IsDirty = true;
    }
}

void AudioBackendXAudio2::Source_VolumeChanged(uint32 sourceID, float volume)
{
    auto aSource = XAudio2::GetSource(sourceID);
    if (aSource && aSource->Voice)
    {
        aSource->Volume = volume;
        const HRESULT hr = aSource->Voice->SetVolume(volume);
        XAUDIO2_CHECK_ERROR(SetVolume);
    }
}

void AudioBackendXAudio2::Source_PitchChanged(uint32 sourceID, float pitch)
{
    auto aSource = XAudio2::GetSource(sourceID);
    if (aSource)
    {
        aSource->Pitch = pitch;
        aSource->IsDirty = true;
    }
}

void AudioBackendXAudio2::Source_PanChanged(uint32 sourceID, float pan)
{
    auto aSource = XAudio2::GetSource(sourceID);
    if (aSource)
    {
        aSource->Pan = pan;
        aSource->IsDirty = true;
    }
}

void AudioBackendXAudio2::Source_IsLoopingChanged(uint32 sourceID, bool loop)
{
    ScopeLock lock(XAudio2::Locker);
    auto aSource = XAudio2::GetSource(sourceID);
    if (!aSource || !aSource->Voice)
        return;
    aSource->IsLoop = loop;

    // Skip if has no buffers (waiting for data or sth)
    XAUDIO2_VOICE_STATE state;
    aSource->Voice->GetState(&state);
    if (state.BuffersQueued == 0)
        return;

    // Looping is defined during buffer submission so reset source buffer (this is called only for non-streamable sources that use a single buffer)
    const uint32 bufferID = aSource->LastBufferID;
    XAudio2::Buffer* aBuffer = XAudio2::Buffers[bufferID - 1];

    HRESULT hr;
    const bool isPlaying = aSource->IsPlaying;
    if (isPlaying)
    {
        hr = aSource->Voice->Stop();
        XAUDIO2_CHECK_ERROR(Stop);
    }

    hr = aSource->Voice->FlushSourceBuffers();
    XAUDIO2_CHECK_ERROR(FlushSourceBuffers);
    aSource->LastBufferStartSamplesPlayed = 0;
    aSource->LastBufferStartTime = 0;
    aSource->BuffersProcessed = 0;

    XAUDIO2_BUFFER buffer = { 0 };
    buffer.pContext = aBuffer;
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    if (loop)
        buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

    // Restore play position
    const UINT32 totalSamples = aBuffer->Info.NumSamples / aBuffer->Info.NumChannels;
    buffer.PlayBegin = state.SamplesPlayed % totalSamples;
    buffer.PlayLength = totalSamples - buffer.PlayBegin;
    aSource->StartTimeForQueueBuffer = 0;

    XAudio2::QueueBuffer(aSource, bufferID, buffer);

    if (isPlaying)
    {
        hr = aSource->Voice->Start();
        XAUDIO2_CHECK_ERROR(Start);
    }
}

void AudioBackendXAudio2::Source_SpatialSetupChanged(uint32 sourceID, bool spatial, float attenuation, float minDistance, float doppler)
{
    auto aSource = XAudio2::GetSource(sourceID);
    if (aSource)
    {
        aSource->Is3D = spatial;
        aSource->MinDistance = minDistance;
        aSource->Attenuation = attenuation;
        aSource->DopplerFactor = doppler;
        aSource->IsDirty = true;
    }
}

void AudioBackendXAudio2::Source_Play(uint32 sourceID)
{
    auto aSource = XAudio2::GetSource(sourceID);
    if (aSource && aSource->Voice && !aSource->IsPlaying)
    {
        // Play
        const HRESULT hr = aSource->Voice->Start();
        XAUDIO2_CHECK_ERROR(Start);
        aSource->IsPlaying = true;
    }
}

void AudioBackendXAudio2::Source_Pause(uint32 sourceID)
{
    auto aSource = XAudio2::GetSource(sourceID);
    if (aSource && aSource->Voice && aSource->IsPlaying)
    {
        // Pause
        const HRESULT hr = aSource->Voice->Stop();
        XAUDIO2_CHECK_ERROR(Stop);
        aSource->IsPlaying = false;
    }
}

void AudioBackendXAudio2::Source_Stop(uint32 sourceID)
{
    auto aSource = XAudio2::GetSource(sourceID);
    if (aSource && aSource->Voice)
    {
        aSource->StartTimeForQueueBuffer = 0.0f;
        aSource->LastBufferStartTime = 0.0f;

        // Pause
        HRESULT hr = aSource->Voice->Stop();
        XAUDIO2_CHECK_ERROR(Stop);
        aSource->IsPlaying = false;

        // Unset streaming buffers to rewind
        hr = aSource->Voice->FlushSourceBuffers();
        XAUDIO2_CHECK_ERROR(FlushSourceBuffers);
        Platform::Sleep(10); // TODO: find a better way to handle case when VoiceCallback::OnBufferEnd is called after source was stopped thus BuffersProcessed != 0, probably via buffers contexts ptrs
        aSource->BuffersProcessed = 0;
        aSource->Callback.PeekSamples();
    }
}

void AudioBackendXAudio2::Source_SetCurrentBufferTime(uint32 sourceID, float value)
{
    const auto aSource = XAudio2::GetSource(sourceID);
    if (aSource)
    {
        // Store start time so next buffer submitted will start from here (assumes audio is stopped)
        aSource->StartTimeForQueueBuffer = value;
    }
}

float AudioBackendXAudio2::Source_GetCurrentBufferTime(uint32 sourceID)
{
    float time = 0;
    auto aSource = XAudio2::GetSource(sourceID);
    if (aSource)
    {
        const auto& clipInfo = aSource->Info;
        XAUDIO2_VOICE_STATE state;
        aSource->Voice->GetState(&state);
        const uint32 totalSamples = clipInfo.NumSamples / clipInfo.NumChannels;
        const uint32 sampleRate = clipInfo.SampleRate; // / clipInfo.NumChannels;
        uint64 lastBufferStartSamplesPlayed = aSource->LastBufferStartSamplesPlayed;
        if (totalSamples > 0)
            lastBufferStartSamplesPlayed %= totalSamples;
        state.SamplesPlayed -= lastBufferStartSamplesPlayed % totalSamples; // Offset by the last buffer start to get time relative to its begin
        if (totalSamples > 0)
            state.SamplesPlayed %= totalSamples;
        time = aSource->LastBufferStartTime + state.SamplesPlayed / static_cast<float>(Math::Max(1U, sampleRate));
    }
    return time;
}

void AudioBackendXAudio2::Source_SetNonStreamingBuffer(uint32 sourceID, uint32 bufferID)
{
    auto aSource = XAudio2::GetSource(sourceID);
    if (!aSource)
        return;
    aSource->LastBufferID = bufferID; // Use for looping change

    XAudio2::Locker.Lock();
    XAudio2::Buffer* aBuffer = XAudio2::Buffers[bufferID - 1];
    XAudio2::Locker.Unlock();

    XAUDIO2_BUFFER buffer = { 0 };
    buffer.pContext = aBuffer;
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    if (aSource->IsLoop)
        buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

    // Queue single buffer
    XAudio2::QueueBuffer(aSource, bufferID, buffer);
}

void AudioBackendXAudio2::Source_GetProcessedBuffersCount(uint32 sourceID, int32& processedBuffersCount)
{
    processedBuffersCount = 0;
    auto aSource = XAudio2::GetSource(sourceID);
    if (aSource && aSource->Voice)
    {
        processedBuffersCount = aSource->BuffersProcessed;
    }
}

void AudioBackendXAudio2::Source_GetQueuedBuffersCount(uint32 sourceID, int32& queuedBuffersCount)
{
    queuedBuffersCount = 0;
    auto aSource = XAudio2::GetSource(sourceID);
    if (aSource && aSource->Voice)
    {
        XAUDIO2_VOICE_STATE state;
        aSource->Voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
        queuedBuffersCount = state.BuffersQueued;
    }
}

void AudioBackendXAudio2::Source_QueueBuffer(uint32 sourceID, uint32 bufferID)
{
    auto aSource = XAudio2::GetSource(sourceID);
    if (!aSource)
        return;
    aSource->LastBufferID = bufferID; // Use for looping change

    XAudio2::Buffer* aBuffer = XAudio2::Buffers[bufferID - 1];

    XAUDIO2_BUFFER buffer = { 0 };
    buffer.pContext = aBuffer;

    XAudio2::QueueBuffer(aSource, bufferID, buffer);
}

void AudioBackendXAudio2::Source_DequeueProcessedBuffers(uint32 sourceID)
{
    auto aSource = XAudio2::GetSource(sourceID);
    if (aSource && aSource->Voice)
    {
        const HRESULT hr = aSource->Voice->FlushSourceBuffers();
        XAUDIO2_CHECK_ERROR(FlushSourceBuffers);
        aSource->BuffersProcessed = 0;
    }
}

uint32 AudioBackendXAudio2::Buffer_Create()
{
    uint32 bufferID;
    ScopeLock lock(XAudio2::Locker);

    // Get first free buffer slot
    XAudio2::Buffer* aBuffer = nullptr;
    for (int32 i = 0; i < XAudio2::Buffers.Count(); i++)
    {
        if (XAudio2::Buffers[i] == nullptr)
        {
            aBuffer = New<XAudio2::Buffer>();
            XAudio2::Buffers[i] = aBuffer;
            bufferID = i + 1;
            break;
        }
    }
    if (!aBuffer)
    {
        // Add new slot
        aBuffer = New<XAudio2::Buffer>();
        XAudio2::Buffers.Add(aBuffer);
        bufferID = XAudio2::Buffers.Count();
    }

    aBuffer->Data.Resize(0);
    return bufferID;
}

void AudioBackendXAudio2::Buffer_Delete(uint32 bufferID)
{
    ScopeLock lock(XAudio2::Locker);
    XAudio2::Buffer*& aBuffer = XAudio2::Buffers[bufferID - 1];
    aBuffer->Data.Resize(0);
    Delete(aBuffer);
    aBuffer = nullptr;
}

void AudioBackendXAudio2::Buffer_Write(uint32 bufferID, byte* samples, const AudioDataInfo& info)
{
    CHECK(info.NumChannels <= MAX_INPUT_CHANNELS);

    XAudio2::Locker.Lock();
    XAudio2::Buffer* aBuffer = XAudio2::Buffers[bufferID - 1];
    XAudio2::Locker.Unlock();

    const uint32 samplesLength = info.NumSamples * info.BitDepth / 8;

    aBuffer->Info = info;
    aBuffer->Data.Set(samples, samplesLength);
}

const Char* AudioBackendXAudio2::Base_Name()
{
    return TEXT("XAudio2");
}

AudioBackend::FeatureFlags AudioBackendXAudio2::Base_Features()
{
    return FeatureFlags::None;
}

void AudioBackendXAudio2::Base_OnActiveDeviceChanged()
{
}

void AudioBackendXAudio2::Base_SetDopplerFactor(float value)
{
    XAudio2::Settings.DopplerFactor = value;
    XAudio2::MarkAllDirty();
}

void AudioBackendXAudio2::Base_SetVolume(float value)
{
    if (XAudio2::MasteringVoice)
    {
        XAudio2::Settings.Volume = 1.0f; // Volume is applied via MasteringVoice
        const HRESULT hr = XAudio2::MasteringVoice->SetVolume(value);
        XAUDIO2_CHECK_ERROR(SetVolume);
    }
}

bool AudioBackendXAudio2::Base_Init()
{
    auto& devices = Audio::Devices;

    // Initialize XAudio backend
    HRESULT hr = XAudio2Create(&XAudio2::Instance, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr))
    {
        LOG(Error, "Failed to initalize XAudio2. Error: 0x{0:x}", hr);
        return true;
    }
    XAudio2::Instance->RegisterForCallbacks(&XAudio2::Callback);

    // Initialize master voice
    hr = XAudio2::Instance->CreateMasteringVoice(&XAudio2::MasteringVoice);
    if (FAILED(hr))
    {
        LOG(Error, "Failed to initalize XAudio2 mastering voice. Error: 0x{0:x}", hr);
        return true;
    }
    XAUDIO2_VOICE_DETAILS details;
    XAudio2::MasteringVoice->GetVoiceDetails(&details);
#if MAX_OUTPUT_CHANNELS > 2
    // TODO: implement multi-channel support (eg. 5.1, 7.1)
    XAudio2::Channels = details.InputChannels;
    hr = XAudio2::MasteringVoice->GetChannelMask(&XAudio2::ChannelMask);
    if (FAILED(hr))
    {
        LOG(Error, "Failed to get XAudio2 mastering voice channel mask. Error: 0x{0:x}", hr);
        return true;
    }
#else
    XAudio2::Channels = 2;
    XAudio2::ChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
#endif
    LOG(Info, "XAudio2: {0} channels at {1} kHz", XAudio2::Channels, details.InputSampleRate / 1000.0f);

    // Dummy device
    devices.Resize(1);
    devices[0].Name = TEXT("XAudio2 device");
    Audio::SetActiveDeviceIndex(0);

    return false;
}

void AudioBackendXAudio2::Base_Update()
{
    // Update dirty voices
    float outputMatrix[MAX_CHANNELS_MATRIX_SIZE];
    for (int32 i = 0; i < XAudio2::Sources.Count(); i++)
    {
        auto& source = XAudio2::Sources[i];
        if (source.IsFree() || !(source.IsDirty || XAudio2::ForceDirty))
            continue;

        auto mix = AudioBackendTools::CalculateSoundMix(XAudio2::Settings, XAudio2::Listener, source, XAudio2::Channels);
        mix.VolumeIntoChannels();
        AudioBackendTools::MapChannels(source.Channels, XAudio2::Channels, mix.Channels, outputMatrix);

        source.Voice->SetFrequencyRatio(mix.Pitch);
        source.Voice->SetOutputMatrix(XAudio2::MasteringVoice, source.Channels, XAudio2::Channels, outputMatrix);

        source.IsDirty = false;
    }

    // Clear flag
    XAudio2::ForceDirty = false;
}

void AudioBackendXAudio2::Base_Dispose()
{
    // Cleanup stuff
    if (XAudio2::MasteringVoice)
    {
        XAudio2::MasteringVoice->DestroyVoice();
        XAudio2::MasteringVoice = nullptr;
    }
    if (XAudio2::Instance)
    {
        XAudio2::Instance->StopEngine();
        XAudio2::Instance->Release();
        XAudio2::Instance = nullptr;
    }
}

#endif
