// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if AUDIO_API_XAUDIO2

#include "AudioBackendXAudio2.h"
#include "Engine/Audio/AudioSettings.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Collections/ChunkedArray.h"
#include "Engine/Core/Log.h"
#include "Engine/Audio/Audio.h"
#include "Engine/Audio/AudioSource.h"
#include "Engine/Audio/AudioListener.h"
#include "Engine/Threading/Threading.h"

#if PLATFORM_WINDOWS
// Tweak Win ver
#define _WIN32_WINNT 0x0602
//#include "Engine/Platform/Windows/IncludeWindowsHeaders.h"
#endif

// Include XAudio library
// Documentation: https://docs.microsoft.com/en-us/windows/desktop/xaudio2/xaudio2-apis-portal
#include <xaudio2.h>
//#include <xaudio2fx.h>
#include <x3daudio.h>

#define MAX_INPUT_CHANNELS 2
#define MAX_OUTPUT_CHANNELS 8
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
#define FLAX_COORD_SCALE 0.01f // units are meters
#define FLAX_DST_TO_XAUDIO(x) x * FLAX_COORD_SCALE
#define FLAX_POS_TO_XAUDIO(vec) X3DAUDIO_VECTOR(vec.X * FLAX_COORD_SCALE,  vec.Y * FLAX_COORD_SCALE, vec.Z * FLAX_COORD_SCALE)
#define FLAX_VEL_TO_XAUDIO(vec) X3DAUDIO_VECTOR(vec.X * (FLAX_COORD_SCALE*FLAX_COORD_SCALE),  vec.Y * (FLAX_COORD_SCALE*FLAX_COORD_SCALE), vec.Z * (FLAX_COORD_SCALE*FLAX_COORD_SCALE))
#define FLAX_VEC_TO_XAUDIO(vec) (*((X3DAUDIO_VECTOR*)&vec))

namespace XAudio2
{
    struct Listener
    {
        AudioListener* AudioListener;
        X3DAUDIO_LISTENER Data;

        Listener()
        {
            Init();
        }

        void Init()
        {
            AudioListener = nullptr;
            Data.pCone = nullptr;
        }

        bool IsFree() const
        {
            return AudioListener == nullptr;
        }

        void UpdateTransform()
        {
            const Vector3& position = AudioListener->GetPosition();
            const Quaternion& orientation = AudioListener->GetOrientation();
            const Vector3 front = orientation * Vector3::Forward;
            const Vector3 top = orientation * Vector3::Up;

            Data.OrientFront = FLAX_VEC_TO_XAUDIO(front);
            Data.OrientTop = FLAX_VEC_TO_XAUDIO(top);
            Data.Position = FLAX_POS_TO_XAUDIO(position);
        }

        void UpdateVelocity()
        {
            const Vector3& velocity = AudioListener->GetVelocity();

            Data.Velocity = FLAX_VEL_TO_XAUDIO(velocity);
        }
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
        AudioSource* Source;

        void PeekSamples();
    };

    struct Source
    {
        IXAudio2SourceVoice* Voice;
        X3DAUDIO_EMITTER Data;
        WAVEFORMATEX Format;
        XAUDIO2_SEND_DESCRIPTOR Destination;
        float Pitch;
        float Pan;
        float StartTimeForQueueBuffer;
        float LastBufferStartTime;
        float DopplerFactor;
        uint64 LastBufferStartSamplesPlayed;
        int32 BuffersProcessed;
        bool IsDirty;
        bool Is3D;
        bool IsPlaying;
        bool IsForceMono3D;
        VoiceCallback Callback;

        Source()
        {
            Init();
        }

        void Init()
        {
            Voice = nullptr;
            Platform::MemoryClear(&Data, sizeof(Data));
            Data.CurveDistanceScaler = 1.0f;
            Destination.Flags = 0;
            Destination.pOutputVoice = nullptr;
            Pitch = 1.0f;
            Pan = 0.0f;
            StartTimeForQueueBuffer = 0.0f;
            LastBufferStartTime = 0.0f;
            IsDirty = false;
            Is3D = false;
            IsPlaying = false;
            LastBufferStartSamplesPlayed = 0;
            BuffersProcessed = 0;
        }

        bool IsFree() const
        {
            return Voice == nullptr;
        }

