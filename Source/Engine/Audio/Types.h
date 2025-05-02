// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

class Audio;
class AudioClip;
class AudioListener;
class AudioSource;

/// <summary>
/// Audio data storage format used by the runtime.
/// </summary>
API_ENUM() enum class AudioFormat
{
    /// <summary>
    /// The raw PCM data.
    /// </summary>
    Raw,

    /// <summary>
    /// The Vorbis data.
    /// </summary>
    Vorbis,
};

/// <summary>
/// Meta-data describing a chunk of audio.
/// </summary>
API_STRUCT(NoDefault) struct AudioDataInfo
{
DECLARE_SCRIPTING_TYPE_MINIMAL(AudioDataInfo);

    /// <summary>
    /// The total number of audio samples in the audio data (includes all channels).
    /// </summary>
    API_FIELD() uint32 NumSamples;

    /// <summary>
    /// The number of audio samples per second, per channel.
    /// </summary>
    API_FIELD() uint32 SampleRate;

    /// <summary>
    /// The number of channels. Each channel has its own set of samples.
    /// </summary>
    API_FIELD() uint32 NumChannels;

    /// <summary>
    /// The number of bits per sample.
    /// </summary>
    API_FIELD() uint32 BitDepth;

    /// <summary>
    /// Gets the length of the audio data (in seconds).
    /// </summary>
    float GetLength() const;
};
