// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include <stdlib.h>

namespace Random
{
    /// <summary>
    /// Generates a pseudo-random number from normalized range [0;1].
    /// </summary>
    /// <returns>The random number.</returns>
    inline float Rand()
    {
        return (float)rand() / (float)RAND_MAX;
    }

    /// <summary>
    /// Generates a pseudo-random number from specific range.
    /// </summary>
    /// <param name="min">The minimum value (inclusive).</param>
    /// <param name="max">The maximum value (inclusive).</param>
    /// <returns>The random number.</returns>
    inline float RandRange(float min, float max)
    {
        return min + (max - min) * Rand();
    }
}
