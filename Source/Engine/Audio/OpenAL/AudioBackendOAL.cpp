// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if AUDIO_API_OPENAL

#include "AudioBackendOAL.h"
#include "Engine/Platform/StringUtils.h"
#include "Engine/Core/Log.h"
#include "Engine/Tools/AudioTool/AudioTool.h"
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

#define ALC_MULTIPLE_LISTENERS 0

#define FLAX_COORD_SCALE 0.01f // units are meters
#define FLAX_DST_TO_OAL(x) x * FLAX_COORD_SCALE
#define FLAX_POS_TO_OAL(vec) ((ALfloat)vec.X * -FLAX_COORD_SCALE), ((ALfloat)vec.Y * FLAX_COORD_SCALE), ((ALfloat)vec.Z * FLAX_COORD_SCALE)
#define FLAX_VEL_TO_OAL(vec) ((ALfloat)vec.X * -(FLAX_COORD_SCALE*FLAX_COORD_SCALE)), ((ALfloat)vec.Y * (FLAX_COORD_SCALE*FLAX_COORD_SCALE)), ((ALfloat)vec.Z * (FLAX_COORD_SCALE*FLAX_COORD_SCALE))
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

#if ALC_MULTIPLE_LISTENERS
#define ALC_FOR_EACH_CONTEXT() \
    for (int32 i = 0; i < Contexts.Count(); i++)
    { \
        if (Contexts.Count() > 1) \
            alcMakeContextCurrent(Contexts[i]);
#define ALC_GET_DEFAULT_CONTEXT() \
    if (Contexts.Count() > 1) \
        alcMakeContextCurrent(Contexts[0]);
#define ALC_GET_LISTENER_CONTEXT(listener) \
    if (Contexts.Count() > 1) \
        alcMakeContextCurrent(ALC::GetContext(listener)));
#else
#define ALC_FOR_EACH_CONTEXT() { int32 i = 0;
#define ALC_GET_DEFAULT_CONTEXT()
#define ALC_GET_LISTENER_CONTEXT(listener)
#endif

namespace ALC
{
    ALCdevice* Device = nullptr;
    Array<ALCcontext*, FixedAllocation<AUDIO_MAX_LISTENERS>> Contexts;
    AudioBackend::FeatureFlags Features = AudioBackend::FeatureFlags::None;

    bool IsExtensionSupported(const char* extension)
    {
        if (Device == nullptr)
            return false;

        const int32 length = StringUtils::Length(extension);
        if ((length > 2) && (StringUtils::Compare(extension, "ALC", 3) == 0))
            return alcIsExtensionPresent(Device, extension) != AL_FALSE;
        return alIsExtensionPresent(extension) != AL_FALSE;
    }

    ALCcontext* GetContext(const class AudioListener* listener)
    {
#if ALC_MULTIPLE_LISTENERS
        const auto& listeners = Audio::Listeners;
        if (listeners.HasItems())
        {
            ASSERT(listeners.Count() == Contexts.Count());

            const int32 numContexts = Contexts.Count();
            ALC_FOR_EACH_CONTEXT()
            {
                if (listeners[i] == listener)
                    return Contexts[i];
            }
        }
        ASSERT(Contexts.HasItems());
#else
        ASSERT(Contexts.Count() == 1);
#endif
        return Contexts[0];
    }

    FORCE_INLINE const Array<ALCcontext*, FixedAllocation<AUDIO_MAX_LISTENERS>>& GetContexts()
    {
        return Contexts;
    }

    void ClearContexts()
    {
        alcMakeContextCurrent(nullptr);

        for (ALCcontext* context : Contexts)
            alcDestroyContext(context);
        Contexts.Clear();
    }

    namespace Listener
    {
        void Rebuild(AudioListener* listener)
        {
            AudioBackend::Listener::TransformChanged(listener);

            const Vector3 velocity = listener->GetVelocity();
            alListener3f(AL_VELOCITY, FLAX_VEL_TO_OAL(velocity));
            alListenerf(AL_GAIN, Audio::GetVolume());
        }
    }

    namespace Source
    {
        void Rebuild(AudioSource* source)
        {
            ASSERT(source->SourceIDs.IsEmpty());
            const bool is3D = source->Is3D();
            const bool loop = source->GetIsLooping() && !source->UseStreaming();

            ALC_FOR_EACH_CONTEXT()
                uint32 sourceID = 0;
                alGenSources(1, &sourceID);

                source->SourceIDs.Add(sourceID);
            }

