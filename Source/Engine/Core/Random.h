// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
}
