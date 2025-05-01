// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Audio/Types.h"
#include "Engine/Core/Types/DataContainer.h"

/// <summary>
/// Interface used for implementations that encodes set of PCM samples into a target audio format.
/// </summary>
class AudioEncoder
{
public:

    /// <summary>
    /// Finalizes an instance of the <see cref="AudioEncoder"/> class.
    /// </summary>
    virtual ~AudioEncoder()
    {
    }

public:

    /// <summary>
    /// Converts the input PCM samples buffer into the encoder audio format.
    /// </summary>
    /// <param name="samples">The buffer containing samples in PCM format. All samples should be in signed integer format.</param>
    /// <param name="info">The input information describing meta-data of the audio in the samples buffer.</param>
    /// <param name="result">The output data.</param>
    /// <param name="quality">The output data compression quality (normalized in range [0;1]).</param>
    /// <returns>True if the data is invalid or conversion failed, otherwise false.</returns>
    virtual bool Convert(byte* samples, AudioDataInfo& info, BytesContainer& result, float quality = 0.5f) = 0;
};