            ALC_FOR_EACH_CONTEXT()
                const uint32 sourceID = source->SourceIDs[i];

                alSourcef(sourceID, AL_GAIN, source->GetVolume());
                alSourcef(sourceID, AL_PITCH, source->GetPitch());
                alSourcef(sourceID, AL_SEC_OFFSET, 0.0f);
                alSourcei(sourceID, AL_LOOPING, loop);
                alSourcei(sourceID, AL_SOURCE_RELATIVE, !is3D);
                alSourcei(sourceID, AL_BUFFER, 0);
                if (is3D)
                {
#ifdef AL_SOFT_source_spatialize
                    alSourcei(sourceID, AL_SOURCE_SPATIALIZE_SOFT, AL_TRUE);
#endif
                    alSourcef(sourceID, AL_ROLLOFF_FACTOR, source->GetAttenuation());
                    alSourcef(sourceID, AL_DOPPLER_FACTOR, source->GetDopplerFactor());
                    alSourcef(sourceID, AL_REFERENCE_DISTANCE, FLAX_DST_TO_OAL(source->GetMinDistance()));
                    alSource3f(sourceID, AL_POSITION, FLAX_POS_TO_OAL(source->GetPosition()));
                    alSource3f(sourceID, AL_VELOCITY, FLAX_VEL_TO_OAL(source->GetVelocity()));
                }
                else
                {
                    alSourcef(sourceID, AL_ROLLOFF_FACTOR, 0.0f);
                    alSourcef(sourceID, AL_DOPPLER_FACTOR, 1.0f);
                    alSourcef(sourceID, AL_REFERENCE_DISTANCE, 0.0f);
                    alSource3f(sourceID, AL_POSITION, 0.0f, 0.0f, 0.0f);
                    alSource3f(sourceID, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
                }
#ifdef AL_EXT_STEREO_ANGLES
                const float panAngle = source->GetPan() * PI_HALF;
                const ALfloat panAngles[2] = { (ALfloat)(PI / 6.0 - panAngle), (ALfloat)(-PI / 6.0 - panAngle) }; // Angles are specified counter-clockwise in radians
                alSourcefv(sourceID, AL_STEREO_ANGLES, panAngles);
#endif
            }

            // Restore state after Cleanup
            source->Restore();
        }
    }

    void RebuildContexts(bool isChangingDevice)
    {
        LOG(Info, "Rebuilding audio contexts");

        if (!isChangingDevice)
        {
            for (AudioSource* source : Audio::Sources)
                source->Cleanup();
        }

        ClearContexts();

        if (Device == nullptr)
            return;

        ALCint attrsHrtf[] = { ALC_HRTF_SOFT, ALC_TRUE };
        const ALCint* attrList = nullptr;
        if (Audio::GetEnableHRTF())
        {
            LOG(Info, "Enabling OpenAL HRTF");
            attrList = attrsHrtf;
        }

#if ALC_MULTIPLE_LISTENERS
        const int32 numListeners = Audio::Listeners.Count();
        const int32 numContexts = numListeners > 1 ? numListeners : 1;
        Contexts.Resize(numContexts);

        ALC_FOR_EACH_CONTEXT()
            ALCcontext* context = alcCreateContext(Device, attrList);
            Contexts[i] = context;
        }
#else
        Contexts.Resize(1);
        Contexts[0] = alcCreateContext(Device, attrList);
#endif

        // If only one context is available keep it active as an optimization.
        // Audio listeners and sources will avoid excessive context switching in such case.
        alcMakeContextCurrent(Contexts[0]);

        for (AudioListener* listener : Audio::Listeners)
            Listener::Rebuild(listener);

        for (AudioSource* source : Audio::Sources)
            Source::Rebuild(source);
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

void AudioBackendOAL::Listener_OnAdd(AudioListener* listener)
{
#if ALC_MULTIPLE_LISTENERS
    ALC::RebuildContexts(false);
#else
    AudioBackend::Listener::TransformChanged(listener);
    const Vector3 velocity = listener->GetVelocity();
    alListener3f(AL_VELOCITY, FLAX_VEL_TO_OAL(velocity));
    alListenerf(AL_GAIN, Audio::GetVolume());
#endif
}

