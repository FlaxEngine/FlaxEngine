// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"
#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// Audio data importing and processing utilities.
/// </summary>
class FLAXENGINE_API AudioTool
{
public:

    /// <summary>
    /// Converts a set of audio samples using multiple channels into a set of mono samples.
    /// </summary>
    /// <param name="input">A set of input samples. Per-channels samples should be interleaved. Size of each sample is determined by bitDepth. Total size of the buffer should be (numSamples * numChannels * bitDepth / 8).</param>
    /// <param name="output">The pre-allocated buffer to store the mono samples. Should be of (numSamples * bitDepth / 8) size.</param>
    /// <param name="bitDepth">The size of a single sample in bits.</param>
    /// <param name="numSamples">The number of samples per a single channel.</param>
    /// <param name="numChannels">The number of channels in the input data.</param>
    static void ConvertToMono(const byte* input, byte* output, uint32 bitDepth, uint32 numSamples, uint32 numChannels);

    /// <summary>
    /// Converts a set of audio samples of a certain bit depth to a new bit depth.
    /// </summary>
    /// <param name="input">A set of input samples. Total size of the buffer should be *numSamples * inBitDepth / 8).</param>
    /// <param name="inBitDepth">The size of a single sample in the input array, in bits.</param>
    /// <param name="output">The pre-allocated buffer to store the output samples in. Total size of the buffer should be (numSamples * outBitDepth / 8).</param>
    /// <param name="outBitDepth">Size of a single sample in the output array, in bits.</param>
    /// <param name="numSamples">The total number of samples to process.</param>
    static void ConvertBitDepth(const byte* input, uint32 inBitDepth, byte* output, uint32 outBitDepth, uint32 numSamples);

    /// <summary>
    /// Converts a set of audio samples of a certain bit depth to a set of floating point samples in range [-1, 1].
    /// </summary>
    /// <param name="input">A set of input samples. Total size of the buffer should be (numSamples * inBitDepth / 8). All input samples should be signed integers.</param>
    /// <param name="inBitDepth">The size of a single sample in the input array, in bits.</param>
    /// <param name="output">The pre-allocated buffer to store the output samples in. Total size of the buffer should be numSamples * sizeof(float).</param>
    /// <param name="numSamples">The total number of samples to process.</param>
    static void ConvertToFloat(const byte* input, uint32 inBitDepth, float* output, uint32 numSamples);

    /// <summary>
    /// Converts a set of audio samples of floating point samples in range [-1, 1] to a 32-bit depth PCM data.
    /// </summary>
    /// <param name="input">A set of input samples. Total size of the buffer should be (numSamples * sizeof(float)). All input samples should be in range [-1, 1].</param>
    /// <param name="output">The pre-allocated buffer to store the output samples in. Total size of the buffer should be numSamples * sizeof(float).</param>
    /// <param name="numSamples">The total number of samples to process.</param>
    static void ConvertFromFloat(const float* input, int32* output, uint32 numSamples);

    /// <summary>
    /// Converts a 24-bit signed integer into a 32-bit signed integer.
    /// </summary>
    /// <param name="input">The 24-bit signed integer as an array of 3 bytes.</param>
    /// <returns>The 32-bit signed integer.</returns>
    FORCE_INLINE static int32 Convert24To32Bits(const byte* input)
    {
        return (input[2] << 24) | (input[1] << 16) | (input[0] << 8);
    }
};
