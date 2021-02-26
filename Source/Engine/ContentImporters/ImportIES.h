// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_ASSETS_IMPORTER

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Utility for loading IES files and extract light emission information.
/// </summary>
class ImportIES
{
private:

    float _brightness = 0;
    Array<float> _hAngles;
    Array<float> _vAngles;
    Array<float> _candalaValues;

public:

    /// <summary>
    /// Loads the IES file.
    /// </summary>
    /// <param name="buffer">The buffer with data.</param>
    /// <returns>True if cannot load, otherwise false.</returns>
    bool Load(const byte* buffer);

    /// <summary>
    /// Extracts IES profile data to R16 format (float).
    /// </summary>
    /// <param name="output">The result data container.</param>
    /// <returns>Thr multiplier as the texture is normalized.</returns>
    float ExtractInR16(Array<byte>& output);

public:

    uint32 GetWidth() const
    {
        return 256;
    }

    uint32 GetHeight() const
    {
        return 1;
    }

    float GetBrightness() const
    {
        return _brightness;
    }

private:

    float InterpolatePoint(int32 x, int32 y) const;
    float InterpolateBilinear(float x, float y) const;
    static float ComputeFilterPos(float value, const Array<float>& sortedValues);
};

#endif