void AudioBackendOAL::Listener_OnRemove(AudioListener* listener)
{
#if ALC_MULTIPLE_LISTENERS
    ALC::RebuildContexts(false);
#endif
}

void AudioBackendOAL::Listener_VelocityChanged(AudioListener* listener)
{
    ALC_GET_LISTENER_CONTEXT(listener)

    const Vector3 velocity = listener->GetVelocity();
    alListener3f(AL_VELOCITY, FLAX_VEL_TO_OAL(velocity));
}

void AudioBackendOAL::Listener_TransformChanged(AudioListener* listener)
{
    ALC_GET_LISTENER_CONTEXT(listener)

    const Vector3 position = listener->GetPosition();
    const Quaternion orientation = listener->GetOrientation();
    const Vector3 flipX(-1, 1, 1);
    const Vector3 alOrientation[2] =
    {
        // Forward
        orientation * Vector3::Forward * flipX,
        // Up
        orientation * Vector3::Up * flipX
    };

    alListenerfv(AL_ORIENTATION, (float*)alOrientation);
    alListener3f(AL_POSITION, FLAX_POS_TO_OAL(position));
}

void AudioBackendOAL::Listener_ReinitializeAll()
{
    ALC::RebuildContexts(false);
}

void AudioBackendOAL::Source_OnAdd(AudioSource* source)
{
    ALC::Source::Rebuild(source);
}

void AudioBackendOAL::Source_OnRemove(AudioSource* source)
{
    source->Cleanup();
}

void AudioBackendOAL::Source_VelocityChanged(AudioSource* source)
{
    if (!source->Is3D())
        return;
    ALC_FOR_EACH_CONTEXT()
        const uint32 sourceID = source->SourceIDs[i];
        alSource3f(sourceID, AL_VELOCITY, FLAX_VEL_TO_OAL(source->GetVelocity()));
    }
}

void AudioBackendOAL::Source_TransformChanged(AudioSource* source)
{
    if (!source->Is3D())
        return;
    ALC_FOR_EACH_CONTEXT()
        const uint32 sourceID = source->SourceIDs[i];
        alSource3f(sourceID, AL_POSITION, FLAX_POS_TO_OAL(source->GetPosition()));
    }
}

void AudioBackendOAL::Source_VolumeChanged(AudioSource* source)
{
    ALC_FOR_EACH_CONTEXT()
        const uint32 sourceID = source->SourceIDs[i];
        alSourcef(sourceID, AL_GAIN, source->GetVolume());
    }
}

void AudioBackendOAL::Source_PitchChanged(AudioSource* source)
{
    ALC_FOR_EACH_CONTEXT()
        const uint32 sourceID = source->SourceIDs[i];
        alSourcef(sourceID, AL_PITCH, source->GetPitch());
    }
}

void AudioBackendOAL::Source_PanChanged(AudioSource* source)
{
#ifdef AL_EXT_STEREO_ANGLES
    const float panAngle = source->GetPan() * PI_HALF;
    const ALfloat panAngles[2] = { (ALfloat)(PI / 6.0 - panAngle), (ALfloat)(-PI / 6.0 - panAngle) }; // Angles are specified counter-clockwise in radians
    ALC_FOR_EACH_CONTEXT()
        const uint32 sourceID = source->SourceIDs[i];
        alSourcefv(sourceID, AL_STEREO_ANGLES, panAngles);
    }
#endif
}

void AudioBackendOAL::Source_IsLoopingChanged(AudioSource* source)
{
    const bool loop = source->GetIsLooping() && !source->UseStreaming();
    ALC_FOR_EACH_CONTEXT()
        const uint32 sourceID = source->SourceIDs[i];
        alSourcei(sourceID, AL_LOOPING, loop);
    }
}

void AudioBackendOAL::Source_SpatialSetupChanged(AudioSource* source)
{
    const bool is3D = source->Is3D();
    ALC_FOR_EACH_CONTEXT()
        const uint32 sourceID = source->SourceIDs[i];
        alSourcei(sourceID, AL_SOURCE_RELATIVE, !is3D);
        if (is3D)
        {
#ifdef AL_SOFT_source_spatialize
            alSourcei(sourceID, AL_SOURCE_SPATIALIZE_SOFT, AL_TRUE);
#endif
            alSourcef(sourceID, AL_ROLLOFF_FACTOR, source->GetAttenuation());
            alSourcef(sourceID, AL_DOPPLER_FACTOR, source->GetDopplerFactor());
            alSourcef(sourceID, AL_REFERENCE_DISTANCE, FLAX_DST_TO_OAL(source->GetMinDistance()));
        }
        else
        {
            alSourcef(sourceID, AL_ROLLOFF_FACTOR, 0.0f);
            alSourcef(sourceID, AL_DOPPLER_FACTOR, 1.0f);
            alSourcef(sourceID, AL_REFERENCE_DISTANCE, 0.0f);
        }
    }
}

