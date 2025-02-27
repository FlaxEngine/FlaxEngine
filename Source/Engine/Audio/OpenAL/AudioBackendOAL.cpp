// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if AUDIO_API_OPENAL

#include "AudioBackendOAL.h"
#include "Engine/Platform/StringUtils.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Tools/AudioTool/AudioTool.h"
#include "Engine/Engine/Units.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Audio/Audio.h"
#include "Engine/Audio/AudioListener.h"
#include "Engine/Audio/AudioSource.h"
#include "Engine/Audio/AudioSettings.h"

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
        bool Spatial;
    };

    ALCdevice* Device = nullptr;
    ALCcontext* Context = nullptr;
    AudioBackend::FeatureFlags Features = AudioBackend::FeatureFlags::None;
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
        void Rebuild(uint32& sourceID, const Vector3& position, const Quaternion& orientation, float volume, float pitch, float pan, bool loop, bool spatial, float attenuation, float minDistance, float doppler)
        {
            ASSERT_LOW_LAYER(sourceID == 0);
            alGenSources(1, &sourceID);
            ASSERT_LOW_LAYER(sourceID != 0);

            alSourcef(sourceID, AL_GAIN, volume);
            alSourcef(sourceID, AL_PITCH, pitch);
            alSourcef(sourceID, AL_SEC_OFFSET, 0.0f);
            alSourcei(sourceID, AL_LOOPING, loop);
            alSourcei(sourceID, AL_SOURCE_RELATIVE, !spatial); // Non-spatial sounds use AL_POSITION for panning
            alSourcei(sourceID, AL_BUFFER, 0);
#ifdef AL_SOFT_source_spatialize
            alSourcei(sourceID, AL_SOURCE_SPATIALIZE_SOFT, AL_TRUE); // Always spatialize, fixes multi-channel played as spatial
#endif
            if (spatial)
            {
                alSourcef(sourceID, AL_ROLLOFF_FACTOR, attenuation);
                alSourcef(sourceID, AL_DOPPLER_FACTOR, doppler);
                alSourcef(sourceID, AL_REFERENCE_DISTANCE, FLAX_DST_TO_OAL(minDistance));
                alSource3f(sourceID, AL_POSITION, FLAX_POS_TO_OAL(position));
                alSource3f(sourceID, AL_VELOCITY, FLAX_VEL_TO_OAL(Vector3::Zero));
#ifdef AL_EXT_STEREO_ANGLES
                const float panAngle = pan * PI_HALF;
                const ALfloat panAngles[2] = { (ALfloat)(PI / 6.0 - panAngle), (ALfloat)(-PI / 6.0 - panAngle) }; // Angles are specified counter-clockwise in radians
                alSourcefv(sourceID, AL_STEREO_ANGLES, panAngles);
#endif
            }
            else
            {
                alSourcef(sourceID, AL_ROLLOFF_FACTOR, 0.0f);
                alSourcef(sourceID, AL_DOPPLER_FACTOR, 1.0f);
                alSourcef(sourceID, AL_REFERENCE_DISTANCE, 0.0f);
                alSource3f(sourceID, AL_POSITION, pan, 0, -sqrtf(1.0f - pan * pan));
                alSource3f(sourceID, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
            }
        }
    }

    struct AudioSourceState
    {
        AudioSource::States State;
        float Time;
    };

    void RebuildContext(const Array<AudioSourceState>& states)
    {
        LOG(Info, "Rebuilding audio contexts");

        ClearContext();

        if (Device == nullptr)
            return;

        ALCint attrsHrtf[] = { ALC_HRTF_SOFT, ALC_TRUE };
        const ALCint* attrList = nullptr;
        if (Audio::GetEnableHRTF())
        {
            LOG(Info, "Enabling OpenAL HRTF");
            attrList = attrsHrtf;
        }

        Context = alcCreateContext(Device, attrList);
        alcMakeContextCurrent(Context);

        for (AudioListener* listener : Audio::Listeners)
            Listener::Rebuild(listener);

        for (int32 i = 0; i < states.Count(); i++)
        {
            AudioSource* source = Audio::Sources[i];
            Source::Rebuild(source->SourceID, source->GetPosition(), source->GetOrientation(), source->GetVolume(), source->GetPitch(), source->GetPan(), source->GetIsLooping() && !source->UseStreaming(), source->Is3D(), source->GetAttenuation(), source->GetMinDistance(), source->GetDopplerFactor());

            if (states.HasItems())
            {
                // Restore playback state
                auto& state = states[i];
                if (state.State != AudioSource::States::Stopped)
                    source->Play();
                if (state.State == AudioSource::States::Paused)
                    source->Pause();
                if (state.State != AudioSource::States::Stopped)
                    source->SetTime(state.Time);
            }
        }
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
    uint32 sourceID = 0;
    ALC::Source::Rebuild(sourceID, position, orientation, volume, pitch, pan, loop, spatial, attenuation, minDistance, doppler);

    // Cache audio data format assigned on source (used in Source_GetCurrentBufferTime)
    ALC::Locker.Lock();
    auto& data = ALC::SourcesData[sourceID];
    data.Format = format;
    data.Spatial = spatial;
    ALC::Locker.Unlock();

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
    const bool spatial = ALC::SourcesData[sourceID].Spatial;
    ALC::Locker.Unlock();
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

void AudioBackendOAL::Source_IsLoopingChanged(uint32 sourceID, bool loop)
{
    alSourcei(sourceID, AL_LOOPING, loop);
}

void AudioBackendOAL::Source_SpatialSetupChanged(uint32 sourceID, bool spatial, float attenuation, float minDistance, float doppler)
{
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
        LOG(Error, "Not suppported audio data format for OpenAL device: BitDepth={}, NumChannels={}", info.BitDepth, info.NumChannels);
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
    const StringAnsi& name = Audio::GetActiveDevice()->InternalName;
    ALC::Device = alcOpenDevice(name.Get());
    if (ALC::Device == nullptr)
    {
        LOG(Fatal, "Failed to open OpenAL device ({0}).", String(name));
        return;
    }

    // Setup
    ALC::RebuildContext(states);
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
    Base_SetDopplerFactor(AudioSettings::Get()->DopplerFactor);
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED); // Default attenuation model
    int32 clampedIndex = Math::Clamp(activeDeviceIndex, -1, Audio::Devices.Count() - 1);
    if (clampedIndex == Audio::GetActiveDeviceIndex())
    {
        ALC::RebuildContext(true);
    }
    Audio::SetActiveDeviceIndex(activeDeviceIndex);
#ifdef AL_SOFT_source_spatialize
    if (ALC::IsExtensionSupported("AL_SOFT_source_spatialize"))
        ALC::Features = EnumAddFlags(ALC::Features, FeatureFlags::SpatialMultiChannel);
#endif

    // Log service info
    LOG(Info, "{0} ({1})", String(alGetString(AL_RENDERER)), String(alGetString(AL_VERSION)));
    for (int32 i = 0; i < devices.Count(); i++)
    {
        LOG(Info, "{0}{1}", i == activeDeviceIndex ? TEXT("[active] ") : TEXT(""), devices[i].Name);
    }

    return false;
}

void AudioBackendOAL::Base_Update()
{
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
