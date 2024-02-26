// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Config.h"
#include "Types.h"
#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// The helper class for that handles active audio backend operations.
/// </summary>
class AudioBackend
{
    friend AudioBackend;
    friend class AudioService;

public:

    enum class FeatureFlags
    {
        None = 0,
        // Supports multi-channel (incl. stereo) audio playback for spatial sources (3D), otherwise 3d audio needs to be in mono format.
        SpatialMultiChannel = 1,
    };

    static AudioBackend* Instance;

private:

    // Listener
    virtual void Listener_OnAdd(AudioListener* listener) = 0;
    virtual void Listener_OnRemove(AudioListener* listener) = 0;
    virtual void Listener_VelocityChanged(AudioListener* listener) = 0;
    virtual void Listener_TransformChanged(AudioListener* listener) = 0;
    virtual void Listener_ReinitializeAll() = 0;

    // Source
    virtual void Source_OnAdd(AudioSource* source) = 0;
    virtual void Source_OnRemove(AudioSource* source) = 0;
    virtual void Source_VelocityChanged(AudioSource* source) = 0;
    virtual void Source_TransformChanged(AudioSource* source) = 0;
    virtual void Source_VolumeChanged(AudioSource* source) = 0;
    virtual void Source_PitchChanged(AudioSource* source) = 0;
    virtual void Source_PanChanged(AudioSource* source) = 0;
    virtual void Source_IsLoopingChanged(AudioSource* source) = 0;
    virtual void Source_SpatialSetupChanged(AudioSource* source) = 0;
    virtual void Source_ClipLoaded(AudioSource* source) = 0;
    virtual void Source_Cleanup(AudioSource* source) = 0;
    virtual void Source_Play(AudioSource* source) = 0;
    virtual void Source_Pause(AudioSource* source) = 0;
    virtual void Source_Stop(AudioSource* source) = 0;
    virtual void Source_SetCurrentBufferTime(AudioSource* source, float value) = 0;
    virtual float Source_GetCurrentBufferTime(const AudioSource* source) = 0;
    virtual void Source_SetNonStreamingBuffer(AudioSource* source) = 0;
    virtual void Source_GetProcessedBuffersCount(AudioSource* source, int32& processedBuffersCount) = 0;
    virtual void Source_GetQueuedBuffersCount(AudioSource* source, int32& queuedBuffersCount) = 0;
    virtual void Source_QueueBuffer(AudioSource* source, uint32 bufferId) = 0;
    virtual void Source_DequeueProcessedBuffers(AudioSource* source) = 0;

    // Buffer
    virtual uint32 Buffer_Create() = 0;
    virtual void Buffer_Delete(uint32 bufferId) = 0;
    virtual void Buffer_Write(uint32 bufferId, byte* samples, const AudioDataInfo& info) = 0;

    // Base
    virtual const Char* Base_Name() = 0;
    virtual FeatureFlags Base_Features() = 0;
    virtual void Base_OnActiveDeviceChanged() = 0;
    virtual void Base_SetDopplerFactor(float value) = 0;
    virtual void Base_SetVolume(float value) = 0;
    virtual bool Base_Init() = 0;
    virtual void Base_Update() = 0;
    virtual void Base_Dispose() = 0;

public:

    virtual ~AudioBackend()
    {
    }

public:

    class Listener
    {
    public:

        FORCE_INLINE static void OnAdd(AudioListener* listener)
        {
            Instance->Listener_OnAdd(listener);
        }

        FORCE_INLINE static void OnRemove(AudioListener* listener)
        {
            Instance->Listener_OnRemove(listener);
        }

        FORCE_INLINE static void VelocityChanged(AudioListener* listener)
        {
            Instance->Listener_VelocityChanged(listener);
        }

        FORCE_INLINE static void TransformChanged(AudioListener* listener)
        {
            Instance->Listener_TransformChanged(listener);
        }

        FORCE_INLINE static void ReinitializeAll()
        {
            Instance->Listener_ReinitializeAll();
        }
    };

    class Source
    {
    public:

        FORCE_INLINE static void OnAdd(AudioSource* source)
        {
            Instance->Source_OnAdd(source);
        }

