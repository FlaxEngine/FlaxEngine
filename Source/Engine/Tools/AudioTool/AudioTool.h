// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_AUDIO_TOOL

#include "Engine/Core/Config.h"
#include "Engine/Core/Types/BaseTypes.h"
#if USE_EDITOR
#include "Engine/Core/ISerializable.h"
#endif
#include "Engine/Audio/Types.h"

/// <summary>
/// Audio data importing and processing utilities.
/// </summary>
API_CLASS(Namespace="FlaxEngine.Tools", Static) class FLAXENGINE_API AudioTool
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(AudioTool);

#if USE_EDITOR

public:
    /// <summary>
    /// Declares the imported audio clip bit depth.
    /// </summary>
    API_ENUM(Attributes="HideInEditor") enum class BitDepth : int32
    {
        // 8-bits per sample.
        _8 = 8,
        // 16-bits per sample.
        _16 = 16,
        // 24-bits per sample.
        _24 = 24,
        // 32-bits per sample.
        _32 = 32,
    };

    /// <summary>
    /// Audio import options.
    /// </summary>
    API_STRUCT(Attributes="HideInEditor") struct FLAXENGINE_API Options : public ISerializable
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(Options);

        /// <summary>
        /// The audio data format to import the audio clip as.
        /// </summary>
        API_FIELD(Attributes="EditorOrder(10)")
        AudioFormat Format = AudioFormat::Vorbis;

        /// <summary>
        /// The audio data compression quality. Used only if target format is using compression. Value 0 means the smallest size, value 1 means the best quality.
        /// </summary>
        API_FIELD(Attributes="EditorOrder(20), EditorDisplay(null, \"Compression Quality\"), Limit(0, 1, 0.01f)")
        float Quality = 0.4f;

        /// <summary>
        /// Disables dynamic audio streaming. The whole clip will be loaded into the memory. Useful for small clips (eg. gunfire sounds).
        /// </summary>
        API_FIELD(Attributes="EditorOrder(30)")
        bool DisableStreaming = false;

        /// <summary>
        /// Checks should the clip be played as spatial (3D) audio or as normal audio. 3D audio is stored in Mono format.
        /// </summary>
        API_FIELD(Attributes="EditorOrder(40), EditorDisplay(null, \"Is 3D\")")
        bool Is3D = false;

        /// <summary>
        /// The size of a single sample in bits. The clip will be converted to this bit depth on import.
        /// </summary>
        API_FIELD(Attributes="EditorOrder(50), VisibleIf(nameof(ShowBtiDepth))")
        BitDepth BitDepth = BitDepth::_16;

        String ToString() const;

        // [ISerializable]
        void Serialize(SerializeStream& stream, const void* otherObj) override;
        void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    };
#endif

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

#endif