void AudioBackendOAL::Source_ClipLoaded(AudioSource* source)
{
    if (source->SourceIDs.Count() < ALC::Contexts.Count())
        return;
    const auto clip = source->Clip.Get();
    const bool is3D = source->Is3D();
    const bool loop = source->GetIsLooping() && !clip->IsStreamable();

    ALC_FOR_EACH_CONTEXT()
        const uint32 sourceID = source->SourceIDs[i];
        alSourcei(sourceID, AL_SOURCE_RELATIVE, !is3D);
        alSourcei(sourceID, AL_LOOPING, loop);
    }
}

void AudioBackendOAL::Source_Cleanup(AudioSource* source)
{
    ALC_FOR_EACH_CONTEXT()
        const uint32 sourceID = source->SourceIDs[i];
        alSourcei(sourceID, AL_BUFFER, 0);
        ALC_CHECK_ERROR(alSourcei);
        alDeleteSources(1, &sourceID);
        ALC_CHECK_ERROR(alDeleteSources);
    }
}

void AudioBackendOAL::Source_Play(AudioSource* source)
{
    ALC_FOR_EACH_CONTEXT()
        const uint32 sourceID = source->SourceIDs[i];

        // Play
        alSourcePlay(sourceID);
        ALC_CHECK_ERROR(alSourcePlay);
    }
}

void AudioBackendOAL::Source_Pause(AudioSource* source)
{
    ALC_FOR_EACH_CONTEXT()
        const uint32 sourceID = source->SourceIDs[i];

        // Pause
        alSourcePause(sourceID);
        ALC_CHECK_ERROR(alSourcePause);
    }
}

void AudioBackendOAL::Source_Stop(AudioSource* source)
{
    ALC_FOR_EACH_CONTEXT()
        const uint32 sourceID = source->SourceIDs[i];

        // Stop and rewind
        alSourceRewind(sourceID);
        ALC_CHECK_ERROR(alSourceRewind);
        alSourcef(sourceID, AL_SEC_OFFSET, 0.0f);

        // Unset streaming buffers
        alSourcei(sourceID, AL_BUFFER, 0);
        ALC_CHECK_ERROR(alSourcei);
    }
}

void AudioBackendOAL::Source_SetCurrentBufferTime(AudioSource* source, float value)
{
    ALC_FOR_EACH_CONTEXT()
        const uint32 sourceID = source->SourceIDs[i];

        alSourcef(sourceID, AL_SEC_OFFSET, value);
    }
}

float AudioBackendOAL::Source_GetCurrentBufferTime(const AudioSource* source)
{
    ALC_GET_DEFAULT_CONTEXT()

#if 0
    float time;
    alGetSourcef(source->SourceIDs[0], AL_SEC_OFFSET, &time);
#else
    ASSERT(source->Clip && source->Clip->IsLoaded());
    const AudioDataInfo& clipInfo = source->Clip->AudioHeader.Info;
    ALint samplesPlayed;
    alGetSourcei(source->SourceIDs[0], AL_SAMPLE_OFFSET, &samplesPlayed);
    const uint32 totalSamples = clipInfo.NumSamples / clipInfo.NumChannels;
    const float time = (samplesPlayed % totalSamples) / static_cast<float>(Math::Max(1U, clipInfo.SampleRate));
#endif

    return time;
}

void AudioBackendOAL::Source_SetNonStreamingBuffer(AudioSource* source)
{
    const uint32 bufferId = source->Clip->Buffers[0];
    ALC_FOR_EACH_CONTEXT()
        const uint32 sourceID = source->SourceIDs[i];

        alSourcei(sourceID, AL_BUFFER, bufferId);
        ALC_CHECK_ERROR(alSourcei);
    }
}

