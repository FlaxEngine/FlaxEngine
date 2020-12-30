// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Types/BaseTypes.h"
#include "Math/Vector3.h"
#include "Math/Math.h"
#include <stdlib.h>

/// <summary>
/// Very basic pseudo numbers generator.
/// </summary>
class RandomStream
{
private:

    /// <summary>
    /// Holds the initial seed.
    /// </summary>
    int32 _initialSeed;

    /// <summary>
    /// Holds the current seed.
    /// </summary>
    mutable int32 _seed;

public:

    /// <summary>
    /// Init
    /// </summary>
    RandomStream()
        : _initialSeed(0)
        , _seed(0)
    {
    }

    /// <summary>
    /// Creates and initializes a new random stream from the specified seed value.
    /// </summary>
    /// <param name="seed">The seed value.</param>
    RandomStream(int32 seed)
        : _initialSeed(seed)
        , _seed(seed)
    {
    }

public:

    /// <summary>
    /// Gets initial seed value
    /// </summary>
    /// <returns>Initial seed.</returns>
    int32 GetInitialSeed() const
    {
        return _initialSeed;
    }

public:

    /// <summary>
    /// Gets the current seed.
    /// </summary>
    /// <returns>Current seed value.</returns>
    int32 GetCurrentSeed() const
    {
        return _seed;
    }

    /// <summary>
    /// Initializes this random stream with the specified seed value.
    /// </summary>
    /// <param name="seed">The seed value.</param>
    void Initialize(int32 seed)
    {
        _initialSeed = seed;
        _seed = seed;
    }

    /// <summary>
    /// Resets this random stream to the initial seed value.
    /// </summary>
    void Reset() const
    {
        _seed = _initialSeed;
    }

    /// <summary>
    /// Generates a new random seed.
    /// </summary>
    void GenerateNewSeed()
    {
        Initialize(rand());
    }

public:

    /// <summary>
    /// Returns a random number between 0 and MAXUINT.
    /// </summary>
    /// <returns>Random number.</returns>
    uint32 GetUnsignedInt() const
    {
        MutateSeed();

        return *(uint32*)&_seed;
    }

    /// <summary>
    /// Returns a random number between 0 and 1.
    /// </summary>
    /// <returns>Random number.</returns>
    float GetFraction() const
    {
        MutateSeed();
        const float temp = 1.0f;
        float result;
        *(int32*)&result = (*(int32*)&temp & 0xff800000) | (_seed & 0x007fffff);
        return Math::Fractional(result);
    }

    /// <summary>
    /// Returns a random number between 0 and 1.
    /// </summary>
    /// <returns>Random number.</returns>
    FORCE_INLINE float Rand() const
    {
        return GetFraction();
    }

    /// <summary>
    /// Returns a random vector of unit size.
    /// </summary>
    /// <returns>Random unit length vector</returns>
    Vector3 GetUnitVector() const
    {
        Vector3 result;
        float l;
        do
        {
            result.X = GetFraction() * 2.0f - 1.0f;
            result.Y = GetFraction() * 2.0f - 1.0f;
            result.Z = GetFraction() * 2.0f - 1.0f;
            l = result.LengthSquared();
        } while (l > 1.0f || l < ZeroTolerance);
        return Vector3::Normalize(result);
    }

    /// <summary>
    /// Helper function for rand implementations.
    /// </summary>
    /// <param name="A">Top border</param>
    /// <returns>A random number in [0..A)</returns>
    FORCE_INLINE int32 RandHelper(int32 A) const
    {
        return A > 0 ? Math::TruncToInt(GetFraction() * ((float)A - ZeroTolerance)) : 0;
    }

    /// <summary>
    /// Helper function for rand implementations
    /// </summary>
    /// <param name="min">Minimum value</param>
    /// <param name="max">Maximum value</param>
    /// <returns>A random number in [min; max] range</returns>
    FORCE_INLINE int32 RandRange(int32 min, int32 max) const
    {
        const int32 range = max - min + 1;
        return min + RandHelper(range);
    }

    /// <summary>
    /// Helper function for rand implementations
    /// </summary>
    /// <param name="min">Minimum value</param>
    /// <param name="max">Maximum value</param>
    /// <returns>A random number in [min; max] range</returns>
    FORCE_INLINE float RandRange(float min, float max) const
    {
        return min + (max - min) * Rand();
    }

protected:

    /// <summary>
    /// Mutates the current seed into the next seed.
    /// </summary>
    void MutateSeed() const
    {
        // This can be modified to provide better randomization
        _seed = _seed * 196314165 + 907633515;
    }
};
