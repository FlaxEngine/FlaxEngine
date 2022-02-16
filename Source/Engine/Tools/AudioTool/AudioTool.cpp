// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "AudioTool.h"
#include "Engine/Core/Core.h"
#include "Engine/Core/Memory/Allocation.h"

void ConvertToMono8(const int8* input, uint8* output, uint32 numSamples, uint32 numChannels)
{
    for (uint32 i = 0; i < numSamples; i++)
    {
        int16 sum = 0;
        for (uint32 j = 0; j < numChannels; j++)
        {
            sum += *input;
            ++input;
        }

        *output = sum / numChannels;
        ++output;
    }
}

void ConvertToMono16(const int16* input, int16* output, uint32 numSamples, uint32 numChannels)
{
    for (uint32 i = 0; i < numSamples; i++)
    {
        int32 sum = 0;
        for (uint32 j = 0; j < numChannels; j++)
        {
            sum += *input;
            ++input;
        }

        *output = sum / numChannels;
        ++output;
    }
}

void Convert32To24Bits(const int32 input, uint8* output)
{
    const uint32 valToEncode = *(uint32*)&input;
    output[0] = (valToEncode >> 8) & 0x000000FF;
    output[1] = (valToEncode >> 16) & 0x000000FF;
    output[2] = (valToEncode >> 24) & 0x000000FF;
}

void ConvertToMono24(const uint8* input, uint8* output, uint32 numSamples, uint32 numChannels)
{
    for (uint32 i = 0; i < numSamples; i++)
    {
        int64 sum = 0;
        for (uint32 j = 0; j < numChannels; j++)
        {
            sum += AudioTool::Convert24To32Bits(input);
            input += 3;
        }

        const int32 avg = (int32)(sum / numChannels);
        Convert32To24Bits(avg, output);
        output += 3;
    }
}

void ConvertToMono32(const int32* input, int32* output, uint32 numSamples, uint32 numChannels)
{
    for (uint32 i = 0; i < numSamples; i++)
    {
        int64 sum = 0;
        for (uint32 j = 0; j < numChannels; j++)
        {
            sum += *input;
            ++input;
        }

        *output = (int32)(sum / numChannels);
        ++output;
    }
}

void Convert8To32Bits(const int8* input, int32* output, uint32 numSamples)
{
    for (uint32 i = 0; i < numSamples; i++)
    {
        const int8 val = input[i];
        output[i] = val << 24;
    }
}

void Convert16To32Bits(const int16* input, int32* output, uint32 numSamples)
{
    for (uint32 i = 0; i < numSamples; i++)
        output[i] = input[i] << 16;
}

void Convert24To32Bits(const uint8* input, int32* output, uint32 numSamples)
{
    for (uint32 i = 0; i < numSamples; i++)
    {
        output[i] = AudioTool::Convert24To32Bits(input);
        input += 3;
    }
}

void Convert32To8Bits(const int32* input, uint8* output, uint32 numSamples)
{
    for (uint32 i = 0; i < numSamples; i++)
        output[i] = (int8)(input[i] >> 24);
}

void Convert32To16Bits(const int32* input, int16* output, uint32 numSamples)
{
    for (uint32 i = 0; i < numSamples; i++)
        output[i] = (int16)(input[i] >> 16);
}

void Convert32To24Bits(const int32* input, uint8* output, uint32 numSamples)
{
    for (uint32 i = 0; i < numSamples; i++)
    {
        Convert32To24Bits(input[i], output);
        output += 3;
    }
}

void AudioTool::ConvertToMono(const byte* input, byte* output, uint32 bitDepth, uint32 numSamples, uint32 numChannels)
{
    switch (bitDepth)
    {
    case 8:
        ConvertToMono8((int8*)input, output, numSamples, numChannels);
        break;
    case 16:
        ConvertToMono16((int16*)input, (int16*)output, numSamples, numChannels);
        break;
    case 24:
        ConvertToMono24(input, output, numSamples, numChannels);
        break;
    case 32:
        ConvertToMono32((int32*)input, (int32*)output, numSamples, numChannels);
        break;
    default:
        CRASH;
        break;
    }
}

void AudioTool::ConvertBitDepth(const byte* input, uint32 inBitDepth, byte* output, uint32 outBitDepth, uint32 numSamples)
{
    int32* srcBuffer = nullptr;

    const bool needTempBuffer = inBitDepth != 32;
    if (needTempBuffer)
        srcBuffer = (int32*)Allocator::Allocate(numSamples * sizeof(int32));
    else
        srcBuffer = (int32*)input;

    // Convert it to a temporary 32-bit buffer and then use that to convert to actual requested bit depth. 
    // It could be more efficient to convert directly from source to requested depth without a temporary buffer,
    // at the cost of additional complexity. If this method ever becomes a performance issue consider that.

    switch (inBitDepth)
    {
    case 8:
        Convert8To32Bits((int8*)input, srcBuffer, numSamples);
        break;
    case 16:
        Convert16To32Bits((int16*)input, srcBuffer, numSamples);
        break;
    case 24:
        ::Convert24To32Bits(input, srcBuffer, numSamples);
        break;
    case 32:
        // Do nothing
        break;
    default:
        CRASH;
        break;
    }

    switch (outBitDepth)
    {
    case 8:
        Convert32To8Bits(srcBuffer, output, numSamples);
        break;
    case 16:
        Convert32To16Bits(srcBuffer, (int16*)output, numSamples);
        break;
    case 24:
        Convert32To24Bits(srcBuffer, output, numSamples);
        break;
    case 32:
        Platform::MemoryCopy(output, srcBuffer, numSamples * sizeof(int32));
        break;
    default:
        CRASH;
        break;
    }

    if (needTempBuffer)
    {
        Allocator::Free(srcBuffer);
        srcBuffer = nullptr;
    }
}

void AudioTool::ConvertToFloat(const byte* input, uint32 inBitDepth, float* output, uint32 numSamples)
{
    if (inBitDepth == 8)
    {
        for (uint32 i = 0; i < numSamples; i++)
        {
            const int8 sample = *(int8*)input;
            output[i] = sample / 127.0f;

            input++;
        }
    }
    else if (inBitDepth == 16)
    {
        for (uint32 i = 0; i < numSamples; i++)
        {
            const int16 sample = *(int16*)input;
            output[i] = sample / 32767.0f;

            input += 2;
        }
    }
    else if (inBitDepth == 24)
    {
        for (uint32 i = 0; i < numSamples; i++)
        {
            const int32 sample = Convert24To32Bits(input);
            output[i] = sample / 2147483647.0f;

            input += 3;
        }
    }
    else if (inBitDepth == 32)
    {
        for (uint32 i = 0; i < numSamples; i++)
        {
            const int32 sample = *(int32*)input;
            output[i] = sample / 2147483647.0f;

            input += 4;
        }
    }
    else
    {
        CRASH;
    }
}

void AudioTool::ConvertFromFloat(const float* input, int32* output, uint32 numSamples)
{
    for (uint32 i = 0; i < numSamples; i++)
    {
        const float sample = *(float*)input;
        output[i] = static_cast<int32>(sample * 2147483647.0f);

        input++;
    }
}