void AudioBackendOAL::Source_GetProcessedBuffersCount(AudioSource* source, int32& processedBuffersCount)
{
    ALC_GET_DEFAULT_CONTEXT()

    // Check the first context only
    const uint32 sourceID = source->SourceIDs[0];
    alGetSourcei(sourceID, AL_BUFFERS_PROCESSED, &processedBuffersCount);
    ALC_CHECK_ERROR(alGetSourcei);
}

void AudioBackendOAL::Source_GetQueuedBuffersCount(AudioSource* source, int32& queuedBuffersCount)
{
    ALC_GET_DEFAULT_CONTEXT()

    // Check the first context only
    const uint32 sourceID = source->SourceIDs[0];
    alGetSourcei(sourceID, AL_BUFFERS_QUEUED, &queuedBuffersCount);
    ALC_CHECK_ERROR(alGetSourcei);
}

void AudioBackendOAL::Source_QueueBuffer(AudioSource* source, uint32 bufferId)
{
    ALC_FOR_EACH_CONTEXT()
        const uint32 sourceID = source->SourceIDs[i];

        // Queue new buffer
        alSourceQueueBuffers(sourceID, 1, &bufferId);
        ALC_CHECK_ERROR(alSourceQueueBuffers);
    }
}

void AudioBackendOAL::Source_DequeueProcessedBuffers(AudioSource* source)
{
    ALuint buffers[AUDIO_MAX_SOURCE_BUFFERS];
    ALC_FOR_EACH_CONTEXT()
        const uint32 sourceID = source->SourceIDs[i];

        int32 numProcessedBuffers;
        alGetSourcei(sourceID, AL_BUFFERS_PROCESSED, &numProcessedBuffers);
        alSourceUnqueueBuffers(sourceID, numProcessedBuffers, buffers);
        ALC_CHECK_ERROR(alSourceUnqueueBuffers);
    }
}

uint32 AudioBackendOAL::Buffer_Create()
{
    uint32 bufferId;
    alGenBuffers(1, &bufferId);
    ALC_CHECK_ERROR(alGenBuffers);
    return bufferId;
}

void AudioBackendOAL::Buffer_Delete(uint32 bufferId)
{
    alDeleteBuffers(1, &bufferId);
    ALC_CHECK_ERROR(alDeleteBuffers);
}

void AudioBackendOAL::Buffer_Write(uint32 bufferId, byte* samples, const AudioDataInfo& info)
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
                alBufferData(bufferId, format, sampleBufferFloat, bufferSize, info.SampleRate);
                ALC_CHECK_ERROR(alBufferData);
                Allocator::Free(sampleBufferFloat);
            }
            else
            {
                LOG(Warning, "OpenAL doesn't support bit depth larger than 16. Your audio data will be truncated.");
                const uint32 bufferSize = info.NumSamples * 2;
                byte* sampleBuffer16 = (byte*)Allocator::Allocate(bufferSize);
                AudioTool::ConvertBitDepth(samples, info.BitDepth, sampleBuffer16, 16, info.NumSamples);

                format = GetOpenALBufferFormat(info.NumChannels, 16);
                alBufferData(bufferId, format, sampleBuffer16, bufferSize, info.SampleRate);
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

            alBufferData(bufferId, format, sampleBuffer, bufferSize, info.SampleRate);
            ALC_CHECK_ERROR(alBufferData);
            Allocator::Free(sampleBuffer);
        }
        else if (format)
        {
            alBufferData(bufferId, format, samples, info.NumSamples * (info.BitDepth / 8), info.SampleRate);
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
            alBufferData(bufferId, format, sampleBuffer32, bufferSize, info.SampleRate);
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
            alBufferData(bufferId, format, sampleBuffer, bufferSize, info.SampleRate);
            ALC_CHECK_ERROR(alBufferData);

            Allocator::Free(sampleBuffer);
        }
        else if (format)
        {
            alBufferData(bufferId, format, samples, info.NumSamples * (info.BitDepth / 8), info.SampleRate);
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
    for (AudioSource* source : Audio::Sources)
        source->Cleanup();
    ALC::ClearContexts();
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
    ALC::RebuildContexts(true);
}

void AudioBackendOAL::Base_SetDopplerFactor(float value)
{
    alDopplerFactor(value);
}

void AudioBackendOAL::Base_SetVolume(float value)
{
    ALC_FOR_EACH_CONTEXT()
        alListenerf(AL_GAIN, value);
    }
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
        ALC::RebuildContexts(true);
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
