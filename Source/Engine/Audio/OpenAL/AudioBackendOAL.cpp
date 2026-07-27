// Copyright (c) Wojciech Figat. All rights reserved.

#if AUDIO_API_OPENAL

#include "AudioBackendOAL.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Platform/StringUtils.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Tools/AudioTool/AudioTool.h"
#include "Engine/Engine/Units.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"
#include "Engine/Engine/Time.h"
#include "Engine/Audio/Audio.h"
#include "Engine/Audio/AudioListener.h"
#include "Engine/Audio/AudioSource.h"
#include "Engine/Audio/AudioSettings.h"
#include "Engine/Content/Content.h"
#include "Engine/Level/Level.h"
#include "Engine/Video/VideoPlayer.h"

// Include OpenAL library
// Source: https://github.com/kcat/openal-soft
//#define AL_LIBTYPE_STATIC
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <OpenAL/alext.h>

#define FLAX_DST_TO_OAL(x) x * UNITS_TO_METERS_SCALE
#define FLAX_POS_TO_OAL(vec) ((ALfloat)vec.X * -UNITS_TO_METERS_SCALE), ((ALfloat)vec.Y * UNITS_TO_METERS_SCALE), ((ALfloat)vec.Z * UNITS_TO_METERS_SCALE)
#define FLAX_VEL_TO_OAL(vec) ((ALfloat)vec.X * -(UNITS_TO_METERS_SCALE*UNITS_TO_METERS_SCALE)), ((ALfloat)vec.Y * (UNITS_TO_METERS_SCALE*UNITS_TO_METERS_SCALE)), ((ALfloat)vec.Z * (UNITS_TO_METERS_SCALE*UNITS_TO_METERS_SCALE))
#if BUILD_RELEASE
#define ALC_CHECK_ERROR(method)
#else
#define ALC_CHECK_ERROR(method) \
    { \
        int alError = alGetError(); \
        if (alError != 0) \
        { \
            const Char* errorStr = GetOpenALErrorString(alError); \
            LOG(Error, "OpenAL method {0} failed with error 0x{1:X}:{2} (at line {3})", TEXT(#method), alError, errorStr, __LINE__ - 1); \
        } \
    }
#endif

const Char* GetOpenALErrorString(int error)
{
    switch (error)
    {
    case AL_NO_ERROR:
        return TEXT("AL_NO_ERROR");
    case AL_INVALID_NAME:
        return TEXT("AL_INVALID_NAME");
    case AL_INVALID_ENUM:
        return TEXT("AL_INVALID_ENUM");
    case AL_INVALID_VALUE:
        return TEXT("AL_INVALID_VALUE");
    case AL_INVALID_OPERATION:
        return TEXT("AL_INVALID_OPERATION");
    case AL_OUT_OF_MEMORY:
        return TEXT("AL_OUT_OF_MEMORY");
    default:
        break;
    }
    return TEXT("???");
}

namespace ALC
{
    struct SourceData
    {
        AudioDataInfo Format;
        float Pan;
        bool Spatial;
    };

    ALCdevice* Device = nullptr;
    ALCcontext* Context = nullptr;
    AudioBackend::FeatureFlags Features = AudioBackend::FeatureFlags::None;
    bool Inited = false;
    CriticalSection Locker;
    Dictionary<uint32, SourceData> SourcesData;

    bool IsExtensionSupported(const char* extension)
    {
        if (Device == nullptr)
            return false;
        const int32 length = StringUtils::Length(extension);
        if ((length > 2) && (StringUtils::Compare(extension, "ALC", 3) == 0))
            return alcIsExtensionPresent(Device, extension) != AL_FALSE;
        return alIsExtensionPresent(extension) != AL_FALSE;
    }

    void ClearContext()
    {
        if (Context)
        {
            alcMakeContextCurrent(nullptr);
            alcDestroyContext(Context);
            Context = nullptr;
        }
    }

    namespace Listener
    {
        void Rebuild(const AudioListener* listener)
        {
            AudioBackend::Listener::Reset();
            AudioBackend::Listener::TransformChanged(listener->GetPosition(), listener->GetOrientation());
            AudioBackend::Listener::VelocityChanged(listener->GetVelocity());
        }
    }

    namespace Source
    {
        void SetupSpatial(uint32 sourceID, float pan, bool spatial)
        {
            alSourcei(sourceID, AL_SOURCE_RELATIVE, !spatial); // Non-spatial sounds use AL_POSITION for panning
#ifdef AL_SOFT_source_spatialize
            alSourcei(sourceID, AL_SOURCE_SPATIALIZE_SOFT, spatial || Math::Abs(pan) > ZeroTolerance ? AL_TRUE : AL_FALSE); // Fix multi-channel sources played as spatial or non-spatial sources played with panning
#endif
            if (spatial)
            {
#ifdef AL_EXT_STEREO_ANGLES
                const float panAngle = pan * PI_HALF;
                const ALfloat panAngles[2] = { (ALfloat)(PI / 6.0 - panAngle), (ALfloat)(-PI / 6.0 - panAngle) }; // Angles are specified counter-clockwise in radians
                alSourcefv(sourceID, AL_STEREO_ANGLES, panAngles);
#endif
            }
            else
            {
                alSource3f(sourceID, AL_POSITION, pan, 0, -sqrtf(1.0f - pan * pan));
            }
        }

        void Rebuild(uint32& sourceID, const Vector3& position, const Quaternion& orientation, float volume, float pitch, float pan, bool loop, bool spatial, float attenuation, float minDistance, float doppler)
        {
            ASSERT_LOW_LAYER(sourceID == 0);
            alGenSources(1, &sourceID);
            if (sourceID == 0)
            {
                ALC_CHECK_ERROR(alGenSources);
                return;
            }

            alSourcef(sourceID, AL_GAIN, volume);
            alSourcef(sourceID, AL_PITCH, pitch);
            alSourcef(sourceID, AL_SEC_OFFSET, 0.0f);
            alSourcei(sourceID, AL_LOOPING, loop);
            alSourcei(sourceID, AL_BUFFER, 0);
            SetupSpatial(sourceID, pan, spatial);
            if (spatial)
            {
                alSourcef(sourceID, AL_ROLLOFF_FACTOR, attenuation);
                alSourcef(sourceID, AL_DOPPLER_FACTOR, doppler);
                alSourcef(sourceID, AL_REFERENCE_DISTANCE, FLAX_DST_TO_OAL(minDistance));
                alSource3f(sourceID, AL_POSITION, FLAX_POS_TO_OAL(position));
                alSource3f(sourceID, AL_VELOCITY, FLAX_VEL_TO_OAL(Vector3::Zero));
            }
            else
            {
                alSourcef(sourceID, AL_ROLLOFF_FACTOR, 0.0f);
                alSourcef(sourceID, AL_DOPPLER_FACTOR, 1.0f);
                alSourcef(sourceID, AL_REFERENCE_DISTANCE, 0.0f);
                alSource3f(sourceID, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
            }
        }
    }

    struct AudioSourceState
    {
        AudioSource::States State;
        float Time;
    };

    void RebuildContext()
    {
        ClearContext();
        if (Device == nullptr)
            return;

#if PLATFORM_WEB
        ALCint* attrList = nullptr;
#else
        ALCint attrList[] = { ALC_HRTF_SOFT, ALC_FALSE };
        if (Audio::GetEnableHRTF())
        {
            LOG(Info, "Enabling OpenAL HRTF");
            attrList[1] = ALC_TRUE; 
        }
#endif

        Context = alcCreateContext(Device, attrList);
        if (Context == nullptr)
        {
            LOG(Error, "Failed to create OpenAL context.");
            return;
        }
        alcMakeContextCurrent(Context);
    }
    
    void RebuildListeners()
    {
        for (AudioListener* listener : Audio::Listeners)
            Listener::Rebuild(listener);
    }
    
    void RebuildSources(const Array<AudioSourceState>& states, float elapsedTime = 0.0f)
    {
        for (int32 i = 0; i < states.Count(); i++)
        {
            AudioSource* source = Audio::Sources[i];
            Source::Rebuild(source->SourceID, source->GetPosition(), source->GetOrientation(), source->GetVolume(), source->GetPitch(), source->GetPan(), source->GetIsLooping() && !source->UseStreaming(), source->Is3D(), source->GetAttenuation(), source->GetMinDistance(), source->GetDopplerFactor());

            if (source->SourceID)
            {
                // Restore playback state and time position without restarting or pausing
                auto& state = states[i];
                if (state.State != AudioSource::States::Stopped)
                {
                    if (source->Clip && source->Clip->IsLoaded())
                    {
                        float targetTime = state.Time;
                        if (state.State == AudioSource::States::Playing && elapsedTime > 0.0f)
                        {
                            targetTime += elapsedTime;
                            const float clipLength = source->Clip->GetLength();
                            if (clipLength > 0.0001f)
                            {
                                if (source->GetIsLooping())
                                {
                                    targetTime = fmodf(targetTime, clipLength);
                                }
                                else
                                {
                                    targetTime = Math::Min(targetTime, clipLength);
                                }
                            }
                        }

                        if (source->UseStreaming())
                        {
                            float relativeTime = 0;
                            const int32 chunkIndex = source->Clip->GetFirstBufferIndex(targetTime, relativeTime);
                            AudioBackend::Source::SetStreamingFirstChunk(source, chunkIndex);
                            source->RequestStreamingBuffersUpdate();
                        }
                        else
                        {
                            if (source->Clip->Buffers.HasItems() && source->Clip->Buffers[0] != 0)
                            {
                                AudioBackend::Source::SetNonStreamingBuffer(source->SourceID, source->Clip->Buffers[0]);
                                AudioBackend::Source::SetCurrentBufferTime(source->SourceID, targetTime);
                            }
                        }

                        if (state.State == AudioSource::States::Playing)
                            source->Play();
                        else if (state.State == AudioSource::States::Paused)
                            source->Pause();
                    }
                }
            }
        }
    }

    void RebuildContext(const Array<AudioSourceState>& states)
    {
        RebuildContext();
        RebuildListeners();
        RebuildSources(states);
    }

    void RebuildContext(bool isChangingDevice)
    {
        Array<AudioSourceState> states;
        if (!isChangingDevice)
        {
            states.EnsureCapacity(Audio::Sources.Count());
            for (AudioSource* source : Audio::Sources)
            {
                states.Add({ source->GetState(), source->GetTime() });
                source->Stop();
                if (source->SourceID)
                {
                    alDeleteSources(1, &source->SourceID);
                    ALC_CHECK_ERROR(alDeleteSources);
                    source->SourceID = 0;
                }
            }
        }

        RebuildContext(states);
    }
}

ALenum GetOpenALBufferFormat(uint32 numChannels, uint32 bitDepth)
{
    // TODO: cache enum values in Init()??
    switch (bitDepth)
    {
    case 8:
        switch (numChannels)
        {
        case 1:
            return AL_FORMAT_MONO8;
        case 2:
            return AL_FORMAT_STEREO8;
        case 4:
            return alGetEnumValue("AL_FORMAT_QUAD8");
        case 6:
            return alGetEnumValue("AL_FORMAT_51CHN8");
        case 7:
            return alGetEnumValue("AL_FORMAT_61CHN8");
        case 8:
            return alGetEnumValue("AL_FORMAT_71CHN8");
        }
    case 16:
        switch (numChannels)
        {
        case 1:
            return AL_FORMAT_MONO16;
        case 2:
            return AL_FORMAT_STEREO16;
        case 4:
            return alGetEnumValue("AL_FORMAT_QUAD16");
        case 6:
            return alGetEnumValue("AL_FORMAT_51CHN16");
        case 7:
            return alGetEnumValue("AL_FORMAT_61CHN16");
        case 8:
            return alGetEnumValue("AL_FORMAT_71CHN16");
        }
    case 32:
        switch (numChannels)
        {
        case 1:
#ifdef AL_FORMAT_MONO_FLOAT32
            return AL_FORMAT_MONO_FLOAT32;
#else
            return alGetEnumValue("AL_FORMAT_MONO_FLOAT32");
#endif
        case 2:
#ifdef AL_FORMAT_STEREO_FLOAT32
            return AL_FORMAT_STEREO_FLOAT32;
#else
            return alGetEnumValue("AL_FORMAT_STEREO_FLOAT32");
#endif
        case 4:
            return alGetEnumValue("AL_FORMAT_QUAD32");
        case 6:
            return alGetEnumValue("AL_FORMAT_51CHN32");
        case 7:
            return alGetEnumValue("AL_FORMAT_61CHN32");
        case 8:
            return alGetEnumValue("AL_FORMAT_71CHN32");
        }
    }
    return 0;
}

void AudioBackendOAL::Listener_Reset()
{
    alListenerf(AL_GAIN, Audio::GetVolume());
}

void AudioBackendOAL::Listener_VelocityChanged(const Vector3& velocity)
{
    alListener3f(AL_VELOCITY, FLAX_VEL_TO_OAL(velocity));
}

void AudioBackendOAL::Listener_TransformChanged(const Vector3& position, const Quaternion& orientation)
{
    const Float3 flipX(-1, 1, 1);
    const Float3 alOrientation[2] =
    {
        orientation * Float3::Forward * flipX,
        orientation * Float3::Up * flipX
    };
    alListenerfv(AL_ORIENTATION, (float*)alOrientation);
    alListener3f(AL_POSITION, FLAX_POS_TO_OAL(position));
}

void AudioBackendOAL::Listener_ReinitializeAll()
{
    ALC::RebuildContext(false);
}

uint32 AudioBackendOAL::Source_Add(const AudioDataInfo& format, const Vector3& position, const Quaternion& orientation, float volume, float pitch, float pan, bool loop, bool spatial, float attenuation, float minDistance, float doppler)
{
    PROFILE_MEM(Audio);
    uint32 sourceID = 0;
    ALC::Source::Rebuild(sourceID, position, orientation, volume, pitch, pan, loop, spatial, attenuation, minDistance, doppler);
    if (sourceID)
    {
        // Cache audio data format assigned on source (used in Source_GetCurrentBufferTime)
        ALC::Locker.Lock();
        auto& data = ALC::SourcesData[sourceID];
        data.Format = format;
        data.Spatial = spatial;
        data.Pan = pan;
        ALC::Locker.Unlock();
    }
    return sourceID;
}

void AudioBackendOAL::Source_Remove(uint32 sourceID)
{
    alSourcei(sourceID, AL_BUFFER, 0);
    ALC_CHECK_ERROR(alSourcei);
    alDeleteSources(1, &sourceID);
    ALC_CHECK_ERROR(alDeleteSources);

    ALC::Locker.Lock();
    ALC::SourcesData.Remove(sourceID);
    ALC::Locker.Unlock();
}

void AudioBackendOAL::Source_VelocityChanged(uint32 sourceID, const Vector3& velocity)
{
    ALC::Locker.Lock();
    const bool spatial = ALC::SourcesData[sourceID].Spatial;
    ALC::Locker.Unlock();
    if (spatial)
    {
        alSource3f(sourceID, AL_VELOCITY, FLAX_VEL_TO_OAL(velocity));
    }
}

void AudioBackendOAL::Source_TransformChanged(uint32 sourceID, const Vector3& position, const Quaternion& orientation)
{
    ALC::Locker.Lock();
    const bool spatial = ALC::SourcesData[sourceID].Spatial;
    ALC::Locker.Unlock();
    if (spatial)
    {
        alSource3f(sourceID, AL_POSITION, FLAX_POS_TO_OAL(position));
    }
}

void AudioBackendOAL::Source_VolumeChanged(uint32 sourceID, float volume)
{
    alSourcef(sourceID, AL_GAIN, volume);
}

void AudioBackendOAL::Source_PitchChanged(uint32 sourceID, float pitch)
{
    alSourcef(sourceID, AL_PITCH, pitch);
}

void AudioBackendOAL::Source_PanChanged(uint32 sourceID, float pan)
{
    ALC::Locker.Lock();
    auto& e = ALC::SourcesData[sourceID];
    e.Pan = pan;
    const bool spatial = e.Spatial;
    ALC::Locker.Unlock();
    ALC::Source::SetupSpatial(sourceID, pan, spatial);
}

void AudioBackendOAL::Source_IsLoopingChanged(uint32 sourceID, bool loop)
{
    alSourcei(sourceID, AL_LOOPING, loop);
}

void AudioBackendOAL::Source_SpatialSetupChanged(uint32 sourceID, bool spatial, float attenuation, float minDistance, float doppler)
{
    ALC::Locker.Lock();
    const float pan = ALC::SourcesData[sourceID].Pan;
    ALC::Locker.Unlock();
    if (spatial)
    {
        alSourcef(sourceID, AL_ROLLOFF_FACTOR, attenuation);
        alSourcef(sourceID, AL_DOPPLER_FACTOR, doppler);
        alSourcef(sourceID, AL_REFERENCE_DISTANCE, FLAX_DST_TO_OAL(minDistance));
    }
    else
    {
        alSourcef(sourceID, AL_ROLLOFF_FACTOR, 0.0f);
        alSourcef(sourceID, AL_DOPPLER_FACTOR, 1.0f);
        alSourcef(sourceID, AL_REFERENCE_DISTANCE, 0.0f);
    }
    ALC::Source::SetupSpatial(sourceID, pan, spatial);
}

void AudioBackendOAL::Source_Play(uint32 sourceID)
{
    alSourcePlay(sourceID);
    ALC_CHECK_ERROR(alSourcePlay);
}

void AudioBackendOAL::Source_Pause(uint32 sourceID)
{
    alSourcePause(sourceID);
    ALC_CHECK_ERROR(alSourcePause);
}

void AudioBackendOAL::Source_Stop(uint32 sourceID)
{
    // Stop and rewind
    alSourceRewind(sourceID);
    ALC_CHECK_ERROR(alSourceRewind);
    alSourcef(sourceID, AL_SEC_OFFSET, 0.0f);

    // Unset streaming buffers
    alSourcei(sourceID, AL_BUFFER, 0);
    ALC_CHECK_ERROR(alSourcei);
}

void AudioBackendOAL::Source_SetCurrentBufferTime(uint32 sourceID, float value)
{
    alSourcef(sourceID, AL_SEC_OFFSET, value);
}

float AudioBackendOAL::Source_GetCurrentBufferTime(uint32 sourceID)
{
#if 0
    float time;
    alGetSourcef(sourceID, AL_SEC_OFFSET, &time);
#else
    ALC::Locker.Lock();
    AudioDataInfo clipInfo = ALC::SourcesData[sourceID].Format;
    ALC::Locker.Unlock();
    ALint samplesPlayed;
    alGetSourcei(sourceID, AL_SAMPLE_OFFSET, &samplesPlayed);
    const uint32 totalSamples = clipInfo.NumSamples / clipInfo.NumChannels;
    if (totalSamples > 0)
        samplesPlayed %= totalSamples;
    const float time = samplesPlayed / static_cast<float>(Math::Max(1U, clipInfo.SampleRate));
#endif
    return time;
}

void AudioBackendOAL::Source_SetNonStreamingBuffer(uint32 sourceID, uint32 bufferID)
{
    alSourcei(sourceID, AL_BUFFER, bufferID);
    ALC_CHECK_ERROR(alSourcei);
}

void AudioBackendOAL::Source_GetProcessedBuffersCount(uint32 sourceID, int32& processedBuffersCount)
{
    // Check the first context only
    alGetSourcei(sourceID, AL_BUFFERS_PROCESSED, &processedBuffersCount);
    ALC_CHECK_ERROR(alGetSourcei);
}

void AudioBackendOAL::Source_GetQueuedBuffersCount(uint32 sourceID, int32& queuedBuffersCount)
{
    // Check the first context only
    alGetSourcei(sourceID, AL_BUFFERS_QUEUED, &queuedBuffersCount);
    ALC_CHECK_ERROR(alGetSourcei);
}

void AudioBackendOAL::Source_QueueBuffer(uint32 sourceID, uint32 bufferID)
{
    // Queue new buffer
    alSourceQueueBuffers(sourceID, 1, &bufferID);
    ALC_CHECK_ERROR(alSourceQueueBuffers);
}

void AudioBackendOAL::Source_DequeueProcessedBuffers(uint32 sourceID)
{
    int32 numProcessedBuffers;
    alGetSourcei(sourceID, AL_BUFFERS_PROCESSED, &numProcessedBuffers);
    Array<ALuint, InlinedAllocation<AUDIO_MAX_SOURCE_BUFFERS>> buffers;
    buffers.Resize(numProcessedBuffers);
    alSourceUnqueueBuffers(sourceID, numProcessedBuffers, buffers.Get());
    ALC_CHECK_ERROR(alSourceUnqueueBuffers);
}

uint32 AudioBackendOAL::Buffer_Create()
{
    uint32 bufferID;
    alGenBuffers(1, &bufferID);
    ALC_CHECK_ERROR(alGenBuffers);
    return bufferID;
}

void AudioBackendOAL::Buffer_Delete(uint32 bufferID)
{
    alDeleteBuffers(1, &bufferID);
    ALC_CHECK_ERROR(alDeleteBuffers);
}

void AudioBackendOAL::Buffer_Write(uint32 bufferID, byte* samples, const AudioDataInfo& info)
{
    PROFILE_CPU();
    PROFILE_MEM(Audio);

    // Pick the format for the audio data (it might not be supported natively)
    ALenum format = GetOpenALBufferFormat(info.NumChannels, info.BitDepth);

    // Mono or stereo
    if (info.NumChannels <= 2)
    {
        if (info.BitDepth > 16)
        {
            if (ALC::IsExtensionSupported("AL_EXT_float32"))
            {
                const uint32 bufferSize = info.NumSamples * sizeof(float);
                float* sampleBufferFloat = (float*)Allocator::Allocate(bufferSize);
                AudioTool::ConvertToFloat(samples, info.BitDepth, sampleBufferFloat, info.NumSamples);

                format = GetOpenALBufferFormat(info.NumChannels, 32);
                alBufferData(bufferID, format, sampleBufferFloat, bufferSize, info.SampleRate);
                ALC_CHECK_ERROR(alBufferData);
                Allocator::Free(sampleBufferFloat);
            }
            else
            {
                LOG(Warning, "OpenAL doesn't support bit depth larger than 16. Audio data will be truncated.");
                const uint32 bufferSize = info.NumSamples * 2;
                byte* sampleBuffer16 = (byte*)Allocator::Allocate(bufferSize);
                AudioTool::ConvertBitDepth(samples, info.BitDepth, sampleBuffer16, 16, info.NumSamples);

                format = GetOpenALBufferFormat(info.NumChannels, 16);
                alBufferData(bufferID, format, sampleBuffer16, bufferSize, info.SampleRate);
                ALC_CHECK_ERROR(alBufferData);
                Allocator::Free(sampleBuffer16);
            }
        }
        else if (info.BitDepth == 8)
        {
            // OpenAL expects unsigned 8-bit data, but engine stores it as signed, so convert
            const uint32 bufferSize = info.NumSamples * (info.BitDepth / 8);
            byte* sampleBuffer = (byte*)Allocator::Allocate(bufferSize);
            for (uint32 i = 0; i < info.NumSamples; i++)
                sampleBuffer[i] = ((int8*)samples)[i] + 128;

            alBufferData(bufferID, format, sampleBuffer, bufferSize, info.SampleRate);
            ALC_CHECK_ERROR(alBufferData);
            Allocator::Free(sampleBuffer);
        }
        else if (format)
        {
            alBufferData(bufferID, format, samples, info.NumSamples * (info.BitDepth / 8), info.SampleRate);
            ALC_CHECK_ERROR(alBufferData);
        }
    }
    // Multichannel
    else
    {
        // Note: Assuming AL_EXT_MCFORMATS is supported. If it's not, channels should be reduced to mono or stereo.

        // 24-bit not supported, convert to 32-bit
        if (info.BitDepth == 24)
        {
            const uint32 bufferSize = info.NumChannels * sizeof(int32);
            byte* sampleBuffer32 = (byte*)Allocator::Allocate(bufferSize);
            AudioTool::ConvertBitDepth(samples, info.BitDepth, sampleBuffer32, 32, info.NumSamples);

            format = GetOpenALBufferFormat(info.NumChannels, 32);
            alBufferData(bufferID, format, sampleBuffer32, bufferSize, info.SampleRate);
            ALC_CHECK_ERROR(alBufferData);

            Allocator::Free(sampleBuffer32);
        }
        else if (info.BitDepth == 8)
        {
            // OpenAL expects unsigned 8-bit data, but engine stores it as signed, so convert
            const uint32 bufferSize = info.NumSamples * (info.BitDepth / 8);
            byte* sampleBuffer = (byte*)Allocator::Allocate(bufferSize);

            for (uint32 i = 0; i < info.NumSamples; i++)
                sampleBuffer[i] = ((int8*)samples)[i] + 128;

            format = GetOpenALBufferFormat(info.NumChannels, 16);
            alBufferData(bufferID, format, sampleBuffer, bufferSize, info.SampleRate);
            ALC_CHECK_ERROR(alBufferData);

            Allocator::Free(sampleBuffer);
        }
        else if (format)
        {
            alBufferData(bufferID, format, samples, info.NumSamples * (info.BitDepth / 8), info.SampleRate);
            ALC_CHECK_ERROR(alBufferData);
        }
    }

    if (!format)
    {
        LOG(Error, "Not supported audio data format for OpenAL device: BitDepth={}, NumChannels={}", info.BitDepth, info.NumChannels);
    }
}

const Char* AudioBackendOAL::Base_Name()
{
    return TEXT("OpenAL");
}

AudioBackend::FeatureFlags AudioBackendOAL::Base_Features()
{
    return ALC::Features;
}

void AudioBackendOAL::Base_OnActiveDeviceChanged()
{
    PROFILE_CPU();
    PROFILE_MEM(Audio);
    // Fast-path on startup
    if (!ALC::Inited && ALC::Device)
    {
        ALC::RebuildContext();
        return;
    }

    const double startTime = Platform::GetTimeSeconds();

    // Cleanup
    Array<ALC::AudioSourceState> states;
    states.EnsureCapacity(Audio::Sources.Count());
    for (AudioSource* source : Audio::Sources)
    {
        states.Add({ source->GetState(), source->GetTime() });
        source->Stop();
        if (source->SourceID)
        {
            Source_Remove(source->SourceID);
            source->SourceID = 0;
        }
    }
    ALC::ClearContext();
    if (ALC::Device != nullptr)
    {
        alcCloseDevice(ALC::Device);
        ALC::Device = nullptr;
    }

    // Open device
    const StringAnsi* namePtr = (Audio::Devices.HasItems() && Audio::GetActiveDeviceIndex() >= 0 && Audio::GetActiveDeviceIndex() < Audio::Devices.Count()) ? &Audio::GetActiveDevice()->InternalName : nullptr;
    if (namePtr)
    {
        ALC::Device = alcOpenDevice(namePtr->Get());
        if (ALC::Device == nullptr)
        {
            LOG(Warning, "Failed to open active OpenAL device ({0}). Trying default device...", String(*namePtr));
            ALC::Device = alcOpenDevice(nullptr);
        }
    }
    else
    {
        ALC::Device = alcOpenDevice(nullptr);
    }

    if (ALC::Device == nullptr && Audio::Devices.HasItems())
    {
        for (int32 i = 0; i < Audio::Devices.Count(); i++)
        {
            ALC::Device = alcOpenDevice(Audio::Devices[i].InternalName.Get());
            if (ALC::Device != nullptr)
                break;
        }
    }

    if (ALC::Device == nullptr)
    {
        LOG(Warning, "Failed to open any OpenAL audio device.");
        return;
    }
    if (ALC::Inited)
    {
        const AudioDevice* activeDev = Audio::GetActiveDevice();
        LOG(Info, "Changed audio device to: {}", activeDev ? String(activeDev->Name) : TEXT("Default"));
    }

    // Rebuild context
    ALC::RebuildContext();
    if (ALC::Inited)
    {
        // Reload all audio clips to recreate their buffers
        for (AudioClip* audioClip : Content::GetAssets<AudioClip>())
        {
            audioClip->WaitForLoaded();
            ScopeLock lock(audioClip->Locker);

            // Clear old buffer IDs
            for (uint32& bufferID : audioClip->Buffers)
                bufferID = 0;

            if (audioClip->IsStreamable())
            {
                // Let the streaming recreate missing buffers
                audioClip->RequestStreamingUpdate();
            }
            else
            {
                // Reload audio clip
                if (audioClip->Storage)
                {
                    auto assetLock = audioClip->Storage->Lock();
                    audioClip->LoadChunk(0);
                }
                if (audioClip->Buffers.HasItems())
                {
                    audioClip->Buffers[0] = AudioBackend::Buffer::Create();
                    audioClip->WriteBuffer(0);
                }
            }
        }

        // Reload all videos to recreate their buffers
        for (VideoPlayer* videoPlayer : Level::GetActors<VideoPlayer>(true))
        {
            VideoBackendPlayer& player = videoPlayer->_player;

            // Clear audio state
            for (uint32& bufferID : player.AudioBuffers)
                bufferID = 0;
            player.NextAudioBuffer = 0;
            player.AudioSource = 0;
        }
    }
    const float elapsedTime = static_cast<float>(Platform::GetTimeSeconds() - startTime);
    ALC::RebuildListeners();
    ALC::RebuildSources(states, elapsedTime);
}

void AudioBackendOAL::Base_SetDopplerFactor(float value)
{
    alDopplerFactor(value);
}

void AudioBackendOAL::Base_SetVolume(float value)
{
    alListenerf(AL_GAIN, value);
}

bool AudioBackendOAL::Base_Init()
{
    auto& devices = Audio::Devices;

#if 0
    // Use it for ALSOFT errors debugging (build OpenAL-Soft in Debug)
    Platform::SetEnvironmentVariable(TEXT("ALSOFT_TRAP_ERROR"), TEXT("1"));
    Platform::SetEnvironmentVariable(TEXT("ALSOFT_LOGLEVEL"), TEXT("9"));
    Platform::SetEnvironmentVariable(TEXT("ALSOFT_LOGFILE"), TEXT("alc_log.txt"));
#endif

    // Initialization (use the preferred device)
    int32 activeDeviceIndex;
    ALC::Device = alcOpenDevice(nullptr);
    if (ALC::Device == nullptr)
    {
        activeDeviceIndex = -1;
        const auto err = alGetError();
        LOG(Warning, "Failed to open default OpenAL device. Error: 0x{0:X}", err);
    }
    else
    {
        activeDeviceIndex = 0;
    }

    // Get audio devices
#if ALC_ENUMERATE_ALL_EXT
    const ALCchar* defaultDevice = alcGetString(nullptr, ALC_DEFAULT_ALL_DEVICES_SPECIFIER);
    if (ALC::IsExtensionSupported("ALC_ENUMERATE_ALL_EXT") && defaultDevice != nullptr)
    {
        const ALCchar* devicesStr = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);

        const StringAnsi defaultDeviceName(defaultDevice);
        Audio::SetSystemDefaultDeviceName(defaultDeviceName);

        devices.Clear();
        devices.EnsureCapacity(8);

        activeDeviceIndex = -1;
        while (devicesStr && *devicesStr)
        {
            const int32 i = devices.Count();
            devices.Resize(i + 1);
            auto& device = devices[i];

            device.InternalName = devicesStr;
            device.Name = String(device.InternalName).TrimTrailing();
            device.Name.Replace(TEXT("OpenAL Soft on "), TEXT(""));

            if (device.InternalName == defaultDeviceName)
            {
                activeDeviceIndex = i;
            }

            devicesStr += (device.InternalName.Length() + 1) * sizeof(ALCchar);
        }
        if (activeDeviceIndex == -1)
        {
            LOG(Warning, "Failed to pick a default device");
            LOG_STR(Warning, String(defaultDeviceName));
            for (int32 i = 0; i < devices.Count(); i++)
                LOG_STR(Warning, devices[i].Name);
            if (devices.IsEmpty())
                return true;
            LOG(Warning, "Using the first audio device");
            activeDeviceIndex = 0;
        }

        // Open default device
        if (ALC::Device)
            alcCloseDevice(ALC::Device);
        const auto& name = devices[activeDeviceIndex].InternalName;
        ALC::Device = alcOpenDevice(name.Get());
        if (ALC::Device == nullptr)
        {
            LOG(Warning, "Failed to open OpenAL device ({0}).", String(name));
            return true;
        }
    }
    else
#endif
    {
        if (ALC::Device)
        {
            // Single device
            devices.Resize(1);
            devices[0].Name = TEXT("Default device");
        }
        else
        {
            // No device
            devices.Resize(0);
        }
    }

    // Init
    // Create context and open initial device
    Base_OnActiveDeviceChanged();
    Audio::SetActiveDeviceIndexSilent(activeDeviceIndex);
#ifdef AL_SOFT_source_spatialize
    if (ALC::IsExtensionSupported("AL_SOFT_source_spatialize"))
        ALC::Features = EnumAddFlags(ALC::Features, FeatureFlags::SpatialMultiChannel);
#endif
#if !PLATFORM_WEB
    ALC::Features = EnumAddFlags(ALC::Features, FeatureFlags::HRTF);
#endif
    Base_SetDopplerFactor(AudioSettings::Get()->DopplerFactor);
    ALC::Inited = true;

    // Log service info
    LOG(Info, "{0} ({1})", String(alGetString(AL_RENDERER)), String(alGetString(AL_VERSION)));
    for (int32 i = 0; i < devices.Count(); i++)
    {
        LOG(Info, "{0}{1}", i == activeDeviceIndex ? TEXT("[active] ") : TEXT(""), devices[i].Name);
    }

    return false;
}

#ifndef ALC_CONNECTED
#define ALC_CONNECTED 0x313
#endif

void AudioBackendOAL::Base_Update()
{
    // Check if active audio device got disconnected or invalidated (WASAPI AUDCLNT_E_DEVICE_INVALIDATED 0x88890004)
    bool forceCheck = false;
    if (ALC::Device != nullptr)
    {
        ALCint connected = ALC_TRUE;
        alcGetIntegerv(ALC::Device, ALC_CONNECTED, 1, &connected);
        if (connected == ALC_FALSE || alcGetError(ALC::Device) != ALC_NO_ERROR)
        {
            LOG(Warning, "Active audio device disconnected. Swapping active audio device...");
            forceCheck = true;
        }
    }

#if ALC_ENUMERATE_ALL_EXT

    // Update audio devices list periodically (throttled every 0.25 seconds / 250ms, or immediately if disconnected)
    static float deviceCheckTimer = 0.0f;
    static StringAnsi lastDefaultDeviceName;
    deviceCheckTimer += Time::Update.UnscaledDeltaTime.GetTotalSeconds();
    if (deviceCheckTimer >= 0.25f || forceCheck)
    {
        deviceCheckTimer = 0.0f;
        if (ALC::IsExtensionSupported("ALC_ENUMERATE_ALL_EXT"))
        {
            const ALCchar* defaultDevice = alcGetString(nullptr, ALC_DEFAULT_ALL_DEVICES_SPECIFIER);
            const StringAnsi defaultDeviceName(defaultDevice);
            Audio::SetSystemDefaultDeviceName(defaultDeviceName);
            const ALCchar* devicesStr = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
            Array<AudioDevice> newDevices;
            int32 defaultDeviceIndex = -1;

            while (devicesStr && *devicesStr)
            {
                const int32 i = newDevices.Count();
                auto& device = newDevices.AddOne();
                device.InternalName = devicesStr;
                device.Name = String(device.InternalName).TrimTrailing();
                device.Name.Replace(TEXT("OpenAL Soft on "), TEXT(""));

                if (device.InternalName == defaultDeviceName)
                {
                    defaultDeviceIndex = i;
                }

                devicesStr += (device.InternalName.Length() + 1) * sizeof(ALCchar);
            }

            if (defaultDeviceIndex == -1 && !newDevices.IsEmpty())
                defaultDeviceIndex = 0;

            bool devicesListChanged = newDevices.Count() != Audio::Devices.Count();
            if (!devicesListChanged)
            {
                for (int32 i = 0; i < newDevices.Count(); i++)
                {
                    if (newDevices[i].InternalName != Audio::Devices[i].InternalName)
                    {
                        devicesListChanged = true;
                        break;
                    }
                }
            }

            const int32 currentActiveIndex = Audio::GetActiveDeviceIndex();
            const StringAnsi& explicitDeviceName = Audio::GetExplicitDeviceName();

            // Determine the internal name of the currently active device.
            // When in System Default Mode (index -1), fall back to the OS default name so
            // comparisons correctly identify "no change" and avoid spurious device swaps.
            StringAnsi currentActiveInternalName;
            if (currentActiveIndex >= 0 && currentActiveIndex < Audio::Devices.Count())
                currentActiveInternalName = Audio::Devices[currentActiveIndex].InternalName;
            else if (explicitDeviceName.IsEmpty())
                currentActiveInternalName = defaultDeviceName; // System Default Mode: treat current as OS default

            int32 currentActiveNewIndex = -1;
            int32 explicitDeviceNewIndex = -1;

            for (int32 i = 0; i < newDevices.Count(); i++)
            {
                if (!currentActiveInternalName.IsEmpty() && newDevices[i].InternalName == currentActiveInternalName)
                {
                    currentActiveNewIndex = i;
                }
                if (!explicitDeviceName.IsEmpty() && newDevices[i].InternalName == explicitDeviceName)
                {
                    explicitDeviceNewIndex = i;
                }
            }

            bool defaultDeviceChanged = (!lastDefaultDeviceName.IsEmpty() && lastDefaultDeviceName != defaultDeviceName);
            lastDefaultDeviceName = defaultDeviceName;

            int32 targetActiveIndex = -1;
            if (!explicitDeviceName.IsEmpty())
            {
                // Explicit Device Lock Mode: User selected a specific audio device
                if (explicitDeviceNewIndex != -1)
                {
                    // Explicit device is available (stay locked or re-claim explicit device)
                    targetActiveIndex = explicitDeviceNewIndex;
                }
                else if (forceCheck || currentActiveNewIndex == -1)
                {
                    // Explicit device disconnected -> Fallback to system default
                    targetActiveIndex = (defaultDeviceIndex != -1) ? defaultDeviceIndex : 0;
                }
                else
                {
                    targetActiveIndex = currentActiveNewIndex;
                }
            }
            else
            {
                // Dynamic System Default Mode: Automatically follow OS default device changes
                if (forceCheck)
                {
                    targetActiveIndex = (defaultDeviceIndex != -1) ? defaultDeviceIndex : 0;
                }
                else if (defaultDeviceChanged && defaultDeviceIndex != -1)
                {
                    targetActiveIndex = defaultDeviceIndex;
                }
                else if (currentActiveNewIndex != -1)
                {
                    targetActiveIndex = currentActiveNewIndex;
                }
                else if (defaultDeviceIndex != -1)
                {
                    targetActiveIndex = defaultDeviceIndex;
                }
            }

            if (devicesListChanged)
            {
                // Fire DeviceAdded for each device in newDevices not present in old list
                for (int32 i = 0; i < newDevices.Count(); i++)
                {
                    bool found = false;
                    for (int32 j = 0; j < Audio::Devices.Count(); j++)
                    {
                        if (Audio::Devices[j].InternalName == newDevices[i].InternalName)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        Audio::DeviceAdded();
                }

                // Fire DeviceRemoved for each device in old list not present in newDevices
                for (int32 i = 0; i < Audio::Devices.Count(); i++)
                {
                    bool found = false;
                    for (int32 j = 0; j < newDevices.Count(); j++)
                    {
                        if (newDevices[j].InternalName == Audio::Devices[i].InternalName)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        Audio::DeviceRemoved();
                }

                Audio::Devices = newDevices;
                Audio::DevicesChanged();
            }

            if (targetActiveIndex != -1)
            {
                StringAnsi targetActiveInternalName = newDevices[targetActiveIndex].InternalName;
                if (forceCheck)
                {
                    Audio::SetActiveDeviceIndexSilent(targetActiveIndex);
                    Base_OnActiveDeviceChanged();
                }
                else if (targetActiveInternalName != currentActiveInternalName)
                {
                    Audio::SetActiveDeviceIndexSilent(targetActiveIndex);
                    Base_OnActiveDeviceChanged();
                }
                else
                {
                    Audio::SetActiveDeviceIndexSilent(targetActiveIndex);
                }
            }
        }
    }
#endif
}

void AudioBackendOAL::Base_Dispose()
{
    if (ALC::Device != nullptr)
    {
        alcCloseDevice(ALC::Device);
        ALC::Device = nullptr;
    }
}

#endif
