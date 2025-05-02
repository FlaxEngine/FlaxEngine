// Copyright (c) Wojciech Figat. All rights reserved.

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
    virtual void Listener_Reset() = 0;
    virtual void Listener_VelocityChanged(const Vector3& velocity) = 0;
    virtual void Listener_TransformChanged(const Vector3& position, const Quaternion& orientation) = 0;
    virtual void Listener_ReinitializeAll() = 0;

    // Source
    virtual uint32 Source_Add(const AudioDataInfo& format, const Vector3& position, const Quaternion& orientation, float volume, float pitch, float pan, bool loop, bool spatial, float attenuation, float minDistance, float doppler) = 0;
    virtual void Source_Remove(uint32 sourceID) = 0;
    virtual void Source_VelocityChanged(uint32 sourceID, const Vector3& velocity) = 0;
    virtual void Source_TransformChanged(uint32 sourceID, const Vector3& position, const Quaternion& orientation) = 0;
    virtual void Source_VolumeChanged(uint32 sourceID, float volume) = 0;
    virtual void Source_PitchChanged(uint32 sourceID, float pitch) = 0;
    virtual void Source_PanChanged(uint32 sourceID, float pan) = 0;
    virtual void Source_IsLoopingChanged(uint32 sourceID, bool loop) = 0;
    virtual void Source_SpatialSetupChanged(uint32 sourceID, bool spatial, float attenuation, float minDistance, float doppler) = 0;
    virtual void Source_Play(uint32 sourceID) = 0;
    virtual void Source_Pause(uint32 sourceID) = 0;
    virtual void Source_Stop(uint32 sourceID) = 0;
    virtual void Source_SetCurrentBufferTime(uint32 sourceID, float value) = 0;
    virtual float Source_GetCurrentBufferTime(uint32 id) = 0;
    virtual void Source_SetNonStreamingBuffer(uint32 sourceID, uint32 bufferID) = 0;
    virtual void Source_GetProcessedBuffersCount(uint32 sourceID, int32& processedBuffersCount) = 0;
    virtual void Source_GetQueuedBuffersCount(uint32 sourceID, int32& queuedBuffersCount) = 0;
    virtual void Source_QueueBuffer(uint32 sourceID, uint32 bufferID) = 0;
    virtual void Source_DequeueProcessedBuffers(uint32 sourceID) = 0;

    // Buffer
    virtual uint32 Buffer_Create() = 0;
    virtual void Buffer_Delete(uint32 bufferID) = 0;
    virtual void Buffer_Write(uint32 bufferID, byte* samples, const AudioDataInfo& info) = 0;

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
        FORCE_INLINE static void Reset()
        {
            Instance->Listener_Reset();
        }

        FORCE_INLINE static void VelocityChanged(const Vector3& velocity)
        {
            Instance->Listener_VelocityChanged(velocity);
        }

        FORCE_INLINE static void TransformChanged(const Vector3& position, const Quaternion& orientation)
        {
            Instance->Listener_TransformChanged(position, orientation);
        }

        FORCE_INLINE static void ReinitializeAll()
        {
            Instance->Listener_ReinitializeAll();
        }
    };

    class Source
    {
    public:
        FORCE_INLINE static uint32 Add(const AudioDataInfo& format, const Vector3& position, const Quaternion& orientation, float volume, float pitch, float pan, bool loop, bool spatial, float attenuation, float minDistance, float doppler)
        {
            return Instance->Source_Add(format, position, orientation, volume, pitch, pan, loop, spatial, attenuation, minDistance, doppler);
        }

        FORCE_INLINE static void Remove(uint32 sourceID)
        {
            Instance->Source_Remove(sourceID);
        }

        FORCE_INLINE static void VelocityChanged(uint32 sourceID, const Vector3& velocity)
        {
            Instance->Source_VelocityChanged(sourceID, velocity);
        }

        FORCE_INLINE static void TransformChanged(uint32 sourceID, const Vector3& position, const Quaternion& orientation)
        {
            Instance->Source_TransformChanged(sourceID, position, orientation);
        }

        FORCE_INLINE static void VolumeChanged(uint32 sourceID, float volume)
        {
            Instance->Source_VolumeChanged(sourceID, volume);
        }

        FORCE_INLINE static void PitchChanged(uint32 sourceID, float pitch)
        {
            Instance->Source_PitchChanged(sourceID, pitch);
        }

        FORCE_INLINE static void PanChanged(uint32 sourceID, float pan)
        {
            Instance->Source_PanChanged(sourceID, pan);
        }

        FORCE_INLINE static void IsLoopingChanged(uint32 sourceID, bool loop)
        {
            Instance->Source_IsLoopingChanged(sourceID, loop);
        }

        FORCE_INLINE static void SpatialSetupChanged(uint32 sourceID, bool spatial, float attenuation, float minDistance, float doppler)
        {
            Instance->Source_SpatialSetupChanged(sourceID, spatial, attenuation, minDistance, doppler);
        }

        FORCE_INLINE static void Play(uint32 sourceID)
        {
            Instance->Source_Play(sourceID);
        }

        FORCE_INLINE static void Pause(uint32 sourceID)
        {
            Instance->Source_Pause(sourceID);
        }

        FORCE_INLINE static void Stop(uint32 sourceID)
        {
            Instance->Source_Stop(sourceID);
        }

        FORCE_INLINE static void SetCurrentBufferTime(uint32 sourceID, float value)
        {
            Instance->Source_SetCurrentBufferTime(sourceID, value);
        }

        FORCE_INLINE static float GetCurrentBufferTime(uint32 sourceID)
        {
            return Instance->Source_GetCurrentBufferTime(sourceID);
        }

        FORCE_INLINE static void SetNonStreamingBuffer(uint32 sourceID, uint32 bufferID)
        {
            Instance->Source_SetNonStreamingBuffer(sourceID, bufferID);
        }

        FORCE_INLINE static void GetProcessedBuffersCount(uint32 sourceID, int32& processedBuffersCount)
        {
            Instance->Source_GetProcessedBuffersCount(sourceID, processedBuffersCount);
        }

        FORCE_INLINE static void GetQueuedBuffersCount(uint32 sourceID, int32& queuedBuffersCount)
        {
            Instance->Source_GetQueuedBuffersCount(sourceID, queuedBuffersCount);
        }

        FORCE_INLINE static void QueueBuffer(uint32 sourceID, uint32 bufferID)
        {
            Instance->Source_QueueBuffer(sourceID, bufferID);
        }

        FORCE_INLINE static void DequeueProcessedBuffers(uint32 sourceID)
        {
            Instance->Source_DequeueProcessedBuffers(sourceID);
        }
    };

    class Buffer
    {
    public:
        FORCE_INLINE static uint32 Create()
        {
            return Instance->Buffer_Create();
        }

        FORCE_INLINE static void Delete(uint32 bufferID)
        {
            Instance->Buffer_Delete(bufferID);
        }

        FORCE_INLINE static void Write(uint32 bufferID, byte* samples, const AudioDataInfo& info)
        {
            Instance->Buffer_Write(bufferID, samples, info);
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
