// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// To load the IES file image format. IES files exist for many real world lights. The file stores how much light is emitted in a specific direction.
/// The data is usually measured but tools to paint IES files exist.
/// </summary>
class IESLoader
{
private:

    float _brightness;
    float _cachedIntegral;

    Array<float> _hAngles;
    Array<float> _vAngles;
    Array<float> _candalaValues;

public:

    /// <summary>
    /// Init
    /// </summary>
    IESLoader();

public:

    /// <summary>
    /// Load IES file
    /// </summary>
    /// <param name="buffer">Buffer with data</param>
    /// <returns>True if cannot load, otherwise false</returns>
    FORCE_INLINE bool Load(const Array<byte>& buffer)
    {
        return Load(buffer.Get());
    }

    /// <summary>
    /// Load IES file
    /// </summary>
    /// <param name="buffer">Buffer with data</param>
    /// <returns>True if cannot load, otherwise false</returns>
    bool Load(const byte* buffer);

public:

    /// <summary>
    /// Gets width
    /// </summary>
    /// <returns>Width in pixels</returns>
    FORCE_INLINE uint32 GetWidth() const
    {
        // We use contant size
        return 256;
    }

    /// <summary>
    /// Gets height
    /// </summary>
    /// <returns>Height in pixels</returns>
    FORCE_INLINE uint32 GetHeight() const
    {
        // We use contant size
        return 1;
    }

    /// <summary>
    /// Gets IES profile brightness value in lumens (always > 0)
    /// </summary>
    /// <returns>Brightness (in lumens)</returns>
    FORCE_INLINE float GetBrightness() const
    {
        return _brightness;
    }

public:

    /// <summary>
    /// Extracts IES profile data to R16F format
    /// </summary>
    /// <param name="output">Result data container</param>
    /// <returns>Multiplier as the texture is normalized</returns>
    float ExtractInR16F(Array<byte>& output);

public:

    float InterpolatePoint(int32 X, int32 Y) const;
    float InterpolateBilinear(float fX, float fY) const;

    /// <summary>
    /// Compute the Candala value for a given direction
    /// </summary>
    /// <param name="HAngle">HAngle (in degrees e.g. 0..180)</param>
    /// <param name="VAngle">VAngle (in degrees e.g. 0..180)</param>
    /// <returns>Sampled value</returns>
    float Interpolate2D(float HAngle, float VAngle) const;

    /// <summary>
    /// Compute the Candala value for a given direction (integrates over HAngle)
    /// </summary>
    /// <param name="VAngle">VAngle (in degrees e.g. 0..180)</param>
    /// <returns>Sampled value</returns>
    float Interpolate1D(float VAngle) const;

public:

    /// <summary>
    /// Calculates maximum value
    /// </summary>
    /// <returns>Maximum value</returns>
    float ComputeMax() const;

    /// <summary>
    /// Integrate over the unit sphere
    /// </summary>
    /// <returns>Integral value</returns>
    float ComputeFullIntegral();

    /// <summary>
    /// Computes filtering position for given value in sorted set of values
    /// </summary>
    /// <param name="value">Value to find</param>
    /// <param name="sortedValues">Value to use</param>
    /// <returns>Filter position</returns>
    static float ComputeFilterPos(float value, const Array<float>& sortedValues);
};
