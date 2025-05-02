// Copyright (c) Wojciech Figat. All rights reserved.

#if AUDIO_API_NONE

#include "AudioBackendNone.h"
#include "Engine/Audio/Audio.h"
#include "Engine/Audio/AudioSource.h"

void AudioBackendNone::Listener_Reset()
{
}

void AudioBackendNone::Listener_VelocityChanged(const Vector3& velocity)
{
}

void AudioBackendNone::Listener_TransformChanged(const Vector3& position, const Quaternion& orientation)
{
}

void AudioBackendNone::Listener_ReinitializeAll()
{
}

uint32 AudioBackendNone::Source_Add(const AudioDataInfo& format, const Vector3& position, const Quaternion& orientation, float volume, float pitch, float pan, bool loop, bool spatial, float attenuation, float minDistance, float doppler)
{
    return 1;
}

void AudioBackendNone::Source_Remove(uint32 sourceID)
{
}

void AudioBackendNone::Source_VelocityChanged(uint32 sourceID, const Vector3& velocity)
{
}

void AudioBackendNone::Source_TransformChanged(uint32 sourceID, const Vector3& position, const Quaternion& orientation)
{
}

void AudioBackendNone::Source_VolumeChanged(uint32 sourceID, float volume)
{
}

void AudioBackendNone::Source_PitchChanged(uint32 sourceID, float pitch)
{
}

void AudioBackendNone::Source_PanChanged(uint32 sourceID, float pan)
{
}

void AudioBackendNone::Source_IsLoopingChanged(uint32 sourceID, bool loop)
{
}

void AudioBackendNone::Source_SpatialSetupChanged(uint32 sourceID, bool spatial, float attenuation, float minDistance, float doppler)
{
}

void AudioBackendNone::Source_Play(uint32 sourceID)
{
}

void AudioBackendNone::Source_Pause(uint32 sourceID)
{
}

void AudioBackendNone::Source_Stop(uint32 sourceID)
{
}

void AudioBackendNone::Source_SetCurrentBufferTime(uint32 sourceID, float value)
{
}

float AudioBackendNone::Source_GetCurrentBufferTime(uint32 sourceID)
{
    return 0.0f;
}

void AudioBackendNone::Source_SetNonStreamingBuffer(uint32 sourceID, uint32 bufferID)
{
}

void AudioBackendNone::Source_GetProcessedBuffersCount(uint32 sourceID, int32& processedBuffersCount)
{
    processedBuffersCount = 0;
}

void AudioBackendNone::Source_GetQueuedBuffersCount(uint32 sourceID, int32& queuedBuffersCount)
{
}

void AudioBackendNone::Source_QueueBuffer(uint32 sourceID, uint32 bufferID)
{
}

void AudioBackendNone::Source_DequeueProcessedBuffers(uint32 sourceID)
{
}

uint32 AudioBackendNone::Buffer_Create()
{
    return 1;
}

void AudioBackendNone::Buffer_Delete(uint32 bufferID)
{
}

void AudioBackendNone::Buffer_Write(uint32 bufferID, byte* samples, const AudioDataInfo& info)
{
}

const Char* AudioBackendNone::Base_Name()
{
    return TEXT("None");
}

AudioBackend::FeatureFlags AudioBackendNone::Base_Features()
{
    return FeatureFlags::None;
}

void AudioBackendNone::Base_OnActiveDeviceChanged()
{
}

void AudioBackendNone::Base_SetDopplerFactor(float value)
{
}

void AudioBackendNone::Base_SetVolume(float value)
{
}

bool AudioBackendNone::Base_Init()
{
    auto& devices = Audio::Devices;

    // Dummy device
    devices.Resize(1);
    devices[0].Name = TEXT("Dummy device");
    Audio::SetActiveDeviceIndex(0);

    return false;
}

void AudioBackendNone::Base_Update()
{
}

void AudioBackendNone::Base_Dispose()
{
}

#endif
