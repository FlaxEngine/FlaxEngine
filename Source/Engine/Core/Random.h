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

    /// <summary>
    ///Generates a random number within a specified range.
    /// </summary>
    /// <param name="min">The minimum value of the range.</param>
    /// <param name="max">The maximum value of the range.</param>
    /// <returns>The random <see cref="float"/>.</returns>
    inline float RandRange(float min, float max)
    {
        return min + (max - min) * Rand();
    }

    /// <summary>
    /// Generates a random number within a specified range.
    /// </summary>
    /// <param name="min">The minimum value of the range.</param>
    /// <param name="max">The maximum value of the range.</param>
    /// <returns>The random <see cref="int"/>.</returns>
    inline int RandRange(int min, int max)
    {
        return min + (max - min) * Rand();
    }

    /// <summary>
    ///Generates a random number within a specified range.
    /// </summary>
    /// <param name="min">The minimum value of the range.</param>
    /// <param name="max">The maximum value of the range.</param>
    /// <returns>The random <see cref="double"/>.</returns>
    inline double RandRange(double min, double max)
    {
        return min + (max - min) * (double)Rand();
    }

    /// <summary>
    /// Generates a random point inside a sphere with radius 1.0.
    /// </summary>
    /// <returns>A random <see cref="Float3"/> within the sphere.</returns>
    inline Float3 GetPointInsideSphere()
    {
        Float3 output;
        float l;

        do
        {
            // Create random float with a mean of 0 and deviation of ±1
            output.X = Rand() * 2.0f - 1.0f;
            output.Y = Rand() * 2.0f - 1.0f;
            output.Z = Rand() * 2.0f - 1.0f;

            l = output.LengthSquared();
        } while (l > 1 || l < ZeroTolerance);

        output.Normalize();

        return output;
    }

    /// <summary>
      /// Generates a random point on the surface of a sphere with radius 1.0.
      /// </summary>
      /// <returns>A random <see cref="Float3"/>.</returns>
    inline Float3 GetPointOnSphere()
    {
        float z = 2 * Rand() - 1;
        float t = 2 * PI * Rand();

        float r = sqrt(1 - z * z);
        float x = r * cos(t);
        float y = r * sin(t);

        return Float3(x, y, z);
    }

    /// <summary>
    /// Generates a random uniform rotation quaternion.
    /// </summary>
    /// <returns>A random <see cref="Quaternion"/>.</returns>
    /// <param name="randomRoll">Should the roll value be randomized.</param>
    inline Quaternion GetUniformOrientation(bool randomRoll = false)
    {
        float randomYaw = Rand() * 360.0f;
        float randomPitch = Rand() * 360.0f; 
        float randomRoll = randomRoll ? Rand() * 360.0f : 0.0f; 

        return Quaternion::Euler(randomYaw, randomPitch, randomRoll);
    }

}
