// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingType.h"
#include "AudioDevice.h"
#include "Types.h"

/// <summary>
/// The audio service used for music and sound effects playback.
/// </summary>
API_CLASS(Static, Attributes="DebugCommand") class FLAXENGINE_API Audio
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(Audio);
    friend class AudioStreamingHandler;
    friend class AudioClip;

public:
    /// <summary>
    /// The audio listeners collection registered by the service.
    /// </summary>
    static Array<AudioListener*> Listeners;

    /// <summary>
    /// The audio sources collection registered by the service.
    /// </summary>
    static Array<AudioSource*> Sources;

    /// <summary>
    /// The all audio devices.
    /// </summary>
    API_FIELD(ReadOnly) static Array<AudioDevice> Devices;

    /// <summary>
    /// Event called when audio devices collection gets changed.
    /// </summary>
    API_EVENT() static Action DevicesChanged;

    /// <summary>
    /// Event called when the active audio device gets changed.
    /// </summary>
    API_EVENT() static Action ActiveDeviceChanged;

public:
    /// <summary>
    /// Gets the active device.
    /// </summary>
    /// <returns>The active device.</returns>
    API_PROPERTY() static AudioDevice* GetActiveDevice();

    /// <summary>
    /// Gets the index of the active device.
    /// </summary>
    /// <returns>The active device index.</returns>
    API_PROPERTY() static int32 GetActiveDeviceIndex();

    /// <summary>
    /// Sets the index of the active device.
    /// </summary>
    /// <param name="index">The index.</param>
    API_PROPERTY() static void SetActiveDeviceIndex(int32 index);

public:
    /// <summary>
    /// Gets the master volume applied to all the audio sources (normalized to range 0-1).
    /// </summary>
    /// <returns>The value</returns>
    API_PROPERTY() static float GetMasterVolume();

    /// <summary>
    /// Sets the master volume applied to all the audio sources (normalized to range 0-1).
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() static void SetMasterVolume(float value);

    /// <summary>
    /// Gets the actual master volume (including all side effects and mute effectors).
    /// </summary>
    /// <returns>The final audio volume applied to all the listeners.</returns>
    API_PROPERTY() static float GetVolume();

    /// <summary>
    /// Sets the doppler effect factor. Scale for source and listener velocities. Default is 1.
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() static void SetDopplerFactor(float value);

    /// <summary>
    /// Gets the preference to use HRTF audio (when available on platform). Default is true.
    /// </summary>
    API_PROPERTY() static bool GetEnableHRTF();

    /// <summary>
    /// Sets the preference to use HRTF audio (when available on platform). Default is true.
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() static void SetEnableHRTF(bool value);
};