        void UpdateTransform(const AudioSource* source)
        {
            const Vector3& position = source->GetPosition();
            const Quaternion& orientation = source->GetOrientation();
            const Vector3 front = orientation * Vector3::Forward;
            const Vector3 top = orientation * Vector3::Up;

            Data.OrientFront = FLAX_VEC_TO_XAUDIO(front);
            Data.OrientTop = FLAX_VEC_TO_XAUDIO(top);
            Data.Position = FLAX_POS_TO_XAUDIO(position);
        }

        void UpdateVelocity(const AudioSource* source)
        {
            const Vector3& velocity = source->GetVelocity();

            Data.Velocity = FLAX_VEL_TO_XAUDIO(velocity);
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
    X3DAUDIO_HANDLE X3DInstance;
    DWORD ChannelMask;
    UINT32 SampleRate;
    UINT32 Channels;
    bool ForceDirty = true;
    Listener Listeners[AUDIO_MAX_LISTENERS];
    CriticalSection Locker;
    ChunkedArray<Source, 32> Sources;
    ChunkedArray<Buffer*, 64> Buffers; // TODO: use ChunkedArray for better performance or use buffers pool?
    EngineCallback Callback;

    Listener* GetListener()
    {
        for (int32 i = 0; i < AUDIO_MAX_LISTENERS; i++)
        {
            if (Listeners[i].AudioListener)
                return &Listeners[i];
        }

        return nullptr;
    }

    Listener* GetListener(const AudioListener* listener)
    {
        for (int32 i = 0; i < AUDIO_MAX_LISTENERS; i++)
        {
            if (Listeners[i].AudioListener == listener)
                return &Listeners[i];
        }

        return nullptr;
    }

    Source* GetSource(const AudioSource* source)
    {
        if (source->SourceIDs.Count() == 0)
            return nullptr;
        const AUDIO_SOURCE_ID_TYPE sourceId = source->SourceIDs[0];
        // 0 is invalid ID so shift them
        return &Sources[sourceId - 1];
    }

    void MarkAllDirty()
    {
        ForceDirty = true;
    }

    void QueueBuffer(Source* aSource, const AudioSource* source, const int32 bufferId, XAUDIO2_BUFFER& buffer)
    {
        Buffer* aBuffer = Buffers[bufferId - 1];
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
        auto aSource = GetSource(Source);
        if (aSource->IsPlaying)
            aSource->BuffersProcessed++;
    }

    void VoiceCallback::PeekSamples()
    {
        auto aSource = GetSource(Source);
        XAUDIO2_VOICE_STATE state;
        aSource->Voice->GetState(&state);
        aSource->LastBufferStartSamplesPlayed = state.SamplesPlayed;
    }
}

void AudioBackendXAudio2::Listener_OnAdd(AudioListener* listener)
{
    // Get first free listener
    XAudio2::Listener* aListener = nullptr;
    for (int32 i = 0; i < AUDIO_MAX_LISTENERS; i++)
    {
        if (XAudio2::Listeners[i].IsFree())
        {
            aListener = &XAudio2::Listeners[i];
            break;
        }
    }
    ASSERT(aListener);

    // Setup
    aListener->AudioListener = listener;
    aListener->UpdateTransform();
    aListener->UpdateVelocity();

    XAudio2::MarkAllDirty();
}

void AudioBackendXAudio2::Listener_OnRemove(AudioListener* listener)
{
    XAudio2::Listener* aListener = XAudio2::GetListener(listener);
    if (aListener)
    {
        aListener->Init();
        XAudio2::MarkAllDirty();
    }
}

void AudioBackendXAudio2::Listener_VelocityChanged(AudioListener* listener)
{
    XAudio2::Listener* aListener = XAudio2::GetListener(listener);
    if (aListener)
    {
        aListener->UpdateVelocity();
        XAudio2::MarkAllDirty();
    }
}

void AudioBackendXAudio2::Listener_TransformChanged(AudioListener* listener)
{
    XAudio2::Listener* aListener = XAudio2::GetListener(listener);
    if (aListener)
    {
        aListener->UpdateTransform();
        XAudio2::MarkAllDirty();
    }
}

void AudioBackendXAudio2::Listener_ReinitializeAll()
{
    // TODO: Implement XAudio2 reinitialization; read HRTF audio value from Audio class
}

void AudioBackendXAudio2::Source_OnAdd(AudioSource* source)
{
    // Skip if has no clip (needs audio data to create a source - needs data format information)
    if (source->Clip == nullptr || !source->Clip->IsLoaded())
        return;
    auto clip = source->Clip.Get();
    ScopeLock lock(XAudio2::Locker);

    // Get first free source
    XAudio2::Source* aSource = nullptr;
    AUDIO_SOURCE_ID_TYPE sourceID;
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

    // Initialize audio data format information (from clip)
    const auto& header = clip->AudioHeader;
    auto& format = aSource->Format;
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = clip->Is3D() ? 1 : header.Info.NumChannels; // 3d audio is always mono (AudioClip auto-converts before buffer write)
    format.nSamplesPerSec = header.Info.SampleRate;
    format.wBitsPerSample = header.Info.BitDepth;
    format.nBlockAlign = (WORD)(format.nChannels * (format.wBitsPerSample / 8));
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    format.cbSize = 0;

    // Setup dry effect
    aSource->Destination.pOutputVoice = XAudio2::MasteringVoice;

    // Create voice
    const XAUDIO2_VOICE_SENDS sendList =
    {
        1,
        &aSource->Destination
    };
    HRESULT hr = XAudio2::Instance->CreateSourceVoice(&aSource->Voice, &aSource->Format, 0, 2.0f, &aSource->Callback, &sendList);
    XAUDIO2_CHECK_ERROR(CreateSourceVoice);
    if (FAILED(hr))
        return;

    // Prepare source state
    aSource->Callback.Source = source;
    aSource->IsDirty = true;
    aSource->Data.ChannelCount = format.nChannels;
    aSource->Data.InnerRadius = FLAX_DST_TO_XAUDIO(source->GetMinDistance());
    aSource->Is3D = source->Is3D();
    aSource->IsForceMono3D = header.Is3D && header.Info.NumChannels > 1;
    aSource->Pitch = source->GetPitch();
    aSource->Pan = source->GetPan();
    aSource->DopplerFactor = source->GetDopplerFactor();
    aSource->UpdateTransform(source);
    aSource->UpdateVelocity(source);
    hr = aSource->Voice->SetVolume(source->GetVolume());
    XAUDIO2_CHECK_ERROR(SetVolume);

    // 0 is invalid ID so shift them
    sourceID++;

    source->SourceIDs.Add(sourceID);

    source->Restore();
}

void AudioBackendXAudio2::Source_OnRemove(AudioSource* source)
{
    ScopeLock lock(XAudio2::Locker);
    source->Cleanup();
}

void AudioBackendXAudio2::Source_VelocityChanged(AudioSource* source)
{
    auto aSource = XAudio2::GetSource(source);
    if (aSource)
    {
        aSource->UpdateVelocity(source);
        aSource->IsDirty = true;
    }
}

void AudioBackendXAudio2::Source_TransformChanged(AudioSource* source)
{
    auto aSource = XAudio2::GetSource(source);
    if (aSource)
    {
        aSource->UpdateTransform(source);
        aSource->IsDirty = true;
    }
}

void AudioBackendXAudio2::Source_VolumeChanged(AudioSource* source)
{
    auto aSource = XAudio2::GetSource(source);
    if (aSource && aSource->Voice)
    {
        const HRESULT hr = aSource->Voice->SetVolume(source->GetVolume());
        XAUDIO2_CHECK_ERROR(SetVolume);
    }
}

void AudioBackendXAudio2::Source_PitchChanged(AudioSource* source)
{
    auto aSource = XAudio2::GetSource(source);
    if (aSource)
    {
        aSource->Pitch = source->GetPitch();
        aSource->IsDirty = true;
    }
}

void AudioBackendXAudio2::Source_PanChanged(AudioSource* source)
{
    auto aSource = XAudio2::GetSource(source);
    if (aSource)
    {
        aSource->Pan = source->GetPan();
        aSource->IsDirty = true;
    }
}

void AudioBackendXAudio2::Source_IsLoopingChanged(AudioSource* source)
{
    auto aSource = XAudio2::GetSource(source);
    if (!aSource || !aSource->Voice)
        return;

    // Skip if has no buffers (waiting for data or sth)
    XAUDIO2_VOICE_STATE state;
    aSource->Voice->GetState(&state);
    if (state.BuffersQueued == 0)
        return;

    // Looping is defined during buffer submission so reset source buffer (this is called only for non-streamable sources that ue single buffer)

    XAudio2::Locker.Lock();
    const uint32 bufferId = source->Clip->Buffers[0];
    XAudio2::Buffer* aBuffer = XAudio2::Buffers[bufferId - 1];
    XAudio2::Locker.Unlock();

    HRESULT hr;
    const bool isPlaying = source->IsActuallyPlayingSth();
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
    if (source->GetIsLooping())
        buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

    // Restore play position
    const UINT32 totalSamples = aBuffer->Info.NumSamples / aBuffer->Info.NumChannels;
    buffer.PlayBegin = state.SamplesPlayed % totalSamples;
    buffer.PlayLength = totalSamples - buffer.PlayBegin;
    aSource->StartTimeForQueueBuffer = 0;

    XAudio2::QueueBuffer(aSource, source, bufferId, buffer);

    if (isPlaying)
    {
        hr = aSource->Voice->Start();
        XAUDIO2_CHECK_ERROR(Start);
    }
}

void AudioBackendXAudio2::Source_SpatialSetupChanged(AudioSource* source)
{
    auto aSource = XAudio2::GetSource(source);
    if (aSource)
    {
        // TODO: implement attenuation settings for 3d audio
        auto clip = source->Clip.Get();
        if (clip && clip->IsLoaded())
        {
            const auto& header = clip->AudioHeader;
            aSource->Data.ChannelCount = source->Is3D() ? 1 : header.Info.NumChannels; // 3d audio is always mono (AudioClip auto-converts before buffer write)
            aSource->IsForceMono3D = header.Is3D && header.Info.NumChannels > 1;
        }
        aSource->Is3D = source->Is3D();
        aSource->DopplerFactor = source->GetDopplerFactor();
        aSource->Data.InnerRadius = FLAX_DST_TO_XAUDIO(source->GetMinDistance());
        aSource->IsDirty = true;
    }
}

void AudioBackendXAudio2::Source_ClipLoaded(AudioSource* source)
{
    ScopeLock lock(XAudio2::Locker);
    auto aSource = XAudio2::GetSource(source);
    if (!aSource)
    {
        // Register source if clip was missing
        Source_OnAdd(source);
    }
}

void AudioBackendXAudio2::Source_Cleanup(AudioSource* source)
{
    ScopeLock lock(XAudio2::Locker);
    auto aSource = XAudio2::GetSource(source);
    if (!aSource)
        return;

    // Free source
    if (aSource->Voice)
    {
        aSource->Voice->DestroyVoice();
    }
    aSource->Init();
}

void AudioBackendXAudio2::Source_Play(AudioSource* source)
{
    auto aSource = XAudio2::GetSource(source);
    if (aSource && aSource->Voice && !aSource->IsPlaying)
    {
        // Play
        const HRESULT hr = aSource->Voice->Start();
        XAUDIO2_CHECK_ERROR(Start);
        aSource->IsPlaying = true;
    }
}

void AudioBackendXAudio2::Source_Pause(AudioSource* source)
{
    auto aSource = XAudio2::GetSource(source);
    if (aSource && aSource->Voice && aSource->IsPlaying)
    {
        // Pause
        const HRESULT hr = aSource->Voice->Stop();
        XAUDIO2_CHECK_ERROR(Stop);
        aSource->IsPlaying = false;
    }
}

void AudioBackendXAudio2::Source_Stop(AudioSource* source)
{
    auto aSource = XAudio2::GetSource(source);
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

void AudioBackendXAudio2::Source_SetCurrentBufferTime(AudioSource* source, float value)
{
    const auto aSource = XAudio2::GetSource(source);
    if (aSource)
    {
        // Store start time so next buffer submitted will start from here (assumes audio is stopped)
        aSource->StartTimeForQueueBuffer = value;
    }
}

float AudioBackendXAudio2::Source_GetCurrentBufferTime(const AudioSource* source)
{
    float time = 0;
    auto aSource = XAudio2::GetSource(source);
    if (aSource)
    {
        ASSERT(source->Clip && source->Clip->IsLoaded());
        const auto& clipInfo = source->Clip->AudioHeader.Info;
        XAUDIO2_VOICE_STATE state;
        aSource->Voice->GetState(&state);
        const uint32 numChannels = clipInfo.NumChannels;
        const uint32 totalSamples = clipInfo.NumSamples / numChannels;
        const uint32 sampleRate = clipInfo.SampleRate;// / clipInfo.NumChannels;
        state.SamplesPlayed -= aSource->LastBufferStartSamplesPlayed % totalSamples; // Offset by the last buffer start to get time relative to its begin
        time = aSource->LastBufferStartTime + (state.SamplesPlayed % totalSamples) / static_cast<float>(Math::Max(1U, sampleRate));
    }
    return time;
}

void AudioBackendXAudio2::Source_SetNonStreamingBuffer(AudioSource* source)
{
    auto aSource = XAudio2::GetSource(source);
    if (!aSource)
        return;

    XAudio2::Locker.Lock();
    const uint32 bufferId = source->Clip->Buffers[0];
    XAudio2::Buffer* aBuffer = XAudio2::Buffers[bufferId - 1];
    XAudio2::Locker.Unlock();

    XAUDIO2_BUFFER buffer = { 0 };
    buffer.pContext = aBuffer;
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    if (source->GetIsLooping())
        buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

    // Queue single buffer
    XAudio2::QueueBuffer(aSource, source, bufferId, buffer);
}

void AudioBackendXAudio2::Source_GetProcessedBuffersCount(AudioSource* source, int32& processedBuffersCount)
{
    processedBuffersCount = 0;
    auto aSource = XAudio2::GetSource(source);
    if (aSource && aSource->Voice)
    {
        processedBuffersCount = aSource->BuffersProcessed;
    }
}

void AudioBackendXAudio2::Source_GetQueuedBuffersCount(AudioSource* source, int32& queuedBuffersCount)
{
    queuedBuffersCount = 0;
    auto aSource = XAudio2::GetSource(source);
    if (aSource && aSource->Voice)
    {
        XAUDIO2_VOICE_STATE state;
        aSource->Voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
        queuedBuffersCount = state.BuffersQueued;
    }
}

void AudioBackendXAudio2::Source_QueueBuffer(AudioSource* source, uint32 bufferId)
{
    auto aSource = XAudio2::GetSource(source);
    if (!aSource)
        return;

    XAudio2::Buffer* aBuffer = XAudio2::Buffers[bufferId - 1];

    XAUDIO2_BUFFER buffer = { 0 };
    buffer.pContext = aBuffer;

    XAudio2::QueueBuffer(aSource, source, bufferId, buffer);
}

void AudioBackendXAudio2::Source_DequeueProcessedBuffers(AudioSource* source)
{
    auto aSource = XAudio2::GetSource(source);
    if (aSource && aSource->Voice)
    {
        const HRESULT hr = aSource->Voice->FlushSourceBuffers();
        XAUDIO2_CHECK_ERROR(FlushSourceBuffers);
        aSource->BuffersProcessed = 0;
    }
}

uint32 AudioBackendXAudio2::Buffer_Create()
{
    uint32 bufferId;
    ScopeLock lock(XAudio2::Locker);

    // Get first free buffer slot
    XAudio2::Buffer* aBuffer = nullptr;
    for (int32 i = 0; i < XAudio2::Buffers.Count(); i++)
    {
        if (XAudio2::Buffers[i] == nullptr)
        {
            aBuffer = New<XAudio2::Buffer>();
            XAudio2::Buffers[i] = aBuffer;
            bufferId = i + 1;
            break;
        }
    }
    if (!aBuffer)
    {
        // Add new slot
        aBuffer = New<XAudio2::Buffer>();
        XAudio2::Buffers.Add(aBuffer);
        bufferId = XAudio2::Buffers.Count();
    }

    aBuffer->Data.Resize(0);
    return bufferId;
}

void AudioBackendXAudio2::Buffer_Delete(uint32 bufferId)
{
    ScopeLock lock(XAudio2::Locker);
    XAudio2::Buffer*& aBuffer = XAudio2::Buffers[bufferId - 1];
    aBuffer->Data.Resize(0);
    Delete(aBuffer);
    aBuffer = nullptr;
}

void AudioBackendXAudio2::Buffer_Write(uint32 bufferId, byte* samples, const AudioDataInfo& info)
{
    XAudio2::Locker.Lock();
    XAudio2::Buffer* aBuffer = XAudio2::Buffers[bufferId - 1];
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
    XAudio2::MarkAllDirty();
}

void AudioBackendXAudio2::Base_SetVolume(float value)
{
    if (XAudio2::MasteringVoice)
    {
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
    XAudio2::SampleRate = details.InputSampleRate;
    XAudio2::Channels = details.InputChannels;
    hr = XAudio2::MasteringVoice->GetChannelMask(&XAudio2::ChannelMask);
    if (FAILED(hr))
    {
        LOG(Error, "Failed to get XAudio2 mastering voice channel mask. Error: 0x{0:x}", hr);
        return true;
    }

    // Initialize spatial audio subsystem
    DWORD dwChannelMask;
    XAudio2::MasteringVoice->GetChannelMask(&dwChannelMask);
    hr = X3DAudioInitialize(dwChannelMask, X3DAUDIO_SPEED_OF_SOUND, XAudio2::X3DInstance);
    if (FAILED(hr))
    {
        LOG(Error, "Failed to initalize XAudio2 3D support. Error: 0x{0:x}", hr);
        return true;
    }

    // Info
    LOG(Info, "XAudio2: {0} channels at {1} kHz (channel mask {2})", XAudio2::Channels, XAudio2::SampleRate / 1000.0f, XAudio2::ChannelMask);

    // Dummy device
    devices.Resize(1);
    devices[0].Name = TEXT("XAudio2 device");
    Audio::SetActiveDeviceIndex(0);

    return false;
}

void AudioBackendXAudio2::Base_Update()
{
    // Update dirty voices
    const auto listener = XAudio2::GetListener();
    const float dopplerFactor = AudioSettings::Get()->DopplerFactor;
    float matrixCoefficients[MAX_CHANNELS_MATRIX_SIZE];
    X3DAUDIO_DSP_SETTINGS dsp = { 0 };
    dsp.DstChannelCount = XAudio2::Channels;
    dsp.pMatrixCoefficients = matrixCoefficients;
    for (int32 i = 0; i < XAudio2::Sources.Count(); i++)
    {
        auto& source = XAudio2::Sources[i];
        if (source.IsFree() || !(source.IsDirty || XAudio2::ForceDirty))
            continue;

        dsp.SrcChannelCount = source.Data.ChannelCount;
        if (source.Is3D && listener)
        {
            // 3D sound
            X3DAudioCalculate(XAudio2::X3DInstance, &listener->Data, &source.Data, X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER, &dsp);
        }
        else
        {
            // 2D sound
            dsp.DopplerFactor = 1.0f;
            Platform::MemoryClear(dsp.pMatrixCoefficients, sizeof(matrixCoefficients));
            dsp.pMatrixCoefficients[0] = 1.0f;
            if (source.Format.nChannels == 1)
            {
                dsp.pMatrixCoefficients[1] = 1.0f;
            }
            else
            {
                dsp.pMatrixCoefficients[3] = 1.0f;
            }
            const float panLeft = Math::Min(1.0f - source.Pan, 1.0f);
            const float panRight = Math::Min(1.0f + source.Pan, 1.0f);
            if (source.Format.nChannels >= 2)
            {
                dsp.pMatrixCoefficients[0] *= panLeft;
                dsp.pMatrixCoefficients[3] *= panRight;
            }
        }
        if (source.IsForceMono3D)
        {
            // Hack to fix playback speed for 3D clip that has auto-converted stereo to mono at runtime
            dsp.DopplerFactor *= 0.5f;
        }
        const float frequencyRatio = dopplerFactor * source.Pitch * dsp.DopplerFactor * source.DopplerFactor;
        source.Voice->SetFrequencyRatio(frequencyRatio);
        source.Voice->SetOutputMatrix(XAudio2::MasteringVoice, dsp.SrcChannelCount, dsp.DstChannelCount, dsp.pMatrixCoefficients);

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
