// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if AUDIO_API_NONE

#include "AudioBackendNone.h"
#include "Engine/Audio/Audio.h"
#include "Engine/Audio/AudioSource.h"

void AudioBackendNone::Listener_OnAdd(AudioListener* listener)
{
}

void AudioBackendNone::Listener_OnRemove(AudioListener* listener)
{
}

void AudioBackendNone::Listener_VelocityChanged(AudioListener* listener)
{
}

void AudioBackendNone::Listener_TransformChanged(AudioListener* listener)
{
}

void AudioBackendNone::Listener_ReinitializeAll()
{
}

void AudioBackendNone::Source_OnAdd(AudioSource* source)
{
    source->Restore();
}

void AudioBackendNone::Source_OnRemove(AudioSource* source)
{
    source->Cleanup();
}

void AudioBackendNone::Source_VelocityChanged(AudioSource* source)
{
}

void AudioBackendNone::Source_TransformChanged(AudioSource* source)
{
}

void AudioBackendNone::Source_VolumeChanged(AudioSource* source)
{
}

void AudioBackendNone::Source_PitchChanged(AudioSource* source)
{
}

void AudioBackendNone::Source_PanChanged(AudioSource* source)
{
}

void AudioBackendNone::Source_IsLoopingChanged(AudioSource* source)
{
}

void AudioBackendNone::Source_SpatialSetupChanged(AudioSource* source)
{
}

void AudioBackendNone::Source_ClipLoaded(AudioSource* source)
{
}

void AudioBackendNone::Source_Cleanup(AudioSource* source)
{
}

void AudioBackendNone::Source_Play(AudioSource* source)
{
}

void AudioBackendNone::Source_Pause(AudioSource* source)
{
}

void AudioBackendNone::Source_Stop(AudioSource* source)
{
}

void AudioBackendNone::Source_SetCurrentBufferTime(AudioSource* source, float value)
{
}

float AudioBackendNone::Source_GetCurrentBufferTime(const AudioSource* source)
{
    return 0.0f;
}

void AudioBackendNone::Source_SetNonStreamingBuffer(AudioSource* source)
{
}

void AudioBackendNone::Source_GetProcessedBuffersCount(AudioSource* source, int32& processedBuffersCount)
{
    processedBuffersCount = 0;
}

void AudioBackendNone::Source_GetQueuedBuffersCount(AudioSource* source, int32& queuedBuffersCount)
{
}

void AudioBackendNone::Source_QueueBuffer(AudioSource* source, uint32 bufferId)
{
}

void AudioBackendNone::Source_DequeueProcessedBuffers(AudioSource* source)
{
}

uint32 AudioBackendNone::Buffer_Create()
{
    return 1;
}

void AudioBackendNone::Buffer_Delete(uint32 bufferId)
{
}

void AudioBackendNone::Buffer_Write(uint32 bufferId, byte* samples, const AudioDataInfo& info)
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