        FORCE_INLINE static void OnRemove(AudioSource* source)
        {
            Instance->Source_OnRemove(source);
        }

        FORCE_INLINE static void VelocityChanged(AudioSource* source)
        {
            Instance->Source_VelocityChanged(source);
        }

        FORCE_INLINE static void TransformChanged(AudioSource* source)
        {
            Instance->Source_TransformChanged(source);
        }

        FORCE_INLINE static void VolumeChanged(AudioSource* source)
        {
            Instance->Source_VolumeChanged(source);
        }

        FORCE_INLINE static void PitchChanged(AudioSource* source)
        {
            Instance->Source_PitchChanged(source);
        }

        FORCE_INLINE static void PanChanged(AudioSource* source)
        {
            Instance->Source_PanChanged(source);
        }

        FORCE_INLINE static void IsLoopingChanged(AudioSource* source)
        {
            Instance->Source_IsLoopingChanged(source);
        }

        FORCE_INLINE static void SpatialSetupChanged(AudioSource* source)
        {
            Instance->Source_SpatialSetupChanged(source);
        }

        FORCE_INLINE static void ClipLoaded(AudioSource* source)
        {
            Instance->Source_ClipLoaded(source);
        }

        FORCE_INLINE static void Cleanup(AudioSource* source)
        {
            Instance->Source_Cleanup(source);
        }

        FORCE_INLINE static void Play(AudioSource* source)
        {
            Instance->Source_Play(source);
        }

        FORCE_INLINE static void Pause(AudioSource* source)
        {
            Instance->Source_Pause(source);
        }

        FORCE_INLINE static void Stop(AudioSource* source)
        {
            Instance->Source_Stop(source);
        }

        FORCE_INLINE static void SetCurrentBufferTime(AudioSource* source, float value)
        {
            Instance->Source_SetCurrentBufferTime(source, value);
        }

        FORCE_INLINE static float GetCurrentBufferTime(const AudioSource* source)
        {
            return Instance->Source_GetCurrentBufferTime(source);
        }

        FORCE_INLINE static void SetNonStreamingBuffer(AudioSource* source)
        {
            Instance->Source_SetNonStreamingBuffer(source);
        }

        FORCE_INLINE static void GetProcessedBuffersCount(AudioSource* source, int32& processedBuffersCount)
        {
            Instance->Source_GetProcessedBuffersCount(source, processedBuffersCount);
        }

        FORCE_INLINE static void GetQueuedBuffersCount(AudioSource* source, int32& queuedBuffersCount)
        {
            Instance->Source_GetQueuedBuffersCount(source, queuedBuffersCount);
        }

        FORCE_INLINE static void QueueBuffer(AudioSource* source, uint32 bufferId)
        {
            Instance->Source_QueueBuffer(source, bufferId);
        }

        FORCE_INLINE static void DequeueProcessedBuffers(AudioSource* source)
        {
            Instance->Source_DequeueProcessedBuffers(source);
        }
    };

    class Buffer
    {
    public:

        FORCE_INLINE static uint32 Create()
        {
            return Instance->Buffer_Create();
        }

        FORCE_INLINE static void Delete(uint32 bufferId)
        {
            Instance->Buffer_Delete(bufferId);
        }

        FORCE_INLINE static void Write(uint32 bufferId, byte* samples, const AudioDataInfo& info)
        {
            Instance->Buffer_Write(bufferId, samples, info);
        }
    };

    FORCE_INLINE static const Char* Name()
    {
        return Instance->Base_Name();
    }

    FORCE_INLINE static FeatureFlags Features()
    {
        return Instance->Base_Features();
    }

    FORCE_INLINE static void OnActiveDeviceChanged()
    {
        Instance->Base_OnActiveDeviceChanged();
    }

    FORCE_INLINE static void SetDopplerFactor(float value)
    {
        Instance->Base_SetDopplerFactor(value);
    }

    FORCE_INLINE static void SetVolume(float value)
    {
        return Instance->Base_SetVolume(value);
    }

    FORCE_INLINE static bool Init()
    {
        return Instance->Base_Init();
    }

    FORCE_INLINE static void Update()
    {
        Instance->Base_Update();
    }

    FORCE_INLINE static void Dispose()
    {
        Instance->Base_Dispose();
    }
};
